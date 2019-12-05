#include "Jepg.h"


#define CHUNK_SIZE_IN  ((uint32_t)(64 * 1024)) 
#define CHUNK_SIZE_OUT ((uint32_t)(64 * 1024))

#define JPEG_BUFFER_EMPTY 0
#define JPEG_BUFFER_FULL  1

 uint32_t *Decode_Buffer;
 uint32_t *OUT_Buffer;
JPG_DEC	*Decode_dec;

struct	__jpg_dec
{
	const void	*pSrcData;
	U32		DataSize;
	//u8		*line_buf;
//	struct jpeg_decompress_struct cinfo;
	
};

static DMA2D_HandleTypeDef    DMA2D_Handle;

typedef struct
{
  uint8_t State;  
  uint8_t *DataBuffer;
  uint32_t DataBufferSize;

}JPEG_Data_BufferTypeDef;

JPEG_Data_BufferTypeDef Jpeg_IN_BufferTab;

uint8_t PIC_JPEG_FLAG;
uint8_t Input_Is_Paused = 0;
uint32_t FrameBufferAddress;


void JPEG_Decode_Mem(void)
{	
	Jpeg_IN_BufferTab.State = JPEG_BUFFER_EMPTY;
	Jpeg_IN_BufferTab.DataBuffer = GUI_VMEM_Alloc(CHUNK_SIZE_IN);
	Jpeg_IN_BufferTab.DataBufferSize = 0;	
}

void JPEG_Decode_free(void)
{
	GUI_VMEM_Free(Jpeg_IN_BufferTab.DataBuffer);
}

void JPEG_picture(uint32_t *databuf)
{ 
	
	PIC_JPEG_FLAG = 1;
	Input_Is_Paused = 0;
	Jpeg_HWDecodingEnd = 0;
	FrameBufferAddress = (uint32_t)databuf;
	
	if(Decode_dec->pSrcData != NULL)
	{	
		if(Read_Buf_From_File( Jpeg_IN_BufferTab.DataBuffer , CHUNK_SIZE_IN, (UINT*)(&Jpeg_IN_BufferTab.DataBufferSize)) == FR_OK)
		{
			Jpeg_IN_BufferTab.State = JPEG_BUFFER_FULL;
		}   
		
		HAL_JPEG_Decode_DMA(&JPEG_Handle ,Jpeg_IN_BufferTab.DataBuffer ,Jpeg_IN_BufferTab.DataBufferSize ,(uint8_t *)FrameBufferAddress ,CHUNK_SIZE_OUT);
		
		while(Jpeg_HWDecodingEnd == 0)
		{
			if(Jpeg_IN_BufferTab.State == JPEG_BUFFER_EMPTY)
			{
				if(Read_Buf_From_File (Jpeg_IN_BufferTab.DataBuffer , CHUNK_SIZE_IN, (UINT*)(&Jpeg_IN_BufferTab.DataBufferSize)) == FR_OK)
				{  
					Jpeg_IN_BufferTab.State = JPEG_BUFFER_FULL;
				}

				if(Input_Is_Paused == 1)
				{
					Input_Is_Paused = 0;
					HAL_JPEG_ConfigInputBuffer(&JPEG_Handle,Jpeg_IN_BufferTab.DataBuffer, Jpeg_IN_BufferTab.DataBufferSize);    

					HAL_JPEG_Resume(&JPEG_Handle, JPEG_PAUSE_RESUME_INPUT); 
				}			
			}
		}
		
		HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);  

}
	
	PIC_JPEG_FLAG = 0;
}
	

/**
  * @brief  JPEG Get Data callback
  * @param hjpeg: JPEG handle pointer
  * @param NbDecodedData: Number of decoded (consummed) bytes from input buffer
  * @retval None
  */
void PIC_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	if(NbDecodedData == Jpeg_IN_BufferTab.DataBufferSize)
	{  
		Jpeg_IN_BufferTab.State = JPEG_BUFFER_EMPTY;
		Jpeg_IN_BufferTab.DataBufferSize = 0;

		HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
		Input_Is_Paused = 1;
	}
	else
	{
		HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab.DataBuffer + NbDecodedData, Jpeg_IN_BufferTab.DataBufferSize - NbDecodedData);      
	} 
}

/**
  * @brief  JPEG Data Ready callback
  * @param hjpeg: JPEG handle pointer
  * @param pDataOut: pointer to the output data buffer
  * @param OutDataLength: length of output buffer in bytes
  * @retval None
  */
void PIC_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	FrameBufferAddress += OutDataLength;
	HAL_JPEG_ConfigOutputBuffer(hjpeg, (uint8_t *)FrameBufferAddress, CHUNK_SIZE_OUT); 
}


/* Private typedef -----------------------------------------------------------*/
static MDMA_HandleTypeDef   hmdmaIn;
static MDMA_HandleTypeDef   hmdmaOut; 

JPEG_HandleTypeDef    JPEG_Handle;
JPEG_ConfTypeDef      JPEG_Info;

__IO uint8_t Jpeg_HWDecodingEnd = 0;
/**
  * @brief  JPEG Get Data callback
  * @param hjpeg: JPEG handle pointer
  * @param NbDecodedData: Number of decoded (consummed) bytes from input buffer
  * @retval None
  */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	if(PIC_JPEG_FLAG == 1)
	{
		PIC_JPEG_GetDataCallback(hjpeg,NbDecodedData);
	}
}

/**
  * @brief  JPEG Data Ready callback
  * @param hjpeg: JPEG handle pointer
  * @param pDataOut: pointer to the output data buffer
  * @param OutDataLength: length of output buffer in bytes
  * @retval None
  */
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	if(PIC_JPEG_FLAG == 1)
	{
		PIC_JPEG_DataReadyCallback(hjpeg,pDataOut,OutDataLength);
	}
}

/**
  * @brief  JPEG Decode complete callback
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{    
	Jpeg_HWDecodingEnd = 1;
}

void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
	__HAL_RCC_JPEG_CLK_ENABLE();
	
	/* Enable JPEG clock */
	__HAL_RCC_JPGDECEN_CLK_ENABLE();

	/* Enable MDMA clock */
	__HAL_RCC_MDMA_CLK_ENABLE();

	HAL_NVIC_SetPriority(JPEG_IRQn, 0x07, 0);
	HAL_NVIC_EnableIRQ(JPEG_IRQn);  

	/* Input MDMA */
	/* Set the parameters to be configured */ 
	hmdmaIn.Init.Priority           = MDMA_PRIORITY_HIGH;
	hmdmaIn.Init.Endianness         = MDMA_LITTLE_ENDIANNESS_PRESERVE;
	hmdmaIn.Init.SourceInc          = MDMA_SRC_INC_BYTE;
	hmdmaIn.Init.DestinationInc     = MDMA_DEST_INC_DISABLE;
	hmdmaIn.Init.SourceDataSize     = MDMA_SRC_DATASIZE_BYTE;
	hmdmaIn.Init.DestDataSize       = MDMA_DEST_DATASIZE_WORD;
	hmdmaIn.Init.DataAlignment      = MDMA_DATAALIGN_PACKENABLE;   
	hmdmaIn.Init.SourceBurst        = MDMA_SOURCE_BURST_32BEATS;
	hmdmaIn.Init.DestBurst          = MDMA_DEST_BURST_16BEATS; 
	hmdmaIn.Init.SourceBlockAddressOffset = 0;
	hmdmaIn.Init.DestBlockAddressOffset  = 0;

	/*Using JPEG Input FIFO Threshold as a trigger for the MDMA*/
	hmdmaIn.Init.Request = MDMA_REQUEST_JPEG_INFIFO_TH; /* Set the MDMA HW trigger to JPEG Input FIFO Threshold flag*/  
	hmdmaIn.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;  
	hmdmaIn.Init.BufferTransferLength = 32; /*Set the MDMA buffer size to the JPEG FIFO threshold size i.e 32 bytes (8 words)*/

	hmdmaIn.Instance = MDMA_Channel7;

	/* Associate the DMA handle */
	__HAL_LINKDMA(hjpeg, hdmain, hmdmaIn);

	/* DeInitialize the DMA Stream */
	HAL_MDMA_DeInit(&hmdmaIn);  
	/* Initialize the DMA stream */
	HAL_MDMA_Init(&hmdmaIn);

	/* Output MDMA */
	/* Set the parameters to be configured */ 
	hmdmaOut.Init.Priority        = MDMA_PRIORITY_VERY_HIGH;
	hmdmaOut.Init.Endianness      = MDMA_LITTLE_ENDIANNESS_PRESERVE;
	hmdmaOut.Init.SourceInc       = MDMA_SRC_INC_DISABLE;
	hmdmaOut.Init.DestinationInc  = MDMA_DEST_INC_BYTE;
	hmdmaOut.Init.SourceDataSize  = MDMA_SRC_DATASIZE_WORD;
	hmdmaOut.Init.DestDataSize    = MDMA_DEST_DATASIZE_BYTE;
	hmdmaOut.Init.DataAlignment   = MDMA_DATAALIGN_PACKENABLE;
	hmdmaOut.Init.SourceBurst     = MDMA_SOURCE_BURST_32BEATS;
	hmdmaOut.Init.DestBurst       = MDMA_DEST_BURST_32BEATS;  
	hmdmaOut.Init.SourceBlockAddressOffset = 0;
	hmdmaOut.Init.DestBlockAddressOffset  = 0;

	/*Using JPEG Output FIFO Threshold as a trigger for the MDMA*/
	hmdmaOut.Init.Request              = MDMA_REQUEST_JPEG_OUTFIFO_TH; /* Set the MDMA HW trigger to JPEG Output FIFO Threshold flag*/ 
	hmdmaOut.Init.TransferTriggerMode  = MDMA_BUFFER_TRANSFER;    
	hmdmaOut.Init.BufferTransferLength = 32; /*Set the MDMA buffer size to the JPEG FIFO threshold size i.e 32 bytes (8 words)*/

	hmdmaOut.Instance = MDMA_Channel6;
	/* DeInitialize the DMA Stream */
	HAL_MDMA_DeInit(&hmdmaOut);  
	/* Initialize the DMA stream */
	HAL_MDMA_Init(&hmdmaOut);

	/* Associate the DMA handle */
	__HAL_LINKDMA(hjpeg, hdmaout, hmdmaOut);

	HAL_NVIC_SetPriority(MDMA_IRQn, 0x08, 0);
	HAL_NVIC_EnableIRQ(MDMA_IRQn); 
}


void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg)
{
    __HAL_RCC_JPEG_CLK_DISABLE();	
	__HAL_RCC_JPGDECEN_CLK_DISABLE();
	
    HAL_MDMA_DeInit(hjpeg->hdmain);
    HAL_MDMA_DeInit(hjpeg->hdmaout);

    HAL_NVIC_DisableIRQ(JPEG_IRQn);
}

void JPEG_IRQHandler(void)
{
	HAL_JPEG_IRQHandler(&JPEG_Handle);
}

void MDMA_IRQHandler()
{
	/* Check the interrupt and clear flag */
	HAL_MDMA_IRQHandler(JPEG_Handle.hdmain);
	HAL_MDMA_IRQHandler(JPEG_Handle.hdmaout);  
}

void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling)
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
  destination = (uint32_t)pDst + ((y * GUI_XSIZE) + x) * 2;

  HAL_DMA2D_Start(&DMA2D_Handle, (uint32_t)pSrc, destination, xsize, ysize);
  HAL_DMA2D_PollForTransfer(&DMA2D_Handle, 25);  /* wait for the previous DMA2D transfer to ends */
}

void _Write_AllPixels_RGB(HDC hdc,const u8 *p, int x, int y, int xSize)
{
	BITMAP bm;
	bm.Format	 =BM_RGB565;
	bm.Width	 =JPEG_Info.ImageWidth;
	bm.Height	 =JPEG_Info.ImageHeight;;
	bm.WidthBytes =GUI_XSIZE*2;//JPEG_Info.ImageWidth * 2;
	bm.Bits	    =(u8*)p;
	bm.LUT		=NULL;
	DrawBitmap(hdc,x,y,&bm,NULL);
}



FIL JPEG_File;

static	BOOL _Draw(HDC hdc, int x, int y,JPG_DEC *dec)
{
	
			HAL_JPEG_DeInit(&JPEG_Handle);
	
      JPEG_Handle.Instance = JPEG;
	
      HAL_JPEG_Init(&JPEG_Handle);
       
      Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(3*512*1024);
      OUT_Buffer = GUI_VMEM_Alloc(800*480*2); 
	
			JPEG_Decode_Mem();
	
			JPEG_picture((uint32_t *)Decode_Buffer);
	
			DMA2D_CopyBuffer((uint32_t *)Decode_Buffer, (uint32_t *)OUT_Buffer, 0 , 0, JPEG_Info.ImageWidth, JPEG_Info.ImageHeight, JPEG_Info.ChromaSubsampling);

			_Write_AllPixels_RGB(hdc,(uint8_t*)OUT_Buffer,0,0,800);
	
			JPEG_Decode_free();

			
			Jpeg_HWDecodingEnd = 0;//解码完成,恢复结束标志为初始状态
			
//			HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
			

      GUI_VMEM_Free(Decode_Buffer);
      GUI_VMEM_Free(OUT_Buffer);
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
	
  JPEG_Handle.Instance = JPEG;
	
  HAL_JPEG_Init(&JPEG_Handle);
       
	Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(3*512*1024);

	JPEG_Decode_Mem();

	JPEG_picture((uint32_t *)Decode_Buffer);

	JPEG_Decode_free();

	Jpeg_HWDecodingEnd = 0;//解码完成,恢复结束标志为初始状态
		
	GUI_VMEM_Free(Decode_Buffer);
	
	HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
	/* 恢复指针位置和大小 */
	Decode_dec->pSrcData =jdec->pSrcData;
	Decode_dec->DataSize =jdec->DataSize;
	if(width!=NULL && height!=NULL)
	{
		*width =JPEG_Info.ImageWidth;
	  *height =JPEG_Info.ImageHeight;
   	return TRUE;	
	}
	else
	{
		return FALSE;	
	}
}

/**
  * @brief  从文件系统读出的图像数据中,拷贝数据
  * @param 	Buffer_Dest       : 拷贝到哪里
  * @param  Byte_Want_Read    : 想读取几个字节
  * @param  Byte_Real_Read    : 实际读取到的字节(因为可能到达文件尾部)
  * @retval True of False
  */
uint32_t Data_Offset = 0;

FRESULT Read_Buf_From_File(void* Buffer_Dest, uint32_t Byte_Want_Read , uint32_t* Byte_Real_Read)
{
	uint8_t * Temp_pointer = (uint8_t *)Decode_dec->pSrcData;
  if( (Buffer_Dest != NULL) && (Decode_dec->pSrcData != NULL) && (Decode_dec->DataSize >0))
  {
    if(Decode_dec->DataSize >= Byte_Want_Read)
    {
      /* 如果数据的大小>想要读的数据,直接拷贝,并把数据源地址指针偏移读取的字节数,数据大小减去读取字节的数量 */
			taskENTER_CRITICAL();
			memcpy((uint8_t *)Buffer_Dest,Temp_pointer + Data_Offset,Byte_Want_Read);
			taskEXIT_CRITICAL();
			
      *Byte_Real_Read = Byte_Want_Read;
			Data_Offset += Byte_Want_Read; //数据偏移量
      Decode_dec->DataSize -= Byte_Want_Read;
      return FR_OK;
    }
    else
    {
      /* 如果数据的大小<想要读的数据,拷贝剩余的数据大小Decode_dec->DataSize,并把数据源地址指针偏移读取的字节数,数据置零 */
			taskENTER_CRITICAL();
      memcpy((uint8_t *)Buffer_Dest,Temp_pointer + Data_Offset,Decode_dec->DataSize);
			taskEXIT_CRITICAL();
			
			*Byte_Real_Read = Decode_dec->DataSize;
      Decode_dec->DataSize = 0;
			Data_Offset = 0;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
