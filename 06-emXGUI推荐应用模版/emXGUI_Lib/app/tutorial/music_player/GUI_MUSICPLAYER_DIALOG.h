#ifndef _GUI_RGBLED_DIALOG_H_
#define _GUI_RGBLED_DIALOG_H_

#include "emXGUI.h"
#include "Widget.h"

#define LYRIC_MAX_SIZE          200		
#define MUSIC_MAX_NUM           50
#define MUSICFILE_NAME_LEN      100	
#define _DF1S                   0x81
#define MAX_LINE_LEN            200	
#define COMDATA_SIZE				    1024*5
typedef enum
{
  eID_BUTTON_Power = 0x1000,
  eID_BUTTON_List,
  eID_BUTTON_LRC,
  eID_BUTTON_BACK,
  eID_BUTTON_NEXT,
  eID_MUSIC_TRUM, //喇叭
  
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

typedef __packed struct 
{ 
	uint8_t 	indexsize; 	
  uint8_t		length_tbl[LYRIC_MAX_SIZE]; 
 	uint16_t	addr_tbl[LYRIC_MAX_SIZE];										
 	int32_t		curtime;										
  int32_t		oldtime;	
  int32_t 	time_tbl[LYRIC_MAX_SIZE];	
  uint8_t flag;//读取文件标志位
}LYRIC;
typedef struct
{ 
  HWND MUSIC_Hwnd;                 //播放器窗口句柄
  HWND LIST_Hwnd;
  HWND LRC_Hwnd;
  HWND TIME_Hwnd;
  
  HDC hdc_bk;
  uint8_t  music_file_num;//歌曲数目
  uint16_t curtime;
  uint16_t alltime;
  int8_t  playindex;
  char* filename;
  uint16_t power;                  //音量值       
  BOOL Load_File;
  BOOL Init_State;
  BOOL mList_State;
  BOOL mLRC_State;
  BOOL Update_Content;
  uint8_t chgsch;
  int angle;
}MUSIC_DIALOG_Typedef;
extern MUSIC_DIALOG_Typedef MusicDialog;
extern LYRIC lrc;
extern char music_playlist[MUSIC_MAX_NUM][100];//播放List
extern char music_lcdlist[MUSIC_MAX_NUM][100];//显示list
extern uint8_t ReadBuffer1[1024*5];
extern ICON_Typedef music_icon[12];
void GUI_MUSIC_DIALOGTest(void *param);

#endif

