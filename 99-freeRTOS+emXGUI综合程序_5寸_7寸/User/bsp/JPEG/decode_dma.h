/**
  ******************************************************************************
  * @file    JPEG/JPEG_DecodingUsingFs_DMA/CM7/Inc/decode_dma.h
  * @author  MCD Application Team
  * @brief   Header for decode_dma.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DECODE_DMA_H
#define __DECODE_DMA_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx.h"
#include "ff.h"
#include "JPEG_LIB.h"
/* Exported variables --------------------------------------------------------*/
extern JPEG_HandleTypeDef    JPEG_Handle;
extern JPEG_ConfTypeDef      JPEG_Info;
extern uint32_t              Jpeg_HWDecodingEnd;//ת��������־
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
// uint32_t JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg, FIL *file, uint32_t DestAddress);
uint32_t JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg, uint32_t DestAddress);
uint32_t JPEG_InputHandler(JPEG_HandleTypeDef *hjpeg);
//uint32_t JPEG_Get_Info();
#endif /* __DECODE_DMA_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
