/*
******************************************************************************
* @file    recorder.c
* @author  fire
* @version V1.0
* @date    2015-xx-xx
* @brief   WM8978放音功能测试+mp3解码
******************************************************************************
* @attention
*
* 实验平台:秉火  STM32 F767 开发板  
* 论坛    :http://www.chuxue123.com
* 淘宝    :http://firestm32.taobao.com
*
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "./usart/bsp_usart.h"
#include "./wm8978/bsp_wm8978.h"
#include "ff.h" 
#include "./mp3Player/mp3Player.h"
#include "mp3dec.h"
#include "GUI_MUSICPLAYER_DIALOG.h"
#include "./sai/bsp_sai.h" 
#include "x_libc.h"
#include "GUI_MUSICPLAYER_DIALOG.h"
/* 推荐使用以下格式mp3文件：
 * 采样率：44100Hz
 * 声  道：2
 * 比特率：320kbps
 */

/* 处理立体声音频数据时，输出缓冲区需要的最大大小为2304*16/8字节(16为PCM数据为16位)，
 * 这里我们定义MP3BUFFER_SIZE为2304，实际输出缓冲区为MP3BUFFER_SIZE*2个字节
 */
#define MP3BUFFER_SIZE  2304		
#define INPUTBUF_SIZE   3000	

static HMP3Decoder		Mp3Decoder;			/* mp3解码器指针	*/
static MP3FrameInfo		Mp3FrameInfo;		/* mP3帧信息  */

MP3_TYPE mp3player;         /* mp3播放设备 */
volatile uint8_t Isread=0;           /* DMA传输完成标志 */
static uint8_t bufflag=0;          /* 数据缓存区选择标志 */

uint32_t led_delay=0;

uint8_t inputbuf[INPUTBUF_SIZE]={0};        /* 解码输入缓冲区，1940字节为最大MP3帧大小  */
short outbuffer[2][MP3BUFFER_SIZE];  /* 解码输出缓冲区，也是I2S输入数据，实际占用字节数：RECBUFFER_SIZE*2 */


FIL file;											/* file objects */
FRESULT result; 
UINT bw;            					/* File R/W count */






/**
  * @brief  获取MP3ID3V2文件头的大小
  * @param  输入MP3文件开头的数据，至少10个字节
  * @retval ID3V2的大小
  */
uint32_t mp3_GetID3V2_Size(unsigned char *buf)
{
 uint32_t ID3V2_size;
	
 if(buf[0] == 'I' && buf[1] == 'D' && buf[2] =='3')//存在ID3V2
 {
 	 ID3V2_size = (buf[6]<<21) | (buf[7]<<14) | (buf[8]<<7) | buf[9];
   ID3V2_size = (buf[6]&0x7F)*0x200000+ (buf[7]&0x7F)*0x400 + (buf[8]&0x7F)*0x80 +(buf[9]&0x7F);
 }
 else//不存在ID3V2
	 ID3V2_size = 0;

 return ID3V2_size;

}
/**
  * @brief   MP3格式音频播放主程序
  * @param  无
  * @retval 无
  */
int times = 0;
void mp3PlayerDemo(const char *mp3file)
{
	uint8_t *read_ptr=inputbuf;
	uint32_t frames=0;
  DWORD pos;//记录文字变量
  static uint8_t timecount = 0;
	int err=0, i=0, outputSamps=0;
  static uint8_t lyriccount=0;//歌词index记录	
	int	read_offset = 0;				/* 读偏移指针 */
	int	bytes_left = 0;					/* 剩余字节数 */	
  uint32_t time_sum = 0; //计算当前已播放到的时间位置
  uint16_t frame_size;//MP3帧的大小
	uint32_t ID3V2_size;//MP3的ID3V2的大小
	WCHAR wbuf[128];//保存文本数组
	mp3player.ucFreq = SAI_AUDIOFREQ_DEFAULT;
	mp3player.ucStatus = STA_IDLE;
	mp3player.ucVolume = MusicDialog.power;
	
	result=f_open(&file,mp3file,FA_READ);
	if(result!=FR_OK)
	{
		printf("Open mp3file :%s fail!!!->%d\r\n",mp3file,result);
		result = f_close (&file);
		return;	/* 停止播放 */
	}
	printf("当前播放文件 -> %s\n",mp3file);
	
	//初始化MP3解码器
	Mp3Decoder = MP3InitDecoder();	
	if(Mp3Decoder==0)
	{
		printf("初始化helix解码库设备\n");
		return;	/* 停止播放 */
	}
	printf("初始化中...\n");
	
	Delay_ms(10);	/* 延迟一段时间，等待I2S中断结束 */
	wm8978_Reset();		/* 复位WM8978到复位状态 */
	/* 配置WM8978芯片，输入为DAC，输出为耳机 */
  if(music_icon[2].state == FALSE)
  {
    wm8978_CfgAudioPath(DAC_ON,  EAR_LEFT_ON|EAR_RIGHT_ON);
  }
  //当音量icon被按下时，设置为静音模式
  else
  {                
    wm8978_CfgAudioPath(DAC_ON,  SPK_ON|EAR_LEFT_ON|EAR_RIGHT_ON);
  } 

	/* 调节音量，左右相同音量 */
	wm8978_SetOUT1Volume(mp3player.ucVolume);

	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	wm8978_CfgAudioIF(SAI_I2S_STANDARD, 16);
	
	/*  初始化并配置I2S  */
  SAI_Play_Stop();
	SAI_GPIO_Config();
  SAI_DMA_TX_Callback = MusicPlayer_SAI_DMA_TX_Callback;
//	SAIxA_Tx_Config(SAI_I2S_STANDARD,SAI_PROTOCOL_DATASIZE_16BIT,mp3player.ucFreq);	
//	SAIA_TX_DMA_Init();	
	
	bufflag=0;
	Isread=0;

	MusicDialog.chgsch=0;
  lyriccount=0;
  timecount=0;

	mp3player.ucStatus = STA_PLAYING;		/* 放音状态 */
  result=f_read(&file,inputbuf,INPUTBUF_SIZE,&bw);
	if(result!=FR_OK)
	{
		printf("读取%s失败 -> %d\r\n",mp3file,result);
		MP3FreeDecoder(Mp3Decoder);
		return;
	}

  ID3V2_size = mp3_GetID3V2_Size(inputbuf);
	read_ptr=inputbuf;
	bytes_left=bw;
	/* 进入主程序循环体 */
	while(mp3player.ucStatus == STA_PLAYING)
	{
    if(!(music_icon[6].state == FALSE))
    {
      GUI_msleep(10);
      continue;
    }
		read_offset = MP3FindSyncWord(read_ptr, bytes_left);	//寻找帧同步，返回第一个同步字的位置
		if(read_offset < 0)										//没有找到同步字
		{
			result=f_read(&file,inputbuf,INPUTBUF_SIZE,&bw);
			if(result!=FR_OK)
			{
				printf("读取%s失败 -> %d\r\n",mp3file,result);
				break;
			}
			read_ptr=inputbuf;
			bytes_left=bw;
			continue;		//跳出循环2，回到循环1	
		}
		read_ptr += read_offset;				//偏移至同步字的位置
		bytes_left -= read_offset;				//同步字之后的数据大小	
		if(bytes_left < 1024)							//补充数据
		{
			/* 注意这个地方因为采用的是DMA读取，所以一定要4字节对齐  */
			i=(uint32_t)(bytes_left)&3;			//判断多余的字节
			if(i) i=4-i;						//需要补充的字节
			memcpy(inputbuf+i, read_ptr, bytes_left);	//从对齐位置开始复制
			read_ptr = inputbuf+i;										//指向数据对齐位置
			result = f_read(&file, inputbuf+bytes_left+i, INPUTBUF_SIZE-bytes_left-i, &bw);//补充数据
			if(result!=FR_OK)
			{
				printf("读取%s失败 -> %d\r\n",mp3file,result);
				break;
			}
			bytes_left += bw;		//有效数据流大小
		}
		err = MP3Decode(Mp3Decoder, &read_ptr, &bytes_left, outbuffer[bufflag], 0);	//bufflag开始解码 参数：mp3解码结构体、输入流指针、输入流大小、输出流指针、数据格式
    time_sum +=26;//每帧26ms      
		frames++;	
		if (err != ERR_MP3_NONE)	//错误处理
		{
			switch (err)
			{
				case ERR_MP3_INDATA_UNDERFLOW:
					printf("ERR_MP3_INDATA_UNDERFLOW\r\n");
					result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
					read_ptr = inputbuf;
					bytes_left = bw;
					break;		
				case ERR_MP3_MAINDATA_UNDERFLOW:
					/* do nothing - next call to decode will provide more mainData */
					printf("ERR_MP3_MAINDATA_UNDERFLOW\r\n");
					break;		
				default:
					printf("UNKNOWN ERROR:%d\r\n", err);		
					// 跳过此帧
					if (bytes_left > 0)
					{
						bytes_left --;
						read_ptr ++;
					}	
					break;
			}
			Isread=1;
		}
   
		else		//解码无错误，准备把数据输出到PCM
		{
			MP3GetLastFrameInfo(Mp3Decoder, &Mp3FrameInfo);		//获取解码信息				
			/* 输出到DAC */
			outputSamps = Mp3FrameInfo.outputSamps;							//PCM数据个数
			if (outputSamps > 0)
			{
				if (Mp3FrameInfo.nChans == 1)	//单声道
				{
					//单声道数据需要复制一份到另一个声道
					for (i = outputSamps - 1; i >= 0; i--)
					{
						outbuffer[bufflag][i * 2] = outbuffer[bufflag][i];
						outbuffer[bufflag][i * 2 + 1] = outbuffer[bufflag][i];
					}
					outputSamps *= 2;
				}//if (Mp3FrameInfo.nChans == 1)	//单声道
			}//if (outputSamps > 0)
			
			/* 根据解码信息设置采样率 */
			if (Mp3FrameInfo.samprate != mp3player.ucFreq)	//采样率 
			{
        printf("Mp3FrameInfo%d\n",Mp3FrameInfo.samprate);
				mp3player.ucFreq = Mp3FrameInfo.samprate;
        frame_size = (((Mp3FrameInfo.version == MPEG1)? 144:72)*Mp3FrameInfo.bitrate)/Mp3FrameInfo.samprate+Mp3FrameInfo.paddingBit;				
        MusicDialog.alltime=(((file.fsize-ID3V2_size-128)/frame_size)*26+1000)/1000;
        
       
				if(mp3player.ucFreq >= SAI_AUDIOFREQ_DEFAULT)	//I2S_AudioFreq_Default = 2，正常的帧，每次都要改速率
				{

          
					SAIxA_Tx_Config(SAI_I2S_STANDARD,SAI_PROTOCOL_DATASIZE_16BIT,mp3player.ucFreq);						//根据采样率修改iis速率
          SAIA_TX_DMA_Init((uint16_t *)outbuffer[0],(uint16_t *)outbuffer[1],outputSamps);
				}

				SAI_Play_Start();
			}
		}//else 解码正常
		
		if(file.fptr==file.fsize) 		//mp3文件读取完成，退出
		{
      mp3player.ucStatus=STA_SWITCH;
      MusicDialog.playindex++;
      if(MusicDialog.playindex > MusicDialog.music_file_num)
        MusicDialog.playindex = 0;      
			printf("END\r\n");
			break;
		}	
    int update = 0;//记录只更新一次的参数
//    static int times= 0 ;
		while(Isread==0)
		{
      if(MusicDialog.chgsch == 0)
      {
        //times = 0;
        if(timecount>=20)
        {
          //当前值
          MusicDialog.curtime = time_sum/1000;
          
     
          
          if(!MusicDialog.mList_State && update == 0)//进入列表界面，不进行更新
          {
            update = 1;
            x_wsprintf(wbuf, L"%02d:%02d",MusicDialog.alltime/60,MusicDialog.alltime%60);
            SetWindowText(GetDlgItem(MusicDialog.MUSIC_Hwnd, eID_ALL_TIME), wbuf);
            x_mbstowcs_cp936(wbuf, music_lcdlist[MusicDialog.playindex], 100);
            SetWindowText(GetDlgItem(MusicDialog.MUSIC_Hwnd, eID_MUSIC_ITEM), wbuf);    
          }          
                      
          x_wsprintf(wbuf, L"%02d:%02d",MusicDialog.curtime/60,MusicDialog.curtime%60);
          if(!MusicDialog.mList_State && !(SendMessage(MusicDialog.TIME_Hwnd, SBM_GETSTATE,0,0)&SST_THUMBTRACK))//进入列表界面，不进行更新
          {
            SendMessage(MusicDialog.TIME_Hwnd, SBM_SETVALUE, TRUE, MusicDialog.curtime*255/MusicDialog.alltime);
            //InvalidateRect(MusicDialog.TIME_Hwnd, NULL, FALSE); 
            SetWindowText(GetDlgItem(MusicDialog.MUSIC_Hwnd, eID_CUR_TIME), wbuf);    
          }            
          lrc.curtime = MusicDialog.curtime;  
          if(lrc.flag == 1)
          {
           //+100是提前显示，显示需要消耗一点时间
            if((lrc.oldtime <= lrc.curtime*100+100)&&(lrc.indexsize>7))
            {
              //显示当前行的歌词
              x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount]-1], LYRIC_MAX_SIZE);
              SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC3),wbuf);
              //显示第i-1行的歌词（前一行）
              if(lyriccount>0)
              {
                x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-1]-1], LYRIC_MAX_SIZE);
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC2),wbuf);
              }
              else
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC2),L" ");
              //显示第i-2行的歌词（前两行）
              if(lyriccount>0)
              {
                x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-2]-1], LYRIC_MAX_SIZE);
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC1),wbuf);
              }
              else
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC1),L" ");
              //显示第i+1行的歌词（后一行）   
              if(lyriccount < lrc.indexsize-1)
              {
                x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount+1]-1], LYRIC_MAX_SIZE);
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC4),wbuf);                    
              }
              else
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC4),L" ");
              //显示第i+2行的歌词（后二行）   
              if(lyriccount < lrc.indexsize-2)
              {
                x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount+2]-1], LYRIC_MAX_SIZE);
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC5),wbuf);                    
              }
              else
                SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC5),L" ");

              do{
                lyriccount++;					
                if(lyriccount>=lrc.indexsize)
                {
                  lrc.oldtime=0xffffff;
                  break;
                }
                lrc.oldtime=lrc.time_tbl[lyriccount];
              }while(lrc.oldtime<=(lrc.curtime*100));
            }
          }
          else
          {
             
             SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC3),L"请在SDCard放入相应的歌词文件(*.lrc)");
             SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC2),L" ");
             SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC1),L" ");
             SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC4),L" ");
             SetWindowText(GetDlgItem(MusicDialog.LRC_Hwnd,eID_TEXTBOX_LRC5),L" ");
          }           
          
          timecount=0;
        }         
      }
      else
      {
        uint8_t temp=0;	
           
        //根据进度条调整播放位置				
        temp=SendMessage(MusicDialog.TIME_Hwnd, SBM_GETVALUE, NULL, NULL);        
           
        //计算进度条表示的时间
        time_sum = (float)MusicDialog.alltime/255*temp*1000;  	
        //根据时间计算文件位置并跳转至该位置
        pos = ID3V2_size + (time_sum/26)*frame_size;

//        if(pos%4!=0)          //对齐
//        {
//         uint8_t num_left = 0;
//         num_left = pos % 4;
//         pos += (4-num_left);
//     
//        } 
        if(MusicDialog.Update_Content == 1)
        {
          
          MusicDialog.Update_Content = 0;
          result = f_lseek(&file,pos);
        }
        lrc.oldtime=0;
        lyriccount=0;
         
        MusicDialog.chgsch=0;     
      }  
           
		}
		Isread=0; 
    timecount++;
	}
	SAI_Play_Stop();

	mp3player.ucStatus=STA_IDLE;
	MP3FreeDecoder(Mp3Decoder);
	f_close(&file);	
}

/* DMA发送完成中断回调函数 */
/* 缓冲区内容已经播放完成，需要切换缓冲区，进行新缓冲区内容播放 
   同时读取WAV文件数据填充到已播缓冲区  */
void MusicPlayer_SAI_DMA_TX_Callback(void)
{

  if(DMA_Instance->CR&(1<<19)) //当前使用Memory1数据
  {
    bufflag=0;                       //可以将数据读取到缓冲区0
  }
  else                               //当前使用Memory0数据
  {
    bufflag=1;                       //可以将数据读取到缓冲区1
  }
  Isread=1;                          // DMA传输完成标志
	
}

/***************************** (END OF FILE) *********************************/
