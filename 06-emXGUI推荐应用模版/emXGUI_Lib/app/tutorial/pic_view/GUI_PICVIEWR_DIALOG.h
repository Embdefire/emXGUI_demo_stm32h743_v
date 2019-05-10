#ifndef _GUI_PICVIEW_H_
#define _GUI_PICVIEW_H_


#define PICFILE_NUM_MAX           20
#define PICFILE_NAME_MAXLEN       100	


typedef enum 
{
  eID_Pic_Name = 0x1000,
  eID_Pic_EXIT,
//  eID_Pic_PREV,
//  eID_Pic_NEXT,
//  eID_Pic_MsgBOX,
//  eID_Pic_Time,
//  eID_Pic_Res,
//  eID_Pic_Res_Value,
//  //eID_Pic_Time,
//  eID_Pic_Time_Value,
//  eID_Pic_INTFLASH,
//  eID_Pic_EXTFLASH,
//  eID_Pic_SDCARD,
//  eID_Pic_Return,
//  eID_Pic_JPG,
//  eID_Pic_PNG,
//  eID_Pic_GIF,
//  eID_Pic_BMP,
//  eID_Pic_Title,
  eID_Pic_Def,
//  eID_FILEPATH,
//  eID_ZOOMIN,
//  eID_ZOOMOUT,
//  eID_Pic_FPS,
//  eID_Pic_FPS_Value,
//  eID_LIST_1,
}GUI_PicViewer_ID;

typedef struct 
{
  HWND PicView_Handle;
  
}PicViewer_Dialog_Typedef;
#endif /* _CDLG_PICVIEW_H_ */

