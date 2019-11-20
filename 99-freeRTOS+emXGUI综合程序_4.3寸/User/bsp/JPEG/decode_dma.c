/**
  ******************************************************************************
  * @file    JPEG/JPEG_DecodingUsingFs_DMA/CM7/Src/decode_dma.c
  * @author  MCD Application Team
  * @brief   This file provides routines for JPEG decoding with DMA method.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "decode_dma.h"
#include "emXGUI.h"
/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup JPEG_DecodingUsingFs_DMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  __IO  uint8_t State;  
  uint8_t *DataBuffer;
  __IO  uint32_t DataBufferSize;

}JPEG_Data_BufferTypeDef;

/* Private define ------------------------------------------------------------*/

#define CHUNK_SIZE_IN  ((uint32_t)(4096)) 
#define CHUNK_SIZE_OUT ((uint32_t)(64 * 1024))

#define JPEG_BUFFER_EMPTY 0
#define JPEG_BUFFER_FULL  1

#define NB_INPUT_DATA_BUFFERS       2

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
JPEG_HandleTypeDef    JPEG_Handle;
JPEG_ConfTypeDef      JPEG_Info;

uint8_t JPEG_Data_InBuffer0[CHUNK_SIZE_IN];
uint8_t JPEG_Data_InBuffer1[CHUNK_SIZE_IN];


JPEG_Data_BufferTypeDef Jpeg_IN_BufferTab[NB_INPUT_DATA_BUFFERS] =
{
  {JPEG_BUFFER_EMPTY , JPEG_Data_InBuffer0, 0},
  {JPEG_BUFFER_EMPTY , JPEG_Data_InBuffer1, 0}
};

uint32_t MCU_TotalNb = 0;
uint32_t MCU_BlockIndex = 0;
uint32_t Jpeg_HWDecodingEnd = 0;

uint32_t JPEG_IN_Read_BufferIndex = 0;
uint32_t JPEG_IN_Write_BufferIndex = 0;
__IO uint32_t Input_Is_Paused = 0;

uint32_t FrameBufferAddress;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Decode_DMA,初次调用解码函数
  * @param hjpeg: JPEG handle pointer,HAL JPEG句柄
  * @param  DestAddress : 解码后数据存放的BUF.
  * @retval None
  */
uint32_t JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg,uint32_t DestAddress)
{
  uint32_t i;
  
  FrameBufferAddress = DestAddress;
          
  /* Read from JPG file and fill input buffers */
	/* 循环两次将双缓冲区填入数据 */
  for(i = 0; i < NB_INPUT_DATA_BUFFERS; i++)
  {
    if(Read_Buf_From_File (Jpeg_IN_BufferTab[i].DataBuffer , CHUNK_SIZE_IN, (UINT*)(&Jpeg_IN_BufferTab[i].DataBufferSize)) == FR_OK)
    {
      Jpeg_IN_BufferTab[i].State = JPEG_BUFFER_FULL;
    }
    else
    {
			/* 否则数据填充失败 */
    }        
  } 
  /* Start JPEG decoding with DMA method */
	/* 开始解码 */
  HAL_JPEG_Decode_DMA(hjpeg ,Jpeg_IN_BufferTab[0].DataBuffer ,Jpeg_IN_BufferTab[0].DataBufferSize ,(uint8_t *)FrameBufferAddress ,CHUNK_SIZE_OUT);
  
  return 0;
}

/**
  * @brief  JPEG Input Data BackGround processing .轮询处理解码数据块任务,这里切换缓冲区
  * @param hjpeg: JPEG handle pointer
  * @retval None
*/
uint32_t JPEG_InputHandler(JPEG_HandleTypeDef *hjpeg)
{
  if(Jpeg_HWDecodingEnd == 0)
  {
    if(Jpeg_IN_BufferTab[JPEG_IN_Write_BufferIndex].State == JPEG_BUFFER_EMPTY)
    {
			/* 如果BUF为空(第一次解码调用JPEG_Decode_DMA,会改变BUF状态),读取数据,并改变BUF状态,如果读取不到,可能出错或者是所有读取完成 */
      if(Read_Buf_From_File(Jpeg_IN_BufferTab[JPEG_IN_Write_BufferIndex].DataBuffer , CHUNK_SIZE_IN, (UINT*)(&Jpeg_IN_BufferTab[JPEG_IN_Write_BufferIndex].DataBufferSize)) == FR_OK)
      {  
        Jpeg_IN_BufferTab[JPEG_IN_Write_BufferIndex].State = JPEG_BUFFER_FULL;
      }
      else
      {
//        Error_Handler();
      }
			
      /* Input_Is_Paused由中断回调置位,该位置位表示输入缓冲区状态为空。下if语句中的条件为新的数据缓冲区准备完成,准备恢复解码 */
      if((Input_Is_Paused == 1) && (JPEG_IN_Write_BufferIndex == JPEG_IN_Read_BufferIndex))
      {
        Input_Is_Paused = 0;
        HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBuffer, Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBufferSize);    
        
        HAL_JPEG_Resume(hjpeg, JPEG_PAUSE_RESUME_INPUT); 
      }
			
      /* 此处实现由询填充缓冲区,JPEG_IN_Write_BufferIndex在每次轮询中改变,只要输入BUF状态为空,就会被外层语句判断并填充 */
      JPEG_IN_Write_BufferIndex++;
      if(JPEG_IN_Write_BufferIndex >= NB_INPUT_DATA_BUFFERS)
      {
        JPEG_IN_Write_BufferIndex = 0;
      }            
    }
    return 0;
  }
  else
  {
    return 1;
  }
}

/**
  * @brief  JPEG Info ready callback,JPEG头数据解码完成回调函数,通知用户JPEG图像数据信息解码完成,可以读取了
  * @param hjpeg: JPEG handle pointer
  * @param pInfo: JPEG Info Struct pointer
  * @retval None
  */
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo)
{  
#if 0
	if(JPEG_Info_Flag == 1)
	{
		GUI_DEBUG("Jpeg Info is ready ! \r\n");
		HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);
		GUI_DEBUG("\r\n  ChromaSampling : %d  \r\n",JPEG_Info.ChromaSubsampling);
		GUI_DEBUG("\r\n JPEG_Info.ImageWidth: %d,\r\n JPEG_Info.ImageHeight: %d.",JPEG_Info.ImageWidth,JPEG_Info.ImageHeight);
		GUI_VMEM_Free(JPEG_Info_Buf);
	}
#endif
}

/**
  * @brief  JPEG Get Data callback,获取新的数据块回调
  * @param hjpeg: JPEG handle pointer,HAL JPEG句柄
  * @param NbDecodedData: Number of decoded (consummed) bytes from input buffer,先前传入需要解码的数据块已经解码的大小
  * @retval None
  */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	/* 如果已经解码块的大小和之前传入需要解码块大小相等,说明该块解码完成,准备新数据或所以数据解码完成。否则继续解码该块 */
  if(NbDecodedData == Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBufferSize)
  {  
    Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].State = JPEG_BUFFER_EMPTY;//该块解码完成,改变状态
    Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBufferSize = 0;//复位buf大小
  
    JPEG_IN_Read_BufferIndex++;
    if(JPEG_IN_Read_BufferIndex >= NB_INPUT_DATA_BUFFERS)//双缓存,切换缓冲区
    {
      JPEG_IN_Read_BufferIndex = 0;        
    }
  
		/* 如果下一块缓冲区状态为空,有三种情况:数据读取完毕、数据缓冲区未就绪、读取数据失败。操作为暂停解码,如果下一块缓冲区不为空,配置新的解码数据 */
    if(Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].State == JPEG_BUFFER_EMPTY)
    {
      HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
      Input_Is_Paused = 1;
    }
    else
    {    
      HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBuffer, Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBufferSize);    
    }
  }
  else
  {
    HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBuffer + NbDecodedData, Jpeg_IN_BufferTab[JPEG_IN_Read_BufferIndex].DataBufferSize);      
  }
}

/**
  * @brief  JPEG Data Ready callback,JPEG解码输入的数据块完成回调函数
  * @param hjpeg: JPEG handle pointer,HAL JPEG句柄
  * @param pDataOut: pointer to the output data buffer,指向输出缓冲区
  * @param OutDataLength: length of output buffer in bytes,指定输出缓冲区中可用数据的字节数
  * @retval None
  */
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
  /* Update JPEG encoder output buffer address*/  
	/* 输出缓冲区中已经解码OutDataLength个字节可以,buf地址加OutDataLength,配置输出缓冲区(会将缓冲区里的内容拷贝到指定的BUF(FrameBufferAddress)) */
  FrameBufferAddress += OutDataLength;
#if 0
	GUI_DEBUG("Number Byte is decode :  %d",OutDataLength);
#endif 
  HAL_JPEG_ConfigOutputBuffer(hjpeg, (uint8_t *)FrameBufferAddress, CHUNK_SIZE_OUT); 
}

/**
  * @brief  JPEG Error callback,解码出错回调函数
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
	GUI_DEBUG("Error_Handler();\r\n");
}

/**
  * @brief  JPEG Decode complete callback,所有数据解码完成中断回调函数,通知用户解码完成
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{    
	GUI_DEBUG("解码完成\r\n");
  Jpeg_HWDecodingEnd = 1; //解码结束标志,人为设置
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @brief  HAL_JPEG_MspInit,底层初始化,配置JPEG解码数据MDAM的数据采集模式
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
  static MDMA_HandleTypeDef   hmdmaIn;
  static MDMA_HandleTypeDef   hmdmaOut;  
  
  /* Enable JPEG clock */
  __HAL_RCC_JPGDECEN_CLK_ENABLE();
  
  /* Enable MDMA clock */
  __HAL_RCC_MDMA_CLK_ENABLE();
  
  HAL_NVIC_SetPriority(JPEG_IRQn, 1, 0);
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
  
  
  HAL_NVIC_SetPriority(MDMA_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);
  
}

/**
  * @brief  HAL_JPEG_MspInit,底层反初始化,反向配置JPEG解码数据MDAM的数据采集模式
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg)
{
  HAL_NVIC_DisableIRQ(MDMA_IRQn);
  
  /* DeInitialize the MDMA Stream */
  HAL_MDMA_DeInit(hjpeg->hdmain);
  
  /* DeInitialize the MDMA Stream */
  HAL_MDMA_DeInit(hjpeg->hdmaout);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
