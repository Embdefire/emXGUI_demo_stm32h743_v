/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"
#include "stm32h7xx_it.h"
/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
#include "./sai/bsp_sai.h" 
#include "bsp_mpu_exti.h"
#include "emXGUI.h"
//#include "./JPEG./decode_dma.h"
extern RTC_HandleTypeDef hrtc;
extern SD_HandleTypeDef uSdHandle;
extern volatile uint8_t video_timeout;//视频播放引入
extern __IO uint32_t LocalTime;//以太网提供tick
extern void gyro_data_ready_cb(void);
extern DCMI_HandleTypeDef DCMI_Handle;
extern DMA_HandleTypeDef DMA_Handle_dcmi;
/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles Non maskable interrupt.
*/
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
* @brief This function handles Hard fault interrupt.
*/
//void HardFault_Handler(void)
//{
//  /* USER CODE BEGIN HardFault_IRQn 0 */

//  /* USER CODE END HardFault_IRQn 0 */
//	GUI_ERROR("****************Hard Fault!***************\r\n");
//  while (1)
//  {
//		
//  }
//  /* USER CODE BEGIN HardFault_IRQn 1 */

//  /* USER CODE END HardFault_IRQn 1 */
//}

/**
* @brief This function handles Memory management fault.
*/
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN MemoryManagement_IRQn 1 */

  /* USER CODE END MemoryManagement_IRQn 1 */
}

/**
* @brief This function handles Pre-fetch fault, memory access fault.
*/
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN BusFault_IRQn 1 */

  /* USER CODE END BusFault_IRQn 1 */
}

/**
* @brief This function handles Undefined instruction or illegal state.
*/
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN UsageFault_IRQn 1 */

  /* USER CODE END UsageFault_IRQn 1 */
}




/**
* @brief This function handles Debug monitor.
*/
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

void SDMMC1_IRQHandler(void)
{
//  uint32_t ulReturn;
//  /* 进入临界段，临界段可以嵌套 */
//  ulReturn = taskENTER_CRITICAL_FROM_ISR(); 
  HAL_SD_IRQHandler(&uSdHandle);
		
  /* 退出临界段 */
//  taskEXIT_CRITICAL_FROM_ISR( ulReturn );  
}

/**
* @brief This function handles System tick timer.
*/
extern void xPortSysTickHandler(void);

//systick中断服务函数
void SysTick_Handler(void)
{	
    #if (INCLUDE_xTaskGetSchedulerState  == 1 )
      if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
      {
    #endif  /* INCLUDE_xTaskGetSchedulerState */  
        xPortSysTickHandler();
    #if (INCLUDE_xTaskGetSchedulerState  == 1 )
      }
    #endif  /* INCLUDE_xTaskGetSchedulerState */
}

///* 用于统计运行时间 */

volatile uint32_t CPU_RunTime = 0UL;
extern TIM_HandleTypeDef TIM_Base;
void BASIC_TIM_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM_Base);
}
/**
  * @brief  定时器更新中断回调函数
  * @param  htim : TIM句柄
  * @retval 无
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
      xPortGetFreeHeapSize(); 
      CPU_RunTime++;
    if(htim->Instance == TIM3)
    {
        video_timeout = 1;
//			  LocalTime+=10;
//      LED1_TOGGLE;
    }
}

//void DMA1_Stream2_IRQHandler(void)
//{
////  uint32_t ulReturn;
////  /* 进入临界段，临界段可以嵌套 */
////  ulReturn = taskENTER_CRITICAL_FROM_ISR(); 
//  I2Sx_TX_DMA_STREAM_IRQFUN();
////  taskEXIT_CRITICAL_FROM_ISR( ulReturn );  
//}
void RTC_Alarm_IRQHandler(void)
{
  HAL_RTC_AlarmIRQHandler(&Rtc_Handle);
}

void DMA1_Stream2_IRQHandler(void)
{
  SAI_TX_DMA_STREAM_IRQFUN();
}
void DMA1_Stream3_IRQHandler(void)
{
  SAI_RX_DMA_STREAM_IRQFUN();
}

void EXTI3_IRQHandler(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(MPU_INT_GPIO_PIN) != RESET) //确保是否产生了EXTI Line中断
	{
		/* Handle new gyro*/
		gyro_data_ready_cb();

		__HAL_GPIO_EXTI_CLEAR_IT(MPU_INT_GPIO_PIN);     //清除中断标志位
	}
}

/**
  * @brief  DMA中断服务函数
  * @param  None
  * @retval None
  */
void DMA2_Stream1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&DMA_Handle_dcmi);
  
}

/**
  * @brief  DCMI中断服务函数
  * @param  None
  * @retval None
  */
void DCMI_IRQHandler(void)
{
  HAL_DCMI_IRQHandler(&DCMI_Handle);
  
}

///**
//  * @brief  This function handles JPEG interrupt request.
//  * @param  None
//  * @retval None
//  */

//void JPEG_IRQHandler(void)
//{
////  HAL_JPEG_IRQHandler(&JPEG_Handle);
//}

///**
//  * @brief  This function handles MDMA interrupt request.
//  * @param  None
//  * @retval None
//  */

//void MDMA_IRQHandler()
//{
//  /* Check the interrupt and clear flag */
////  HAL_MDMA_IRQHandler(JPEG_Handle.hdmain);
////  HAL_MDMA_IRQHandler(JPEG_Handle.hdmaout);  
//}
