#include "user_usb_init.h"

int usb_clockconfig(void)
{
#if 0
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		printf("Set USB clock Faild \n");
		return 0;
	}
	/** Enable USB Voltage detector 
	*/
#endif
	HAL_PWREx_EnableUSBVoltageDetector();
	MX_GPIO_Init();
	return 1;
}

static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

