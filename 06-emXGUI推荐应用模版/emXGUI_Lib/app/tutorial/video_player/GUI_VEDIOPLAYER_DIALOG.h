#ifndef _GUI_VIDEOPLAYER_DIALOG_H_
#define _GUI_VIDEOPLAYER_DIALOG_H_


#define GUI_RGB_BACKGROUNG_PIC      "avi_desktop.jpg"

typedef enum
{
  eID_Vedio_Power = 0x1000,
  eID_Vedio_List,
  eID_BUTTON_LRC,
  eID_Vedio_BACK,
  eID_Vedio_NEXT,
  eID_Vedio_TRUM, //À®°È
  eID_Vedio_START,
  
  eID_SBN_TIMER,
  eID_SBN_POWER,
  
                  
  eID_FileList,
  
  eID_VIDEO_EXIT,
  eID_MUSICLIST,
  
  eMUSIC_VIEWER_ID_PREV,
  eMUSIC_VIEWER_ID_NEXT,
  eID_VIDEO_RETURN,
  
  eID_TEXTBOX_ITEM,
  eID_TEXTBOX_RES,
  eID_TEXTBOX_FPS,
  eID_TEXTBOX_CURTIME,
  eID_TEXTBOX_ALLTIME,
  eID_TEXTBOX_LRC5,
  
  
}VideoDlg_Master_ID;

typedef struct
{
  HWND Video_Hwnd;
  HWND List_Hwnd;
  HWND SBN_TIMER_Hwnd;
  HDC hdc_bk;
  
  int power;  
  uint16_t curtime;
  uint16_t alltime;
  
  uint8_t avi_file_num;
  int8_t  playindex;
  BOOL LOAD_STATE;
  
}VIDEO_DIALOG_Typedef;
extern VIDEO_DIALOG_Typedef VideoDialog;
#endif


