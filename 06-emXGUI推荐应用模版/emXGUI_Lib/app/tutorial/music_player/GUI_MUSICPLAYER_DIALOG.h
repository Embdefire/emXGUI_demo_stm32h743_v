#ifndef _GUI_RGBLED_DIALOG_H_
#define _GUI_RGBLED_DIALOG_H_


#define LCD_MID_LINE    (4)

#define MUSIC_MAX_NUM           50
#define MUSICFILE_NAME_LEN      100	
#define _DF1S                   0x81

typedef enum
{
  eID_BUTTON_Power = 0x1000,
  eID_BUTTON_List,
  eID_BUTTON_LRC,
  eID_BUTTON_BACK,
  eID_BUTTON_NEXT,
  eID_BUTTON_START,
  eID_SCROLLBAR_TIMER,
  eID_SCROLLBAR_POWER,
  eID_MUSIC_ITEM,
  eID_CUR_TIME,
  eID_ALL_TIME,
  eID_MUSIC_EXIT,
  eID_MUSICLIST,
  eMUSIC_VIEWER_ID_PREV,
  eMUSIC_VIEWER_ID_NEXT,
  eID_MUSIC_RETURN,
  eID_TEXTBOX_LRC1,
  eID_TEXTBOX_LRC2,
  eID_TEXTBOX_LRC3,
  eID_TEXTBOX_LRC4,
  eID_TEXTBOX_LRC5,
}Cdlg_Master_ID;

typedef struct
{ 
  HWND MUSIC_Hwnd;                 //播放器窗口句柄
  HWND LIST_Hwnd;
  HDC hdc_bk;
  uint8_t  music_file_num;//歌曲数目
  char* filename;
  uint16_t power;                  //音量值       
  BOOL Load_File;
  BOOL Init_State;
}MUSIC_DIALOG_Typedef;

void GUI_MUSIC_DIALOGTest(void *param);

#endif

