/**
  ******************************************************************************
  * @file    bsp_sdio_sd.c
  * @author  fire
  * @version V1.0
  * @date    2017-xx-xx
  * @brief   SDIO-SD����
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:����  STM32 H743 ������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */ 
#include "./sdmmc/bsp_sdmmc_sd.h"

SD_HandleTypeDef uSdHandle;

#define SD_TIMEOUT 30 * 1000

/**
  * @brief  ����MPU����
  * @note   �������洢���ӿ���AXI����ô����ַ��0x30040000
  *         ���������СΪ32KB����Ϊ��D2SRAM3��С��ͬ
  * @note   SDIO�ڲ�DMA�ɷ��ʵ�SRAM1������ַΪ0x24000000��
            ���������СΪ512KB
  * @param  ��
  * @retval ��
  */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* ����MPU */
  HAL_MPU_Disable();

  /* ��MPU��������ΪSRAM1��WT */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* ʹ��MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  
}

/**
  * @brief  ��ʼ��SD���豸
  * @retval SD��״̬
  */
uint8_t BSP_SD_Init(void)
{ 
  uint8_t sd_state = HAL_OK;
  
  /* ����SDMMC��� */
  uSdHandle.Instance = SDMMC1;
  HAL_SD_DeInit(&uSdHandle);
	/* SDMMC�ں�ʱ��200Mhz, SDCardʱ��25Mhz  */
  uSdHandle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
  uSdHandle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  uSdHandle.Init.BusWide             = SDMMC_BUS_WIDE_4B;
  uSdHandle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  uSdHandle.Init.ClockDiv            = 4;
  
  /* ��ʼ��SD�ײ����� */
  BSP_SD_MspInit(&uSdHandle, NULL);

  /* HAL SD ��ʼ�� */
  if(HAL_SD_Init(&uSdHandle) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }
  return  sd_state;
}

/**
  * @brief  ȡ����ʼ��SD���豸
  * @retval SD״̬
  */
uint8_t BSP_SD_DeInit(void)
{ 
  uint8_t sd_state = HAL_OK;
 
  uSdHandle.Instance = SDMMC1;
  
  /* ȡ����ʼ��SD���豸 */
  if(HAL_SD_DeInit(&uSdHandle) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  /* ȡ����ʼ��SD�ײ����� */
  uSdHandle.Instance = SDMMC1;
  BSP_SD_MspDeInit(&uSdHandle, NULL);
  
  return  sd_state;
}

/**
  * @brief  ����ѯģʽ�´�SD���е�ָ����ַ��ȡ��
  * @param  pData: ָ�򽫰���Ҫ��������ݵĻ�������ָ��
  * @param  ReadAddr: �������ȡ���ݵĵ�ַ  
  * @param  NumOfBlocks: ��ȡ�������� 
  * @retval SD ״̬
  */
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint64_t ReadAddr, uint32_t NumOfBlocks)
{
  if(HAL_SD_ReadBlocks(&uSdHandle,(uint8_t *)pData, ReadAddr,NumOfBlocks,SD_TIMEOUT) == HAL_OK)
  {
    return HAL_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
  * @brief  ����ѯģʽ�´�SD���е�ָ����ַд��� 
  * @param  pData: ָ�򽫰���Ҫ��������ݵĻ�������ָ��
  * @param  WriteAddr: �������ȡ���ݵĵ�ַ  
  * @param  NumOfBlocks: ��ȡ�������� 
  * @retval SD ״̬
  */
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint64_t WriteAddr,uint32_t NumOfBlocks)
{
  if(HAL_SD_WriteBlocks(&uSdHandle,(uint8_t *)pData, WriteAddr,NumOfBlocks,SD_TIMEOUT) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
  * @brief  ��DMA��ʽ��SD���е�ָ����ַ��ȡ��
  * @param  pData: ָ�򽫰���Ҫ��������ݵĻ�������ָ��
  * @param  ReadAddr: �������ȡ���ݵĵ�ַ  
  * @param  NumOfBlocks: ��ȡ�������� 
  * @retval SD ״̬
  */
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint64_t ReadAddr, uint32_t NumOfBlocks)
{ 
  /* ��DMA��ʽ��SD���е�ָ����ַ��ȡ�� */
  if(HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t *)pData, ReadAddr,NumOfBlocks) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
  * @brief  ��DMA��ʽ��SD���е�ָ����ַд���
  * @param  pData: ָ�򽫰���Ҫ��������ݵĻ�������ָ��
  * @param  ReadAddr: ������д�����ݵĵ�ַ  
  * @param  NumOfBlocks: ��ȡ�������� 
  * @retval SD ״̬
  */
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint64_t WriteAddr, uint32_t NumOfBlocks)
{  
  /* ��DMA��ʽ��SD���е�ָ����ַд��� */
  if(HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
  * @brief  ��������SD����ָ���洢���� 
  * @param  StartAddr: ��ʼ��ַ
  * @param  EndAddr: ������ַ
  * @retval SD ״̬
  */
uint8_t BSP_SD_Erase(uint64_t StartAddr, uint64_t EndAddr)
{
  if(HAL_SD_Erase(&uSdHandle, StartAddr, EndAddr) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
  * @brief  ��ʼ��SD����
  * @param  hsd: SD ���
  * @param  Params
  * @retval None
  */
void BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* ʹ�� SDMMC ʱ�� */
  __HAL_RCC_SDMMC1_CLK_ENABLE();

  /* ʹ�� GPIOs ʱ�� */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* ����GPIO�������졢����������ģʽ */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Alternate = GPIO_AF12_SDIO1;
  
  /* GPIOC ���� */
  gpio_init_structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOC, &gpio_init_structure);

  /* GPIOD ���� */
  gpio_init_structure.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);
	
  __HAL_RCC_SDMMC1_FORCE_RESET();
  __HAL_RCC_SDMMC1_RELEASE_RESET();
  /* SDIO �ж����� */
  HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    
}

/**
  * @brief  ȡ����ʼ��SD�ײ�����
  * @param  hsd: SD handle
  * @param  Params
  * @retval None
  */
void BSP_SD_MspDeInit(SD_HandleTypeDef *hsd, void *Params)
{
  /* ����SDIC�ж�*/
  HAL_NVIC_DisableIRQ(SDMMC1_IRQn);

  /* ����SDMMC1ʱ�� */
  __HAL_RCC_SDMMC1_CLK_DISABLE();

}

/**
  * @brief  ����SD���ж�����
  * @retval ��
  */
void BSP_SD_IRQHandler(void)
{
  HAL_SD_IRQHandler(&uSdHandle);
}

/**
  * @brief  Gets the current SD card data status.
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
uint8_t BSP_SD_GetStatus(void)
{
  return((HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  ��ȡ�й��ض�SD����SD��Ϣ
  * @param  CardInfo: ָ��HAL_SD_CardInfoTypedef�ṹ���ָ��
  * @retval �� 
  */
void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* ��ȡSD��Ϣ */
  HAL_SD_GetCardInfo(&uSdHandle, CardInfo);
}


