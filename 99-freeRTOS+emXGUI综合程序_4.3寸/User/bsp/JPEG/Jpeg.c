#include "./jpeg/Jpeg.h"
/* JPEG硬件解码功能,选用(缓冲区消耗较大),默认使用软件解码 */
#define CHUNK_SIZE_IN  ((uint32_t)(64 * 1024)) /* 一次要解码的数据块大小 */
#define CHUNK_SIZE_OUT ((uint32_t)(64 * 1024)) 

uint32_t *Decode_Buffer;	/* 从文件系统中读取出的数据缓冲区 */
uint32_t *OUT_Buffer; 		/* 硬解码后数据存放的缓冲区 */

uint32_t JPEG_Source_Address;		/* JPEG数据源地址 */
uint32_t Frame_Buffer_Address;	/* 解码后数据存放的缓冲区 */
uint32_t Input_Frame_Size;			/* JPEG源数据大小 */
uint32_t Input_Frame_Index;			/* JPEG解码中,已经解码的数据大小 */

__IO uint8_t Jpeg_HWDecodingEnd = 0;		/* 所有数据完成标志 */

JPG_DEC	*Decode_dec;		/* 图片信息句柄,存放图片的BUFF首地址,大小信息 */

struct	__jpg_dec
{
	const void	*pSrcData;
	U32		DataSize;	
};

static DMA2D_HandleTypeDef    DMA2D_Handle;		/* DMA2D句柄 */


void JPEG_picture(uint32_t *databuf)
{
	JPEG_Source_Address = (uint32_t)Decode_dec->pSrcData;		/* 从DEC中获取待解码缓冲区首地址 */
	Input_Frame_Size = Decode_dec->DataSize;								/* 从DEC中获取待解码数据大小(图片大小) */
	Frame_Buffer_Address = (uint32_t)databuf;								/* 赋值解码后数据的存放缓冲区 */
	Input_Frame_Index = 0;		/* 复位解码偏移量 */
	Jpeg_HWDecodingEnd = 0;		/* 解码完成标志 */
	HAL_JPEG_Decode_DMA(&JPEG_Handle ,(uint8_t *)JPEG_Source_Address,CHUNK_SIZE_IN ,(uint8_t *)Frame_Buffer_Address ,CHUNK_SIZE_OUT);		/* 开始解码 */
}

/*----------------------------解码处理过程-------------------------------*/
static MDMA_HandleTypeDef   hmdmaIn;
static MDMA_HandleTypeDef   hmdmaOut; 

JPEG_HandleTypeDef    JPEG_Handle;		/* 句柄 */
JPEG_ConfTypeDef      JPEG_Info;			/* 解码后获取的JPEG信息句柄 */

/**
  * @brief  传入数据解码完成,请求新的未解码数据回调函数
  * @param hjpeg: JPEG句柄指针
  * @param NbDecodedData: 从输入的缓冲区解码出来的数据大小
  * @retval None
  */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	uint32_t Input_DataLength = 0; 
	
	/* 更新已经解码的数据大小,Input_Frame_Index标识当前解码的位置 */
	Input_Frame_Index += NbDecodedData;
	
	/* 当前已经解码的数据小于总文件大小,继续解码 */
	if( Input_Frame_Index < Input_Frame_Size)
	{
		/* 偏移解码数据位置,从未解码的数据开始继续解码 */
		JPEG_Source_Address = JPEG_Source_Address + NbDecodedData;

		/* 调整要解码的数据大小 */
		if((Input_Frame_Size - Input_Frame_Index) >= CHUNK_SIZE_IN)
		{
			Input_DataLength = CHUNK_SIZE_IN;
		}
		else
		{
			Input_DataLength = Input_Frame_Size - Input_Frame_Index;
		}    
	}
	else
	{
		/* 解码完成或者解码出错,输入0个数据,停止 */
		Input_DataLength = 0; 
	}
	
	/* 更新输入缓冲 */
	HAL_JPEG_ConfigInputBuffer(hjpeg,(uint8_t *)JPEG_Source_Address, Input_DataLength);  
}

/**
  * @brief  数据解码完成,解码后的数据就绪处理回调函数
  * @param hjpeg: JPEG句柄指针
  * @param pDataOut: 指向存放数据的缓存区地址
  * @param NbDecodedData: 从输入的缓冲区解码出来的数据大小
  * @retval None
  */
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	/* 将JPEG解码后的数据,存放进缓存区,并偏移出解码的数据块大小 */  
	Frame_Buffer_Address += OutDataLength;
	
	/* 输出到缓冲区中 */  
	HAL_JPEG_ConfigOutputBuffer(hjpeg, (uint8_t *)Frame_Buffer_Address, CHUNK_SIZE_OUT); 
}

/**
  * @brief  JPEG全部数据解码完成回调函数
  * @param hjpeg: JPEG 句柄指针
  * @retval None
  */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
	/* 解码完成标志位,置位 */  
	Jpeg_HWDecodingEnd = 1;
}

/**
  * @brief  JPEG底层初始函数
  * @param hjpeg: JPEG 句柄指针
  * @retval None
  */
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
	__HAL_RCC_JPEG_CLK_ENABLE();
	
	/* 使能 JPEG 时钟 */
	__HAL_RCC_JPGDECEN_CLK_ENABLE();

	/* 使能 MDMA 时钟 */
	__HAL_RCC_MDMA_CLK_ENABLE();
	
	/* 使能 JPEG 中断优先级 */
	HAL_NVIC_SetPriority(JPEG_IRQn, 0x07, 0);
	HAL_NVIC_EnableIRQ(JPEG_IRQn);  

	/* 输入的 MDMA 配置*/
	hmdmaIn.Instance                = MDMA_Channel7;												/* 设置MDMA 通道*/
	hmdmaIn.Init.Priority           = MDMA_PRIORITY_HIGH;										/* 设置MDMA 优先级*/
	hmdmaIn.Init.Endianness         = MDMA_LITTLE_ENDIANNESS_PRESERVE;			/* 设置MDMA 大小端模式:小端*/
	hmdmaIn.Init.SourceInc          = MDMA_SRC_INC_BYTE;										/* 设置MDMA 源数据地址以BYTE自增*/
	hmdmaIn.Init.DestinationInc     = MDMA_DEST_INC_DISABLE;								/* 设置MDMA 目标地址不自增*/
	hmdmaIn.Init.SourceDataSize     = MDMA_SRC_DATASIZE_BYTE;								/* 设置MDMA 单个源数据大小为BYTE*/
	hmdmaIn.Init.DestDataSize       = MDMA_DEST_DATASIZE_WORD;							/* 设置MDMA 目标数据大小为4个BYTE*/
	hmdmaIn.Init.DataAlignment      = MDMA_DATAALIGN_PACKENABLE; 						/* 设置MDMA 数据包对齐*/  
	hmdmaIn.Init.SourceBurst        = MDMA_SOURCE_BURST_32BEATS;						/* 设置MDMA 源为32位突发传输*/
	hmdmaIn.Init.DestBurst          = MDMA_DEST_BURST_16BEATS; 							/* 设置MDMA 目的为16位突发传输*/
	hmdmaIn.Init.SourceBlockAddressOffset = 0;	/* 设置MDMA 源数据缓冲区偏移*/
	hmdmaIn.Init.DestBlockAddressOffset  = 0;		/* 设置MDMA 目标数据缓冲区偏移*/
	
	/*使用JPEG输入FIFO传输触发模式,以控制MDMA传输 */
	
	/* 设置FIFO触发传输的标志为:请求输入时触发*/  
	hmdmaIn.Init.Request = MDMA_REQUEST_JPEG_INFIFO_TH; 
	hmdmaIn.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;  
	hmdmaIn.Init.BufferTransferLength = 32; /* 将MDMA缓冲区大小设置为JPEG FIFO阈值大小32个字节(8字) */

	/* 链接JPEG句柄和MDMA句柄 */
	__HAL_LINKDMA(hjpeg, hdmain, hmdmaIn);

	/* 反初始化输入DMA流 */
	HAL_MDMA_DeInit(&hmdmaIn);  
	/* 初始化输入DMA流 */
	HAL_MDMA_Init(&hmdmaIn);

	/* 输出的 MDMA 配置,同上类似*/
	hmdmaOut.Instance             = MDMA_Channel6;
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

	/* DeInitialize the DMA Stream */
	HAL_MDMA_DeInit(&hmdmaOut);  
	/* Initialize the DMA stream */
	HAL_MDMA_Init(&hmdmaOut);

	/* Associate the DMA handle */
	__HAL_LINKDMA(hjpeg, hdmaout, hmdmaOut);

	HAL_NVIC_SetPriority(MDMA_IRQn, 0x08, 0);
	HAL_NVIC_EnableIRQ(MDMA_IRQn); 
}

/**
	* @brief  JPEG底层反初始函数,用于关闭JPEG工作
  * @param hjpeg: JPEG 句柄指针
  * @retval None
  */
void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg)
{
	  HAL_NVIC_DisableIRQ(JPEG_IRQn);
	
    __HAL_RCC_JPEG_CLK_DISABLE();	
		__HAL_RCC_JPGDECEN_CLK_DISABLE();
	
    HAL_MDMA_DeInit(hjpeg->hdmain);
    HAL_MDMA_DeInit(hjpeg->hdmaout);
}

/**
	* @brief  JPEG中断函数
  * @param None
  * @retval None
  */
void JPEG_IRQHandler(void)
{
	HAL_JPEG_IRQHandler(&JPEG_Handle);
}

/**
	* @brief  MDMA中断函数
  * @param None
  * @retval None
  */
void MDMA_IRQHandler()
{
	/* 判断中断标志位,并调用相应的回调函数 */
	HAL_MDMA_IRQHandler(JPEG_Handle.hdmain);
	HAL_MDMA_IRQHandler(JPEG_Handle.hdmaout);  
}

/**
	* @brief  使用DMA2D将解码的图像数据(YCBCR)转换成LCD屏幕可以显示的(RGB565)格式
  * @param pSrc: 源数据缓冲区地址
  * @param pDst: 目标数据缓冲区地址(可以为屏幕缓冲区)
  * @param xsize: 解码的图像宽度
  * @param ysize: 解码的图像高度
  * @param ChromaSampling: 图片色彩采样度
  * @retval None
  */
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling)
{   
  uint32_t cssMode = DMA2D_CSS_420, inputLineOffset = 0;  
	
	/* 根据色彩采样度设置行偏移 */
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
  
	/* 配置DMA2D模式、输出色彩模式、输出坐标偏移等 */
  DMA2D_Handle.Init.Mode         = DMA2D_M2M_PFC;					/* DMA2D采用内存到内存的搬运方式 */
  DMA2D_Handle.Init.ColorMode    = DMA2D_OUTPUT_RGB565;		/* 输出格式为RGB565,可以直接显示到RGB屏幕 */  
  DMA2D_Handle.Init.OutputOffset = 0; 										/* 输出偏移 */  
  DMA2D_Handle.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* 不翻转alpha值*/  
  DMA2D_Handle.Init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* 不进行红蓝像素交换,该功能为了支持 BGR 或 ABGR 颜色格式 */  
  
  /* 配置DMA2D传输完成中断回调函数,不设置回调 */
  DMA2D_Handle.XferCpltCallback  = NULL;
  
  /* 前景层配置 */
  DMA2D_Handle.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;			/* 原始前景层图像的 alpha 通道值替换为 InputAlpha - 寄存器ALPHA[7:0] */  
  DMA2D_Handle.LayerCfg[1].InputAlpha = 0xFF;										/* 寄存器ALPHA[7:0] */ 
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;	/* 定义前景层图像的颜色格式 */ 
  DMA2D_Handle.LayerCfg[1].ChromaSubSampling = cssMode;					/* 色度子采样模式 */ 
  DMA2D_Handle.LayerCfg[1].InputOffset = inputLineOffset;				/* 输入行偏移 */ 
  DMA2D_Handle.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; 			/* 不进行红蓝像素交换,该功能为了支持 BGR 或 ABGR 颜色格式 */
  DMA2D_Handle.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; /* 不翻转alpha值 */  
  DMA2D_Handle.Instance          = DMA2D; 
  
  /* 根据上述配置初始化DMA2D */
	if (   HAL_DMA2D_Init(&DMA2D_Handle) !=  HAL_OK) 
	{
		printf("ERROR 1\n");
	}
	if ( HAL_DMA2D_ConfigLayer(&DMA2D_Handle, 1) !=  HAL_OK) 
	{
		printf("ERROR 2\n");
	}
 
  /* 开启DMA2D,使用库函数会超时报错,转换也很慢,可能哪里配置的不对,直接操作寄存器就ok */	
	DMA2D->NLR     = (uint32_t)(xsize << 16) | (uint16_t)ysize;	/* 配置输出图像大小 */  
	DMA2D->OMAR    = (uint32_t)pDst;	/* 配置源数据的缓冲区地址 */  
	DMA2D->FGMAR   = (uint32_t)pSrc;  /* 配置存放数据的缓冲区地址 */  
	DMA2D->CR   |= DMA2D_CR_START; 		/* 开始转换 */  
	while (DMA2D->CR & DMA2D_CR_START) {} 	/* 等待转换完成 */  

  /* 库函数,不用 */	
//  if ( HAL_DMA2D_Start(&DMA2D_Handle, (uint32_t)pSrc, (uint32_t)pDst, xsize, ysize) !=  HAL_OK) 
//	{
//		printf("ERROR 3\n");
//	}
//  if (HAL_DMA2D_PollForTransfer(&DMA2D_Handle, HAL_MAX_DELAY) != HAL_OK)  /* wait for the previous DMA2D transfer to ends */
//	{
//		printf("ERROR 4\n");
//	}
}

void _Write_AllPixels_RGB(HDC hdc,const u8 *p, int x, int y, int xSize)
{
	BITMAP bm;
	bm.Format	 =BM_RGB565;
	bm.Width	 =JPEG_Info.ImageWidth;
	bm.Height	 =JPEG_Info.ImageHeight;;
	bm.WidthBytes =xSize*2;//JPEG_Info.ImageWidth * 2;
	bm.Bits	    =(u8*)p;
	bm.LUT		=NULL;
	DrawBitmap(hdc,x,y,&bm,NULL);
}

static	BOOL _Draw(HDC hdc, int x, int y,JPG_DEC *dec)
{
	HAL_JPEG_DeInit(&JPEG_Handle);

	JPEG_Handle.Instance = JPEG;

	HAL_JPEG_Init(&JPEG_Handle);
	 
	Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(3*512*1024);
	OUT_Buffer = GUI_VMEM_Alloc(800*480*2); 

	JPEG_picture((uint32_t *)Decode_Buffer);
	/* 等待解码完成 */
	while(Jpeg_HWDecodingEnd == 0)
	{
	}
	HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);

	DMA2D_CopyBuffer((uint32_t *)Decode_Buffer, (uint32_t *)OUT_Buffer, JPEG_Info.ImageWidth, JPEG_Info.ImageHeight ,JPEG_Info.ChromaSubsampling);


	_Write_AllPixels_RGB(hdc,(uint8_t*)OUT_Buffer,0,0,JPEG_Info.ImageWidth);
	
	Jpeg_HWDecodingEnd = 0;//解码完成,恢复结束标志为初始状态
	
	GUI_VMEM_Free(Decode_Buffer);
	GUI_VMEM_Free(OUT_Buffer);
	return 1;
}

BOOL	JPG_Draw(HDC hdc,int x,int y,JPG_DEC *jdec)
{
	if(jdec == NULL)
	{
		return FALSE;
	}
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
	uint32_t TimeOut = 0;
	if(jdec==NULL)
	{
	  return FALSE;
	}	
	
  JPEG_Handle.Instance = JPEG;
	
  HAL_JPEG_Init(&JPEG_Handle);
       
	Decode_Buffer = (uint32_t *)GUI_VMEM_Alloc(3*512*1024);

	JPEG_picture((uint32_t *)Decode_Buffer);
	/* 解码完成 */
	while(Jpeg_HWDecodingEnd == 0 && TimeOut <0xfffff)
	{
		TimeOut++;
	}
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
