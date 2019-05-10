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
* 实验平台:秉火  STM32 F429 开发板  
* 论坛    :http://www.firebbs.cn
* 淘宝    :https://fire-stm32.taobao.com
*
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "Bsp/usart/bsp_debug_usart.h"
//#include "Bsp/systick/bsp_SysTick.h"
#include "Bsp/wm8978/bsp_wm8978.h"
#include "ff.h" 
#include "./mp3_player/Backend_mp3Player.h"
#include "mp3dec.h"
#include "./mp3_player/Backend_musiclist.h"
#include "emXGUI.h"
#include "x_libc.h"
#include "./mp3_player/GUI_MUSICPLAYER_DIALOG.h"

#define Delay_ms GUI_msleep
/**
  * @brief   WAV格式音频播放主程序
	* @note   
  * @param  无
  * @retval 无
  */
void wavplayer(const char *wavfile, uint8_t vol, HDC hdc)
{
	static uint8_t timecount;//记录时间
   WCHAR wbuf[128];
   static COLORREF color;
	mp3player.ucStatus=STA_IDLE;    /* 开始设置为空闲状态  */
	Recorder.ucFmtIdx=3;           /* 缺省飞利浦I2S标准，16bit数据长度，44K采样率  */
	Recorder.ucVolume=vol;          /* 缺省耳机音量  */
   
   DWORD pos;//记录文字变量
   static uint8_t lyriccount=0;//歌词index记录   
   
   
	/*  初始化并配置I2S  */
	I2S_Stop();
	I2S_GPIO_Config();
	I2Sx_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
	I2Sxext_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
	
	I2S_DMA_TX_Callback=MP3Player_I2S_DMA_TX_Callback;
	I2S_Play_Stop();
	
	bufflag=0;
	Isread=0;
   if(mp3player.ucStatus == STA_IDLE)
   {						
      printf("当前播放文件 -> %s\n",wavfile);
	
      result=f_open(&file,wavfile,FA_READ);
      if(result!=FR_OK)
      {
         printf("打开音频文件失败!!!->%d\r\n",result);
         result = f_close (&file);
         Recorder.ucStatus = STA_ERR;
         return;
      }
      //读取WAV文件头
      result = f_read(&file,&rec_wav,sizeof(rec_wav),&bw);
      
      //如果进入音乐列表就不显示时长
      if(enter_flag == 0){
         //获取屏幕（385，404）的颜色
         color = GetPixel(hdc, 385, 404);               
        
         mp3player.ucFreq =  rec_wav.dwSamplesPerSec;
         mp3player.ucbps =  mp3player.ucFreq*32;   
         alltime=file.fsize*8/mp3player.ucbps;
        
//         x_wsprintf(wbuf, L"00:00 / %02d:%02d",alltime/60,alltime%60);
//         //清除rc_MusicTimes矩形的内容
///         ClrDisplay(hdc, &rc_MusicTimes, color);
//         //绘制文本
//         DrawText(hdc, wbuf, -1, &rc_MusicTimes, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      }   
      //先读取音频数据到缓冲区
      result = f_read(&file,(uint16_t *)buffer0,RECBUFFER_SIZE*2,&bw);
      result = f_read(&file,(uint16_t *)buffer1,RECBUFFER_SIZE*2,&bw);
      
      Delay_ms(10);	/* 延迟一段时间，等待I2S中断结束 */
      I2S_Stop();			/* 停止I2S录音和放音 */
      wm8978_Reset();		/* 复位WM8978到复位状态 */	

      mp3player.ucStatus = STA_PLAYING;		/* 放音状态 */

      /* 配置WM8978芯片，输入为DAC，输出为耳机 */
      wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
      /* 调节音量，左右相同音量 */
      wm8978_SetOUT1Volume(Recorder.ucVolume);
      /* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
      wm8978_CfgAudioIF(I2S_Standard_Phillips, 16);
      
      I2Sx_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
      I2Sxext_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
      
      I2Sxext_RX_DMA_Init(&recplaybuf[0],&recplaybuf[1],1);
      DMA_ITConfig(I2Sxext_RX_DMA_STREAM,DMA_IT_TC,DISABLE);//开启传输完成中断
      I2Sxext_Recorde_Stop();
      
      I2Sx_TX_DMA_Init(buffer0,buffer1,RECBUFFER_SIZE);		
      I2S_Play_Start();
   }
   /* 进入主程序循环体 */
   while(mp3player.ucStatus == STA_PLAYING){
   /* DMA传输完成 */
      if(Isread==1)
      {
         Isread=0;
         //修改进度条
         if(chgsch==0) 
         {  
           if(timecount>=10)      
           { 
               curtime=file.fptr*8/mp3player.ucbps;                                        //获取当前播放进度(单位：s)
               if(enter_flag == 0){
                  //清除歌曲时间显示和歌词名字的显示
//                  ClrDisplay(hdc, &rc_MusicTimes, color);
//                  ClrDisplay(hdc, &rc_musicname, color);
                  //将字符数组转换为宽字符类型
//                   x_mbstowcs_cp936(wbuf, music_lcdlist[play_index], FILE_NAME_LEN);
//                  DrawText(hdc, wbuf, -1, &rc_musicname, DT_SINGLELINE | DT_CENTER | DT_VCENTER);//绘制文字
                  //将歌曲时间格式化输出到wbuf
//                  x_wsprintf(wbuf, L"%02d:%02d",curtime/60,curtime%60,alltime/60,alltime%60);
//                  DrawText(hdc, wbuf, -1, &rc_MusicTimes, DT_SINGLELINE | DT_CENTER | DT_VCENTER);//绘制文字
                  //更新进度条
                  
                  
                  
                  SendMessage(music_wnd_time, SBM_SETVALUE, TRUE, curtime*255/alltime);
                  InvalidateRect(MusicPlayer_hwnd, &rc_cli, FALSE);   
                  

                  lrc.curtime = curtime;  
                  if(lrc.flag == 1){
                     //+100是提前显示，显示需要消耗一点时间
                     if((lrc.oldtime <= lrc.curtime*100+100)&&(lrc.indexsize>7))
                     {
                        //显示当前行的歌词
                        x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount]-1], LYRIC_MAX_SIZE);
                        SetWindowText(wnd_lrc3,wbuf);
                        //显示第i-1行的歌词（前一行）
                        if(lyriccount>0)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-1]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc2,wbuf);
                        }
                        else
                           SetWindowText(wnd_lrc2,L" ");
                        //显示第i-2行的歌词（前两行）
                        if(lyriccount>0)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-2]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc1,wbuf);
                        }
                        else
                           SetWindowText(wnd_lrc1,L" ");
                        //显示第i+1行的歌词（后一行）   
                        if(lyriccount < lrc.indexsize-1)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount+1]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc4,wbuf);                    
                        }
                        else
                           SetWindowText(wnd_lrc4,L" ");
                        //显示第i+2行的歌词（后二行）   
                        if(lyriccount < lrc.indexsize-2)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount+2]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc5,wbuf);                    
                        }
                        else
                           SetWindowText(wnd_lrc5,L" ");
                                  
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
                  //找不到歌词文件
                  else
                  {
                     
                     SetWindowText(wnd_lrc3,L"请在SDCard放入相应的歌词文件(*.lrc)");
                     SetWindowText(wnd_lrc1,L" ");
                     SetWindowText(wnd_lrc2,L" ");
                     SetWindowText(wnd_lrc4,L" ");
                     SetWindowText(wnd_lrc5,L" ");
                  }                  
               }   
               
               timecount=0;  
           }                              
         } 
         else
         {
           uint8_t temp=0;
          
           temp=SendMessage(music_wnd_time, SBM_GETVALUE, NULL, NULL);  
           pos=file.fsize/255*temp;
           if(pos<sizeof(WavHead))pos=sizeof(WavHead);
           if(rec_wav.wBitsPerSample==24)temp=12;
           else temp=8;
           if((pos-sizeof(WavHead))%temp)
           {
             pos+=temp-(pos-sizeof(WavHead))%temp;
           }        
           f_lseek(&file,pos);
           lrc.oldtime=0;
           lyriccount=0;
           chgsch=0;         
         }
         timecount++;
         if(bufflag==0)
            result = f_read(&file,buffer0,RECBUFFER_SIZE*2,&bw);	
         else
            result = f_read(&file,buffer1,RECBUFFER_SIZE*2,&bw);
         /* 播放完成或读取出错停止工作 */
         if((result!=FR_OK)||(file.fptr==file.fsize))
         {
            //进入切歌状态
            mp3player.ucStatus=STA_SWITCH;
            //播放曲目自增1
            play_index++;
            //printf("%d, %d\n", play_index, music_file_num);
            //设置为列表循环播放
            if(play_index >= music_file_num) play_index = 0;
            if(play_index < 0) play_index = music_file_num - 1;
            printf("播放完或者读取出错退出...\r\n");
            I2S_Play_Stop();
            file.fptr=0;
            f_close(&file);
            I2S_Stop();		/* 停止I2S录音和放音 */
            wm8978_Reset();	/* 复位WM8978到复位状态 */							
         }		    
      }
   }
			
      mp3player.ucStatus = STA_SWITCH;		/* 待机状态 */
      file.fptr=0;
      f_close(&file);
      lrc.oldtime=0;
      lyriccount=0;      
      I2S_Stop();		/* 停止I2S录音和放音 */
      wm8978_Reset();	/* 复位WM8978到复位状态 */
		
	
}
/***************************** (END OF FILE) *********************************/
