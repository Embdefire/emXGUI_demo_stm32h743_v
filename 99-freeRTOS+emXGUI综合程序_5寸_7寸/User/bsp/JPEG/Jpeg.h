#ifndef __JEPG_H
#define __JEPG_H

#include "stm32h7xx.h"
#include "emXGUI.h"
#include "string.h"

extern JPEG_HandleTypeDef    JPEG_Handle;
extern JPEG_ConfTypeDef      JPEG_Info;

extern uint32_t *Decode_Buffer;
extern uint32_t *OUT_Buffer; 
 
typedef	struct	__jpg_dec JPG_DEC;

extern __IO uint8_t Jpeg_HWDecodingEnd;

extern void JPEG_picture(uint32_t *databuf);
extern void PIC_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData);
extern void PIC_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength);

extern void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling);

extern void _Write_AllPixels_RGB(HDC hdc,const u8 *p, int x, int y, int xSize);

JPG_DEC*	JPG_Open(const void *dat,int cbSize);
BOOL 	JPG_GetImageSize(U16 *width,U16 *height,JPG_DEC* jdec);
BOOL	JPG_Draw(HDC hdc,int x,int y,JPG_DEC *jdec);
void	JPG_Close(JPG_DEC *jdec);
void JPEG_Out(HDC hdc,int x,int y,u8 *mjpegbuffer,s32 size);
FRESULT Read_Buf_From_File(void* Buffer_Dest, uint32_t Byte_Want_Read , uint32_t* Byte_Real_Read);
#endif
