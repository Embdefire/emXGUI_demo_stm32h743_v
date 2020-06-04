/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_storage_if.c
  * @version        : v1.0_Cube
  * @brief          : Memory management layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"
#include "gui_fs_port.h"
#include "ff.h"
#include "ff_gen_drv.h"
#if defined(STM32F429_439xx)
#include "sdio/bsp_sdio_sd.h"
#elif defined(STM32H743xx)
#include "./sd_card/bsp_sdio_sd.h"
#endif
#include "./drivers/fatfs_sd_sdio.h"
#include "FreeRTOS.h"
/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
extern SD_HandleTypeDef uSdHandle;
HAL_SD_CardInfoTypeDef pCardInfo;
//发送标志位
extern volatile uint8_t TX_Flag;
//接受标志位
extern volatile uint8_t RX_Flag; 
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @defgroup USBD_STORAGE
  * @brief Usb mass storage device module
  * @{
  */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Defines
  * @brief Private defines.
  * @{
  */

#define STORAGE_LUN_NBR                  1
uint32_t STORAGE_BLK_NBR=0;
//#define STORAGE_BLK_NBR                  512*1024
#define STORAGE_BLK_SIZ                  512

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN INQUIRY_DATA_HS */ 
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_HS[] = {/* 36 */
  
  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,	
  0x00,
  'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  '0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
}; 
/* USER CODE END INQUIRY_DATA_HS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t STORAGE_Init_HS(uint8_t lun);
static int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_HS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_HS(uint8_t lun);
static int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_HS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_StorageTypeDef USBD_Storage_Interface_fops_HS =
{
  STORAGE_Init_HS,
  STORAGE_GetCapacity_HS,
  STORAGE_IsReady_HS,
  STORAGE_IsWriteProtected_HS,
  STORAGE_Read_HS,
  STORAGE_Write_HS,
  STORAGE_GetMaxLun_HS,
  (int8_t *)STORAGE_Inquirydata_HS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Init_HS(uint8_t lun)
{
  /* USER CODE BEGIN 9 */

	if( HAL_SD_GetCardInfo(&uSdHandle,&pCardInfo) != 0 )
	{
		printf(" HAL_SD_GetCardInfo failed \n");
		return (USBD_FAIL);
	}
	printf("Card Info: \n\rCardType->%u\n\
						\rBlockNbr->%u\n\
						\rBlockSize->%u\n\
	",pCardInfo.CardType,pCardInfo.BlockNbr,pCardInfo.BlockSize);
	STORAGE_BLK_NBR = pCardInfo.BlockNbr;
  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  block_num: .
  * @param  block_size: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
  /* USER CODE BEGIN 10 */
  *block_num  = STORAGE_BLK_NBR;
  *block_size = STORAGE_BLK_SIZ;
  return (USBD_OK);
  /* USER CODE END 10 */
}

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsReady_HS(uint8_t lun)
{
  /* USER CODE BEGIN 11 */
  return (USBD_OK);
  /* USER CODE END 11 */
}

/**
  * @brief  .
  * @param  lun: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsWriteProtected_HS(uint8_t lun)
{
  /* USER CODE BEGIN 12 */
  return (USBD_OK);
  /* USER CODE END 12 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  buf: .
  * @param  blk_addr: .
  * @param  blk_len: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
#if 0
//	HAL_SD_ReadBlocks_DMA(&uSdHandle,buf,blk_addr*512,blk_len);
//	if ( SD_read(lun,buf,blk_addr,blk_len) !=0)
//	{
//		printf(" STORAGE_Read_HS Failed! \n");
//	}
//	printf("-------------STORAGE_Read_HS---------------\n");
//	printf("blk_addr: %d ---- sector:%u ----blk_len:%d\n",blk_addr,blk_addr*STORAGE_BLK_SIZ,blk_len);
	DRESULT res = RES_ERROR;
	uint32_t i;
	DWORD pbuff[512/4];	

	uint32_t alignedAddr; 

	RX_Flag = 0;

	if((DWORD)buf&3)
	{
	DRESULT res = RES_OK;
	DWORD scratch[BLOCKSIZE / 4];

	while (blk_len--) 
	{
		res = disk_read(0,(void *)scratch, (blk_addr++), 1);

	if (res != RES_OK) 
	{
		break;
	}
		memcpy(buf, scratch, BLOCKSIZE);
		buf += BLOCKSIZE;
	}
	return res;
	}

//	GUI_MutexLock(mutex_lock,0xffffff);
	for(i=0;i<blk_len;i++)
	{
//		res = SD_ReadBlocks((BYTE *)pbuff,blk_len+i,1);//单个sector的读操作
		  DRESULT res = RES_ERROR;  
		  uint32_t timeout;
		  uint32_t alignedAddr;
		  RX_Flag = 0;

//		  taskENTER_CRITICAL();
		  if(HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t*)pbuff,
								   (uint32_t) (blk_addr),
								   1) == HAL_OK)
		  {
//			taskEXIT_CRITICAL();
			/* Wait that the reading process is completed or a timeout occurs */
//			GUI_SemWait(sem_sd,2000);
			{
				  res = RES_OK;
				  /*
					 the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
					 adjust the address and the D-Cache size to invalidate accordingly.
				   */
				  alignedAddr = (uint32_t)pbuff & ~0x1F;
				  //使相应的DCache无效
				  SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, blk_len*BLOCKSIZE + ((uint32_t)pbuff - alignedAddr));
			}
		  }else
		  {
//			taskEXIT_CRITICAL();//if the HAL_SD_ReadBlocks_DMA operate failed "taskEXIT_CRITICAL" also should be run 
		  }
//	  taskENTER_CRITICAL();
		memcpy(buf,pbuff,512);
//	  taskEXIT_CRITICAL();
		buf+=512;
	}
//	GUI_MutexUnlock(mutex_lock);
#endif
HAL_StatusTypeDef state;
UBaseType_t uxSavedInterruptStatus;
RX_Flag=0;
uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
state = HAL_SD_ReadBlocks(&uSdHandle, (uint8_t*)buf,
                             (uint32_t) (blk_addr),//blk_len*STORAGE_BLK_SIZ
                             1,10000);
//							 printf("HAL_SD_ReadBlocks_DMA ---> %d\n",state);
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
__IO uint32_t time=0xffff;
while(time !=0)
{time--;
}
//while(RX_Flag==0){};
  /* USER CODE BEGIN 13 */
  return (USBD_OK);
  /* USER CODE END 13 */
}

/**
  * @brief  .
  * @param  lun: .
  * @param  buf: .
  * @param  blk_addr: .
  * @param  blk_len: .
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 14 */
//	HAL_SD_WriteBlocks_DMA(&uSdHandle,buf,blk_addr*512,blk_len);
//	if ( SD_write(lun,buf,blk_addr,blk_len) !=0)
//	{
//		printf(" STORAGE_Write_HS Failed! \n");
//	}
//	printf("**********STORAGE_Write_HS*************\n");
//	printf("blk_addr: %d ---- sector:%u ----blk_len:%d\n",blk_addr,blk_len*STORAGE_BLK_SIZ,blk_len);
#if 0
    DRESULT res = RES_ERROR;
    uint32_t timeout=0;

  
    TX_Flag = 0;
	if((DWORD)buf&3)
    {
      DRESULT res = RES_OK;
      DWORD scratch[STORAGE_BLK_SIZ / 4];

      while (blk_len--) 
      {
        memcpy( scratch,buf,STORAGE_BLK_SIZ);
        res = disk_write(0,(void *)scratch, (blk_addr++), 1);
        if (res != RES_OK) 
        {
          break;
        }					
        buf += STORAGE_BLK_SIZ;
      }
	  GUI_DEBUG("1111\n");
      return res;
    }	
//    alignedAddr = (uint32_t)buff & ~0x1F;
//    //更新相应的DCache
//    SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
    if(HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t*)buf,
                             (uint32_t) (blk_addr),//blk_len*STORAGE_BLK_SIZ
                             1) == HAL_OK)
    {
      /* Wait that the reading process is completed or a timeout occurs */
      timeout = HAL_GetTick();
		printf("timeout : %d\n",timeout);
      while((TX_Flag == 0) && ((HAL_GetTick() - timeout) < 0x00100000U))
      {
      }
      /* incase of a timeout return error */
      if (TX_Flag == 0)
      {
		  printf("timeout : %d\n",timeout);
		  GUI_DEBUG("22222\n");
        res = RES_ERROR;
      }
      else
      {
        TX_Flag = 0;
        timeout = HAL_GetTick();

        while((HAL_GetTick() - timeout) < SD_TIMEOUT)
        {
          if (HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER)
          {
            res = RES_OK;
            //使相应的DCache无效
//            SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));

             break;
          }
        }
      }
    }
	GUI_DEBUG("3333\n");
#endif
HAL_StatusTypeDef state;
TX_Flag=0;
UBaseType_t uxSavedInterruptStatus;
uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
state = HAL_SD_WriteBlocks(&uSdHandle, (uint8_t*)buf,
                             (uint32_t) (blk_addr),//blk_len*STORAGE_BLK_SIZ
                             1,10000 );
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
if (state!= 0){
							 printf("HAL_SD_WriteBlocks_DMA ---> %d\n",state);
}
__IO uint32_t time=0xffff;
while(time !=0)
{time--;
}
//while(TX_Flag==0){}
  return (USBD_OK);
  /* USER CODE END 14 */
}

/**
  * @brief  .
  * @param  None
  * @retval .
  */
int8_t STORAGE_GetMaxLun_HS(void)
{
  /* USER CODE BEGIN 15 */
  return (STORAGE_LUN_NBR - 1);
  /* USER CODE END 15 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
