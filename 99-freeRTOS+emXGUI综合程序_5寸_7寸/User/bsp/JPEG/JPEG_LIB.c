#include "JPEG_LIB.h"
#include "x_libc.h"
struct	__jpg_dec
{
	const void	*pSrcData;
	U32		DataSize;
	//u8		*line_buf;
//	struct jpeg_decompress_struct cinfo;
	
};


extern void *GUI_VMEM_Alloc(u32 size);
extern void GUI_VMEM_Free(void *ptr);
static DMA2D_HandleTypeDef    DMA2D_Handle;

/**
  * @brief  复制解码后的数据到指定内存中.
  * @param  pSrc: 指向数据源地址的指针
  * @param  pDst: 指向数据存放目标地址的指针
  * @param  x: 目标地址的水平偏移坐标,destination Horizontal offset.
  * @param  y: 目标地址的垂直偏移坐标,destination Vertical offset.
  * @param  xsize: 图像宽度,image width
  * @param  ysize: 图像高度,image Height
  * @param  ChromaSampling :YCbCr的采样格式,YCbCr Chroma sampling : 4:2:0, 4:2:2 or 4:4:4  
  * @retval None
  */
static void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling)
{   
  
  uint32_t cssMode = DMA2D_CSS_420, inputLineOffset = 0;  
  uint32_t destination = 0; 
  
  if(ChromaSampling == JPEG_420_SUBSAMPLING)
  {
    cssMode = DMA2D_CSS_420;
    
    inputLineOffset = xsize % 16;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 16 - inputLineOffset;
    }    
  }
  else if(ChromaSampling == JPEG_444_SUBSAMPLING)
  {
    cssMode = DMA2D_NO_CSS;
    
    inputLineOffset = xsize % 8;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 8 - inputLineOffset;
    }    
  }
  else if(ChromaSampling == JPEG_422_SUBSAMPLING)
  {
    cssMode = DMA2D_CSS_422;
    
    inputLineOffset = xsize % 16;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 16 - inputLineOffset;
    }      
  }  
  
  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/ 
  DMA2D_Handle.Init.Mode         = DMA2D_M2M_PFC;
  DMA2D_Handle.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
  DMA2D_Handle.Init.OutputOffset = GUI_XSIZE - xsize; 
  DMA2D_Handle.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
  DMA2D_Handle.Init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */  
  
  /*##-2- DMA2D Callbacks Configuration ######################################*/
  DMA2D_Handle.XferCpltCallback  = NULL;
  
  /*##-3- Foreground Configuration ###########################################*/
  DMA2D_Handle.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  DMA2D_Handle.LayerCfg[1].InputAlpha = 0xFF;
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
  DMA2D_Handle.LayerCfg[1].ChromaSubSampling = cssMode;
  DMA2D_Handle.LayerCfg[1].InputOffset = inputLineOffset;
  DMA2D_Handle.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  DMA2D_Handle.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */  
  
  DMA2D_Handle.Instance          = DMA2D; 
  
  /*##-4- DMA2D Initialization     ###########################################*/
  HAL_DMA2D_Init(&DMA2D_Handle);
  HAL_DMA2D_ConfigLayer(&DMA2D_Handle, 1);
  
  /*##-5-  copy the new decoded frame to the LCD Frame buffer ################*/
  destination = (uint32_t)pDst + ((y * GUI_XSIZE) + x) * 4;

  HAL_DMA2D_Start(&DMA2D_Handle, (uint32_t)pSrc, destination, xsize, ysize);
  HAL_DMA2D_PollForTransfer(&DMA2D_Handle, 25);  /* wait for the previous DMA2D transfer to ends */
}

/**
  * @brief  将图像数据写入到绘图上下文HDC中,为XGUI提供接口.
  * @param  hdc: 绘图上下文形参
  * @param  p: 指向数据源的指针(存放显示内容的首地址)
  * @param  x: 目标地址的水平偏移坐标,destination Horizontal offset.
  * @param  y: 目标地址的垂直偏移坐标,destination Vertical offset.
  * @param  xsize: 图像宽度,以参考背景为基准,显示一行内容的字节,如以800*480图片为背景,
                   RGB565则为800*2
  * @retval None
  */

static void _Write_AllPixels_RGB(HDC hdc,const u8 *p, int x, int y, int xSize)
{
	BITMAP bm;
	bm.Format	 =BM_RGB565;
	bm.Width	 =JPEG_Info.ImageWidth;
	bm.Height	 =JPEG_Info.ImageHeight;
	bm.WidthBytes =GUI_XSIZE*2;
	bm.Bits	    =(u8*)p;
	bm.LUT		=NULL;
	DrawBitmap(hdc,x,y,&bm,NULL);
}

FIL JPEG_File;
uint16_t curri=0;
extern heap_t *heap_maneger;
static	BOOL _Draw(HDC hdc, int x, int y,JPG_DEC *dec)
{
			uint32_t JpegProcessing_End = 0;
			HAL_JPEG_DeInit(&JPEG_Handle);
      JPEG_Handle.Instance = JPEG;
	
      HAL_JPEG_Init(&JPEG_Handle);
      GUI_DEBUG("%d ---- BEFORE HEAP SIZE : %d k \r\n",curri,(heap_maneger->used_cur)/1024);
      uint32_t * Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(Decode_Buffer_Size);
      
			
		  JPEG_Decode_DMA(&JPEG_Handle,(uint32_t)Decode_Buffer);
			do{
          JpegProcessing_End = JPEG_InputHandler(&JPEG_Handle);            
        }while(JpegProcessing_End == 0);
			
			Jpeg_HWDecodingEnd = 0;//解码完成,恢复结束标志为初始状态
			
			HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
			
			uint32_t * Output_Buffer = (uint32_t *)GUI_VMEM_Alloc(JPEG_Info.ImageWidth * JPEG_Info.ImageHeight * 3 );
#if JPEGDEBUG
			GUI_DEBUG("ALLOC SIZE : %d",JPEG_Info.ImageWidth * JPEG_Info.ImageHeight * 3 );
#endif
			if(Output_Buffer != NULL)
			{
				DMA2D_CopyBuffer((uint32_t *)Decode_Buffer, (uint32_t *)Output_Buffer, x , y, JPEG_Info.ImageWidth, JPEG_Info.ImageHeight, JPEG_Info.ChromaSubsampling);
			}
			else
			{
				return 0;
			}
			curri++;
			GUI_DEBUG("%d ---- NOW HEAP SIZE : %d k \r\n",curri,(heap_maneger->used_cur)/1024);
			_Write_AllPixels_RGB(hdc, (uint8_t *)Output_Buffer , 0 , 0 ,JPEG_Info.ImageWidth);

//			memset(Decode_Buffer,0,Decode_Buffer_Size);
//			memset(Output_Buffer,0,JPEG_Info.ImageWidth * JPEG_Info.ImageHeight * 2 + 100);
      GUI_VMEM_Free(Decode_Buffer);
			GUI_DEBUG("%d ---- GUI_VMEM_Free Decode_Buffer : %d k \r\n",curri,(heap_maneger->used_cur)/1024);
      GUI_VMEM_Free(Output_Buffer);
			GUI_DEBUG("%d ---- GUI_VMEM_Free Output_Buffer : %d k \r\n\r\n",curri,(heap_maneger->used_cur)/1024);
#if JPEGDEBUG
	    GUI_DEBUG("FREE SIZE : %d %d",JPEG_Info.ImageWidth * JPEG_Info.ImageHeight * 3, JPEG_Info.ChromaSubsampling);
#endif
			return 1;
}

BOOL	JPG_Draw(HDC hdc,int x,int y,JPG_DEC *jdec)
{
//	if(jdec == NULL)
//	{
//		return FALSE;
//	}

	_Draw(hdc,x,y,jdec);
	return	TRUE;
}

JPG_DEC	*Decode_dec;

JPG_DEC*	JPG_Open(const void *dat,int cbSize)
{
	JPG_DEC	*dec;
	
	dec	=GUI_VMEM_Alloc(sizeof(JPG_DEC));
  Decode_dec=GUI_VMEM_Alloc(sizeof(JPG_DEC));
	if(dec!=NULL)
	{
		dec->pSrcData =dat;
		dec->DataSize =cbSize;
	}
  if(Decode_dec!=NULL)
	{
		Decode_dec->pSrcData =dat;
		Decode_dec->DataSize =cbSize;
	}
	return	dec;
}

BOOL JPG_GetImageSize(U16 *width,U16 *height,JPG_DEC* jdec)
{
	if(jdec==NULL)
	{
	  return FALSE;
	}	
	
	uint32_t JpegProcessing_End = 0;
			
  JPEG_Handle.Instance = JPEG;
	
  HAL_JPEG_Init(&JPEG_Handle);
       
  uint32_t * Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(Decode_Buffer_Size);

			
	JPEG_Decode_DMA(&JPEG_Handle,(uint32_t)Decode_Buffer);
	
	do{
			JpegProcessing_End = JPEG_InputHandler(&JPEG_Handle);            
		}while(JpegProcessing_End == 0);
	
	Jpeg_HWDecodingEnd = 0;//解码完成,恢复结束标志为初始状态
		
	memset(Decode_Buffer,0,Decode_Buffer_Size);

	GUI_VMEM_Free(Decode_Buffer);
	
	HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
	/* 恢复指针位置和大小 */
	Decode_dec->pSrcData =jdec->pSrcData;
	Decode_dec->DataSize =jdec->DataSize;
	if(width!=NULL)
	{
		*width =JPEG_Info.ImageWidth;
	}
	if(height!=NULL)
	{
		*height =JPEG_Info.ImageHeight;
	}
	return TRUE;
}


/**
  * @brief  从文件系统读出的图像数据中,拷贝数据
  * @param 	Buffer_Dest       : 拷贝到哪里
  * @param  Byte_Want_Read    : 想读取几个字节
  * @param  Byte_Real_Read    : 实际读取到的字节(因为可能到达文件尾部)
  * @retval True of False
  */
FRESULT Read_Buf_From_File(void* Buffer_Dest, uint32_t Byte_Want_Read , uint32_t* Byte_Real_Read)
{
  if( (Buffer_Dest != NULL) && (Decode_dec->pSrcData != NULL) && (Decode_dec->DataSize >0))
  {
    if(Decode_dec->DataSize >= Byte_Want_Read)
    {
      /* 如果数据的大小>想要读的数据,直接拷贝,并把数据源地址指针偏移读取的字节数,数据大小减去读取字节的数量 */
			memcpy((uint8_t *)Buffer_Dest,(uint8_t *)Decode_dec->pSrcData,Byte_Want_Read);
      *Byte_Real_Read = Byte_Want_Read;
			(uint8_t *)Decode_dec->pSrcData += Byte_Want_Read;
      Decode_dec->DataSize -= Byte_Want_Read;
#if JPEGDEBUG
			GUI_DEBUG("remain data is %d",Decode_dec->DataSize);
#endif
      return FR_OK;
    }
    else
    {
      /* 如果数据的大小<想要读的数据,拷贝剩余的数据大小Decode_dec->DataSize,并把数据源地址指针偏移读取的字节数,数据置零 */
      memcpy((uint8_t *)Buffer_Dest,(uint8_t *)Decode_dec->pSrcData,Decode_dec->DataSize);
			*Byte_Real_Read = Decode_dec->DataSize;
      Decode_dec->DataSize = 0;
#if JPEGDEBUG
			GUI_DEBUG("remain data is %d",Decode_dec->DataSize);
#endif
      return FR_OK;
    }
  }
  else
  {
    return FR_DISK_ERR;
  }
}

void	JPG_Close(JPG_DEC *jdec)
{
	if(jdec!=NULL)
	{	
		GUI_VMEM_Free(jdec);
	}
	if(Decode_dec!=NULL)
	{	
		GUI_VMEM_Free(Decode_dec);
	}

}

void JPEG_Out(HDC hdc,int x,int y,u8 *mjpegbuffer,s32 size)
{
	Decode_dec->pSrcData =mjpegbuffer;
	Decode_dec->DataSize =size;
	JPG_Draw(hdc,x,y,Decode_dec);
}

