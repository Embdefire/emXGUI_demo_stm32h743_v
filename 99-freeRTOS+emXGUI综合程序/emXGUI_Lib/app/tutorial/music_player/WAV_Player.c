/*
******************************************************************************
* @file    recorder.c
* @author  fire
* @version V1.0
* @date    2015-xx-xx
* @brief   WM8978�������ܲ���+mp3����
******************************************************************************
* @attention
*
* ʵ��ƽ̨:����  STM32 F429 ������  
* ��̳    :http://www.firebbs.cn
* �Ա�    :https://fire-stm32.taobao.com
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
  * @brief   WAV��ʽ��Ƶ����������
	* @note   
  * @param  ��
  * @retval ��
  */
void wavplayer(const char *wavfile, uint8_t vol, HDC hdc)
{
	static uint8_t timecount;//��¼ʱ��
   WCHAR wbuf[128];
   static COLORREF color;
	mp3player.ucStatus=STA_IDLE;    /* ��ʼ����Ϊ����״̬  */
	Recorder.ucFmtIdx=3;           /* ȱʡ������I2S��׼��16bit���ݳ��ȣ�44K������  */
	Recorder.ucVolume=vol;          /* ȱʡ��������  */
   
   DWORD pos;//��¼���ֱ���
   static uint8_t lyriccount=0;//���index��¼   
   
   
	/*  ��ʼ��������I2S  */
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
      printf("��ǰ�����ļ� -> %s\n",wavfile);
	
      result=f_open(&file,wavfile,FA_READ);
      if(result!=FR_OK)
      {
         printf("����Ƶ�ļ�ʧ��!!!->%d\r\n",result);
         result = f_close (&file);
         Recorder.ucStatus = STA_ERR;
         return;
      }
      //��ȡWAV�ļ�ͷ
      result = f_read(&file,&rec_wav,sizeof(rec_wav),&bw);
      
      //������������б�Ͳ���ʾʱ��
      if(enter_flag == 0){
         //��ȡ��Ļ��385��404������ɫ
         color = GetPixel(hdc, 385, 404);               
        
         mp3player.ucFreq =  rec_wav.dwSamplesPerSec;
         mp3player.ucbps =  mp3player.ucFreq*32;   
         alltime=file.fsize*8/mp3player.ucbps;
        
//         x_wsprintf(wbuf, L"00:00 / %02d:%02d",alltime/60,alltime%60);
//         //���rc_MusicTimes���ε�����
///         ClrDisplay(hdc, &rc_MusicTimes, color);
//         //�����ı�
//         DrawText(hdc, wbuf, -1, &rc_MusicTimes, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      }   
      //�ȶ�ȡ��Ƶ���ݵ�������
      result = f_read(&file,(uint16_t *)buffer0,RECBUFFER_SIZE*2,&bw);
      result = f_read(&file,(uint16_t *)buffer1,RECBUFFER_SIZE*2,&bw);
      
      Delay_ms(10);	/* �ӳ�һ��ʱ�䣬�ȴ�I2S�жϽ��� */
      I2S_Stop();			/* ֹͣI2S¼���ͷ��� */
      wm8978_Reset();		/* ��λWM8978����λ״̬ */	

      mp3player.ucStatus = STA_PLAYING;		/* ����״̬ */

      /* ����WM8978оƬ������ΪDAC�����Ϊ���� */
      wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
      /* ����������������ͬ���� */
      wm8978_SetOUT1Volume(Recorder.ucVolume);
      /* ����WM8978��Ƶ�ӿ�Ϊ�����ֱ�׼I2S�ӿڣ�16bit */
      wm8978_CfgAudioIF(I2S_Standard_Phillips, 16);
      
      I2Sx_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
      I2Sxext_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
      
      I2Sxext_RX_DMA_Init(&recplaybuf[0],&recplaybuf[1],1);
      DMA_ITConfig(I2Sxext_RX_DMA_STREAM,DMA_IT_TC,DISABLE);//������������ж�
      I2Sxext_Recorde_Stop();
      
      I2Sx_TX_DMA_Init(buffer0,buffer1,RECBUFFER_SIZE);		
      I2S_Play_Start();
   }
   /* ����������ѭ���� */
   while(mp3player.ucStatus == STA_PLAYING){
   /* DMA������� */
      if(Isread==1)
      {
         Isread=0;
         //�޸Ľ�����
         if(chgsch==0) 
         {  
           if(timecount>=10)      
           { 
               curtime=file.fptr*8/mp3player.ucbps;                                        //��ȡ��ǰ���Ž���(��λ��s)
               if(enter_flag == 0){
                  //�������ʱ����ʾ�͸�����ֵ���ʾ
//                  ClrDisplay(hdc, &rc_MusicTimes, color);
//                  ClrDisplay(hdc, &rc_musicname, color);
                  //���ַ�����ת��Ϊ���ַ�����
//                   x_mbstowcs_cp936(wbuf, music_lcdlist[play_index], FILE_NAME_LEN);
//                  DrawText(hdc, wbuf, -1, &rc_musicname, DT_SINGLELINE | DT_CENTER | DT_VCENTER);//��������
                  //������ʱ���ʽ�������wbuf
//                  x_wsprintf(wbuf, L"%02d:%02d",curtime/60,curtime%60,alltime/60,alltime%60);
//                  DrawText(hdc, wbuf, -1, &rc_MusicTimes, DT_SINGLELINE | DT_CENTER | DT_VCENTER);//��������
                  //���½�����
                  
                  
                  
                  SendMessage(music_wnd_time, SBM_SETVALUE, TRUE, curtime*255/alltime);
                  InvalidateRect(MusicPlayer_hwnd, &rc_cli, FALSE);   
                  

                  lrc.curtime = curtime;  
                  if(lrc.flag == 1){
                     //+100����ǰ��ʾ����ʾ��Ҫ����һ��ʱ��
                     if((lrc.oldtime <= lrc.curtime*100+100)&&(lrc.indexsize>7))
                     {
                        //��ʾ��ǰ�еĸ��
                        x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount]-1], LYRIC_MAX_SIZE);
                        SetWindowText(wnd_lrc3,wbuf);
                        //��ʾ��i-1�еĸ�ʣ�ǰһ�У�
                        if(lyriccount>0)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-1]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc2,wbuf);
                        }
                        else
                           SetWindowText(wnd_lrc2,L" ");
                        //��ʾ��i-2�еĸ�ʣ�ǰ���У�
                        if(lyriccount>0)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount-2]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc1,wbuf);
                        }
                        else
                           SetWindowText(wnd_lrc1,L" ");
                        //��ʾ��i+1�еĸ�ʣ���һ�У�   
                        if(lyriccount < lrc.indexsize-1)
                        {
                           x_mbstowcs_cp936(wbuf, (const char *)&ReadBuffer1[lrc.addr_tbl[lyriccount+1]-1], LYRIC_MAX_SIZE);
                           SetWindowText(wnd_lrc4,wbuf);                    
                        }
                        else
                           SetWindowText(wnd_lrc4,L" ");
                        //��ʾ��i+2�еĸ�ʣ�����У�   
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
                  //�Ҳ�������ļ�
                  else
                  {
                     
                     SetWindowText(wnd_lrc3,L"����SDCard������Ӧ�ĸ���ļ�(*.lrc)");
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
         /* ������ɻ��ȡ����ֹͣ���� */
         if((result!=FR_OK)||(file.fptr==file.fsize))
         {
            //�����и�״̬
            mp3player.ucStatus=STA_SWITCH;
            //������Ŀ����1
            play_index++;
            //printf("%d, %d\n", play_index, music_file_num);
            //����Ϊ�б�ѭ������
            if(play_index >= music_file_num) play_index = 0;
            if(play_index < 0) play_index = music_file_num - 1;
            printf("��������߶�ȡ�����˳�...\r\n");
            I2S_Play_Stop();
            file.fptr=0;
            f_close(&file);
            I2S_Stop();		/* ֹͣI2S¼���ͷ��� */
            wm8978_Reset();	/* ��λWM8978����λ״̬ */							
         }		    
      }
   }
			
      mp3player.ucStatus = STA_SWITCH;		/* ����״̬ */
      file.fptr=0;
      f_close(&file);
      lrc.oldtime=0;
      lyriccount=0;      
      I2S_Stop();		/* ֹͣI2S¼���ͷ��� */
      wm8978_Reset();	/* ��λWM8978����λ״̬ */
		
	
}
/***************************** (END OF FILE) *********************************/
