/**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   SDMMC��SD����д����
  ******************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32H743������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************
  */  
#include "stm32h7xx.h"
#include "main.h"
#include "./led/bsp_led.h"
#include "./usart/bsp_debug_usart.h"
#include "./sdmmc/bsp_sdmmc_sd.h"
#include "./key/bsp_key.h"
/* FatFs includes component */
#include "ff.h"
#include "ff_gen_drv.h"
#include "./fatfs/drivers/fatfs_flash_qspi.h"
#include "aux_data.h"
/**
  ******************************************************************************
  *                              �������
  ******************************************************************************
  */
char SDPath[4]; /* SD�߼�������·�� */
extern FATFS sd_fs;	
FRESULT res_sd;                /* �ļ�������� */
uint8_t SDworkBuffer[_MAX_SS];
static void SystemClock_Config(void);
extern FATFS flash_fs;
extern Diskio_drvTypeDef  SD_Driver;
//Ҫ���Ƶ��ļ�·������aux_data.c�޸�
extern char src_dir[];
extern char dst_dir[];

/**
	**************************************************************
	* Description : ��ʼ��WiFiģ��ʹ�����ţ�������WiFiģ��
	* Argument(s) : none.
	* Return(s)   : none.
	**************************************************************
	*/
static void WIFI_PDN_INIT(void)
{
	/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	GPIO_InitTypeDef GPIO_InitStruct;
	/*ʹ������ʱ��*/	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	/*ѡ��Ҫ���Ƶ�GPIO����*/															   
	GPIO_InitStruct.Pin = GPIO_PIN_2;	
	/*�������ŵ��������Ϊ�������*/
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;      
	/*��������Ϊ����ģʽ*/
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	/*������������Ϊ���� */   
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
	/*���ÿ⺯����ʹ���������õ�GPIO_InitStructure��ʼ��GPIO*/
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);	
	/*����WiFiģ��*/
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_2,GPIO_PIN_RESET);  
}
/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	MPU_Config();
	/* ʹ��ָ��� */
	SCB_EnableICache();
  /* ʹ�����ݻ��� */
  SCB_EnableDCache();
	/* ϵͳʱ�ӳ�ʼ����400MHz */
	SystemClock_Config();
	/*����WiFiģ��*/
	WIFI_PDN_INIT();	
	/* LED �˿ڳ�ʼ�� */
	LED_GPIO_Config();	
	LED_BLUE;
	
	/* ���ô���1Ϊ��115200 8-N-1 */
	DEBUG_USART_Config();
	/* ��ʼ���������� */
	Key_GPIO_Config();
	printf("****** ����һ��SD���ļ�ϵͳʵ�� ******\r\n");

	//���ⲿSPI Flash�����ļ�ϵͳ���ļ�ϵͳ����ʱ���SPI�豸��ʼ��
	TM_FATFS_FLASH_SPI_disk_initialize(NULL);

	//�����������������̷�
	FATFS_LinkDriver(&SD_Driver, SDPath);
	//f_mkfs(SDPath, FM_ANY, 0, SDworkBuffer, sizeof(SDworkBuffer));		
	res_sd = f_mount(&sd_fs,(TCHAR const*)SDPath,1);
  //����ļ�ϵͳ����ʧ�ܾ��˳�
  if(res_sd != FR_OK)
  {
    BURN_ERROR("f_mount ERROR!������������SD��Ȼ�����¸�λ������!");
    LED_RED;
    while(1);
  }    
    
  printf("\r\n ��һ��KEY1��ʼ��д�ֿⲢ�����ļ���FLASH�� \r\n"); 
  printf("\r\n ע��ò������FLASH��ԭ���ݻᱻɾ������ \r\n"); 

  while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==0){};
  while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==1){}; 

    
      //     for(j=0;j < dat[i].length/4096 ;j++)//������������ʼλ��710*4096��2116KB
      // {
      //   BSP_QSPI_Erase_Block(write_addr+j*4096);
      // }
      printf("\r\n ���ڲ�����ƬFLASH�������ĵȺ� \r\n"); 

      // BSP_QSPI_Erase_Chip();
  //��¼���ݵ�flash�ķ��ļ�ϵͳ����    
  res_sd = burn_file_sd2flash(burn_data,AUX_MAX_NUM); 
#if 0
  if(res_sd == FR_OK)
  {
    printf("\r\n\r\n\r\n"); 

    //�����ļ���FLASH���ļ�ϵͳ����
    copy_file_sd2flash(src_dir,dst_dir);
      
    if(res_sd == FR_OK)
    {
      printf("\r\n ���������ѳɹ����Ƶ�FLASH������ \r\n");  
      LED_GREEN;
    }
    else
    {
      printf("\r\n �����ļ���FLASHʧ��(�ļ�ϵͳ����)���븴λ���ԣ��� \r\n"); 
    }
  }
  else
  {
    printf("\r\n �������ݵ�FLASHʧ��(���ļ�ϵͳ����)���븴λ���ԣ��� \r\n"); 
  }
 #endif 
  while(1)
	{
			
	}
}

/**
  * @brief  System Clock ����
  *         system Clock ��������: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 400000000 (CPU Clock)
	*            HCLK(Hz)             = 200000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  100MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  100MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  100MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  100MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 160
	*            PLL_P                = 2
	*            PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*ʹ�ܹ������ø��� */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* ��������ʱ��Ƶ�ʵ������ϵͳƵ��ʱ����ѹ���ڿ����Ż����ģ�
		 ����ϵͳƵ�ʵĵ�ѹ����ֵ�ĸ��¿��Բο���Ʒ�����ֲᡣ  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* ����HSE������ʹ��HSE��ΪԴ����PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
	/* ѡ��PLL��Ϊϵͳʱ��Դ����������ʱ�ӷ�Ƶ�� */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK  | \
																 RCC_CLOCKTYPE_HCLK    | \
																 RCC_CLOCKTYPE_D1PCLK1 | \
																 RCC_CLOCKTYPE_PCLK1   | \
                                 RCC_CLOCKTYPE_PCLK2   | \
																 RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }

}
/****************************END OF FILE***************************/
