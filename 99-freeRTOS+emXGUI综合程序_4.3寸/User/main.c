/**
  *********************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   FreeRTOS V9.0.0  + STM32 固件库例程
  *********************************************************************
  * @attention
  *
  * 实验平台:野火 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  **********************************************************************
  */ 
 
/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/ 
/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
/* 开发板硬件bsp头文件 */
#include "board.h"
#include "string.h"
#include <cm_backtrace.h>
#include "./bsp/mpu/bsp_mpu.h" 
#include "diskio.h"
//#include "Backend_RGBLED.h" 

/* hardfault跟踪器需要的定义 */
#define HARDWARE_VERSION               "V1.0.0"
#define SOFTWARE_VERSION               "V0.1.0"

/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */

/********************************** 内核对象句柄 *********************************/
/*
 * 信号量，消息队列，事件标志组，软件定时器这些都属于内核的对象，要想使用这些内核
 * 对象，必须先创建，创建成功之后会返回一个相应的句柄。实际上就是一个指针，后续我
 * 们就可以通过这个句柄操作这些内核对象。
 *
 * 内核对象说白了就是一种全局的数据结构，通过这些数据结构我们可以实现任务间的通信，
 * 任务间的事件同步等各种功能。至于这些功能的实现我们是通过调用这些内核对象的函数
 * 来完成的
 * 
 */


/******************************* 全局变量声明 ************************************/
/*
 * 当我们在写应用程序的时候，可能需要用到一些全局变量。
 */

/*
*************************************************************************
*                             函数声明
*************************************************************************
*/
static void GUI_Thread_Entry(void* pvParameters);/* Test_Task任务实现 */
static void DEBUG_Thread_Entry(void* parameter);
static void MPU_Config(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask,char * pcTaskName);

void BSP_Init(void);/* 用于初始化板载相关资源 */
/***********************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  *********************************************************************/
void BSP_Init(void)
{
//	SCB->CACR|=1<<2;   //强制D-Cache透写,如不开启,实际使用中可能遇到各种问题	  

  /* 系统时钟初始化成400MHz */
#if 0
  /* 设置SDRAM为Normal类型,禁用共享, 直写模式*/  
	Board_MPU_Config(0,MPU_Normal_WT,0xD0000000,MPU_32MB);
	/* 设置AXI RAM为Normal类型,禁用共享, 直写模式*/ 
	Board_MPU_Config(1,MPU_Normal_WT,0x24000000,MPU_512KB);
#endif
	/* 设置SDRAM为Normal类型,禁用共享, 直写模式*/  
	Board_MPU_Config(0,MPU_Normal_WT,0xD0000000,MPU_32MB);
	
	/* 设置AXI RAM为Normal类型,禁用共享, 直写模式*/ 
	Board_MPU_Config(1,MPU_Normal_WT,0x20000000,MPU_128KB);
  Board_MPU_Config(2,MPU_Normal_WT,0x00000000,MPU_64KB);
  Board_MPU_Config(3,MPU_Normal_WT,0x24000000,MPU_512KB);
  Board_MPU_Config(4,MPU_Normal_WT,0x08000000,MPU_2MB);
	
	MPU_Config();	
	
  /* Enable I-Cache */
  SCB_EnableICache(); 
  /* Enable D-Cache */
  SCB_EnableDCache();
	
	
  /*
	 * STM32中断优先级分组为4，即4bit都用来表示抢占优先级，范围为：0~15
	 * 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断，
	 * 都统一用这个优先级分组，千万不要再分组，切忌。
	 */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	/* 硬件BSP初始化统统放在这里，比如LED，串口，LCD等 */
//  SDRAM_Init();
	/* LED 端口初始化 */
	LED_GPIO_Config();	
	
	/* usart 端口初始化 */
  UARTx_Config();
	
  /* 基本定时器初始化	*/
	TIM_Basic_Init();  
	
	/* wm8978 播放器初始化	*/
	if (wm8978_Init()==0)
  {
    printf("检测不到WM8978芯片!!!\n");
  }
	
	RTC_CLK_Config();
	if ( HAL_RTCEx_BKUPRead(&Rtc_Handle,RTC_BKP_DRX) != 0x32F2)
	{
		/* 设置时间和日期 */
		RTC_TimeAndDate_Set();
	}
	else
	{
		/* 检查是否电源复位 */
		if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
		{
			printf("\r\n 发生电源复位....\r\n");
		}
		/* 检查是否外部复位 */
		else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
		{
			printf("\r\n 发生外部复位....\r\n");
		}
		printf("\r\n 不需要重新配置RTC....\r\n");    
		/* 使能 PWR 时钟 */
		__HAL_RCC_RTC_ENABLE();
		/* PWR_CR:DBF置1，使能RTC、RTC备份寄存器和备份SRAM的访问 */
		HAL_PWR_EnableBkUpAccess();
		/* 等待 RTC APB 寄存器同步 */
		HAL_RTC_WaitForSynchro(&Rtc_Handle);
	} 
	
	MODIFY_REG(FMC_Bank1_R->BTCR[0],FMC_BCRx_MBKEN,0); //关闭FMC_Bank1,不然LCD会闪.
	
  /*hardfault 跟踪器初始化*/ 
  cm_backtrace_init("Fire_emxgui", HARDWARE_VERSION, SOFTWARE_VERSION);
  
}


/**
  * @brief  System Clock 配置
  *         system Clock 配置如下: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 480000000 (CPU Clock)
	*            HCLK(Hz)             = 240000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  100MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  100MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  100MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  100MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 192
	*            PLL_P                = 2
	*             PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */

/*****************************************************************
  * @brief  主函数
  * @param  无
  * @retval 无
  * @note   第一步：开发板硬件初始化 
            第二步：创建APP应用任务
            第三步：启动FreeRTOS，开始多任务调度
  ****************************************************************/
int main(void)
{	
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息0返回值，默认为pdPASS */
  
  /* 开发板硬件初始化 */
  BSP_Init();  
  
   /* 创建AppTaskCreate任务 */
  xReturn = xTaskCreate((TaskFunction_t )GUI_Thread_Entry,  /* 任务入口函数 */
                        (const char*    )"gui",/* 任务名字 */
                        (uint16_t       )5*1024,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )10, /* 任务的优先级 */
                        (TaskHandle_t*  )NULL);/* 任务控制块指针 */ 

//	           xTaskCreate((TaskFunction_t )DEBUG_Thread_Entry,  /* 任务入口函数 */
//                        (const char*    )"DEBUG_Thread_Entry",/* 任务名字 */
//                        (uint16_t       )2*1024,  /* 任务栈大小 */
//                        (void*          )NULL,/* 任务入口函数参数 */
//                        (UBaseType_t    )2, /* 任务的优先级 */
//                        (TaskHandle_t*  )NULL);/* 任务控制块指针 */ 
  /* 启动任务调度 */           
  if(pdPASS == xReturn)
    vTaskStartScheduler();   /* 启动任务，开启调度 */
  else
    return -1;  
  
  while(1);   /* 正常不会执行到这里 */    
}

extern void GUI_Startup(void);

/**********************************************************************
  * @ 函数名  ： gui_thread_entry
  * @ 功能说明： gui_thread_entry任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void GUI_Thread_Entry(void* parameter)
{	
//  uint8_t CPU_RunInfo[400];		//保存任务运行时间信息
  printf("野火emXGUI演示例程\n\n");
  /* 执行本函数不会返回 */
	
	GUI_Startup();

  while (1)
  {
    LED1_ON;
    printf("Test_Task Running,LED1_ON\r\n");
    vTaskDelay(500);   /* 延时500个tick */
    
    LED1_OFF;     
    printf("Test_Task Running,LED1_OFF\r\n");
    vTaskDelay(500);   /* 延时500个tick */
  }
}

static void DEBUG_Thread_Entry(void* parameter)
{	
	char tasks_buf[512] = {0};
	
  while (1)
  {

    vTaskDelay(5000);   /* 延时500个tick */
{
	memset(tasks_buf, 0, 512);

	strcat((char *)tasks_buf, "任务名称    运行计数    使用率\r\n" );

	strcat((char *)tasks_buf, "---------------------------------------------\r\n");

	/* displays the amount of time each task has spent in the Running state

	* in both absolute and percentage terms. */

	vTaskGetRunTimeStats((char *)(tasks_buf + strlen(tasks_buf)));

	strcat((char *)tasks_buf, "\r\n");
	printf("%s\r\n",tasks_buf);
	
}
	memset(tasks_buf, 0, 512);

	strcat((char *)tasks_buf, "任务名称    运行状态    优先级    剩余堆栈    任务序号\r\n" );

	strcat((char *)tasks_buf, "---------------------------------------------\r\n");


{
	vTaskList((char *)(tasks_buf + strlen(tasks_buf)));

	strcat((char *)tasks_buf, "\r\n---------------------------------------------\r\n");


	strcat((char *)tasks_buf, "B : 阻塞, R : 就绪, D : 删除, S : 暂停\r\n");
	printf("%s\r\n",tasks_buf);
}
  }
}

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as Device not cacheable 
     for ETH DMA descriptors */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Cacheable write through 
     for LwIP RAM heap which contains the Tx buffers */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30044000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                     char * pcTaskName)
{
	printf(" Stack Overflow! Check Task: %s \r\n",pcTaskName);
	while(1);
}

/********************************END OF FILE****************************/
