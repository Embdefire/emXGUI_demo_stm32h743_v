#ifndef __GUI_PICTURE_PORT_H
#define	__GUI_PICTURE_PORT_H

#if defined(STM32F429_439xx)
#include "stm32f4xx.h"
#elif defined(STM32H743xx)
#include "stm32h7xx.h"
#endif
#include "emXGUI.h"

#ifdef	__cplusplus
extern "C"{
#endif

BOOL PIC_BMP_Draw_Res(HDC hdc, int x, int y, char *file_name, const RECT *lprc);
BOOL PIC_BMP_GetInfo_Res(BITMAPINFO *bm_info, char *file_name);
BOOL PIC_BMP_Draw_FS(HDC hdc, int x, int y, char *file_name, const RECT *lprc);
BOOL PIC_BMP_GetInfo_FS(BITMAPINFO *bm_info, char *file_name); 
BOOL PIC_Capture_Screen_To_BMP(const char *out_file);


#ifdef	__cplusplus
}
#endif

#endif /* __GUI_PICTURE_PORT_H */


