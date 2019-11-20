#ifndef __DMA2D_EMXGUI_H__
#define __DMA2D_EMXGUI_H__

#ifdef	__cplusplus
extern "C"{
#endif

#include "def.h"

//#include "BSP.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_dma2d.h"

#include "emXGUI.h"

/*=========================================================================================*/

void DMA2D_DrvInit(void);
BOOL DMA2D_DrawBitmap_RGB565(const SURFACE *pSurf,int x,int y,U16 w,U16 h,int width_bytes,const U8 *bits);
BOOL DMA2D_DrawBitmap_ARGB(const SURFACE *pSurf,int x,int y,U16 w,U16 h,int width_bytes,const U8 *bits,U32 color_format);

/*=========================================================================================*/

#ifdef	__cplusplus
}
#endif

#endif /*__DMA2D_EMXGUI_H__*/














