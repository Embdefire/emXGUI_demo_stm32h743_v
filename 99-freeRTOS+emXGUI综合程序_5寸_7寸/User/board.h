#ifndef __BOARD_H__
#define __BOARD_H__

/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/
/* STM32 固件库头文件 */
#include "stm32h7xx.h"

/* 开发板硬件bsp头文件 */
#include "./led/bsp_led.h" 
//#include "./key/bsp_key.h" 
#include "./lcd/bsp_lcd.h"
#include "./sdram/bsp_sdram.h" 
#include "./touch/bsp_i2c_touch.h"
#include "./touch/bsp_touch_gtxx.h"
#include "./usart/bsp_usart.h"
//#include "./flash/bsp_spi_flash.h"
//#include "./font/fonts.h"
#include "./wm8978/bsp_wm8978.h"  
#include "./tim/bsp_basic_tim.h"
#include "./beep/bsp_beep.h"
#include "./Phone_SMS/bsp_sim/bsp_gsm_gprs.h"
#include "./RTC/bsp_rtc.h"
#include "./adc_Collect_voltage/bsp_adc.h"
/*
*************************************************************************
*                               函数声明
*************************************************************************
*/
	

#endif /* __BOARD_H__ */
