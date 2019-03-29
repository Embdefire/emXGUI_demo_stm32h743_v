#ifndef __GUI_FONT_PORT_H
#define	__GUI_FONT_PORT_H

#ifdef __cplusplus
extern "C" {
#endif 
  
#include	"emXGUI.h"
#include	"gui_drv_cfg.h"
  
  /** 
  * @brief  字体参数
  *  为适应不同分辨率的屏幕，使用不同的默认字体 
  */   
typedef struct
{
  const char *default_en;  //默认英文字体
  const char *default_extern_cn;  //默认中文字体
  
  const char *default_extern_logo50;  //LOGO字体
  const char *default_extern_logo100;  //LOGO字体
  const char *default_extern_logo252;  //LOGO字体
  
  const char *default_extern_ctrl32;  //控制字体（32）
  const char *default_extern_ctrl48;  //控制字体（48）  
  const char *default_extern_ctrl64;  //控制字体（64） 
  const char *default_extern_ctrl72;  //控制字体（72）  
  const char *default_extern_ctrl100;  //控制字体（100）  
}FONT_PARAM_TypeDef;

/* 字体参数配置 */
extern const FONT_PARAM_TypeDef gui_font_param[];

/* 默认字体 */
extern HFONT defaultFont;
extern HFONT logoFont50;
extern HFONT logoFont100;
extern HFONT logoFont252;

extern HFONT ctrlFont32;
extern HFONT ctrlFont48;
extern HFONT ctrlFont64;
extern HFONT ctrlFont72;
extern HFONT ctrlFont100;
/* 默认英文字体 */
extern HFONT defaultFontEn;
  
/* 用于标记是否有资源文件无法找到 */
extern BOOL res_not_found_flag;

HFONT GUI_Default_FontInit(void);
HFONT GUI_Init_Extern_Font_Stream(const char* res_name);
HFONT GUI_Init_Extern_Font_2RAM(const char* res_name,u8** buf);
HFONT GUI_Init_Extern_Font(void);
  
#ifdef	__cplusplus
}
#endif

#endif /* __GUI_FONT_PORT_H */
