#ifndef _JPEG_LIB_H
#define _JPEG_LIB_H

#include "decode_dma.h"
#include "emXGUI.h"
#include "string.h"
#include "emXGUI_JPEG.h"
#define Decode_Buffer_Size	(3*512*1024)
//#define Output_Buffer_Size	(1*512*1024)

extern FIL JPEG_File;
typedef	struct	__jpg_dec JPG_DEC;

JPG_DEC*	JPG_Open(const void *dat,int cbSize);
BOOL 	JPG_GetImageSize(U16 *width,U16 *height,JPG_DEC* jdec);
BOOL	JPG_Draw(HDC hdc,int x,int y,JPG_DEC *jdec);
void	JPG_Close(JPG_DEC *jdec);
void JPEG_Out(HDC hdc,int x,int y,u8 *mjpegbuffer,s32 size);
	
FRESULT Read_Buf_From_File(void* Buffer_Dest, uint32_t Byte_Want_Read , uint32_t* Byte_Real_Read);

static void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling);

#endif
