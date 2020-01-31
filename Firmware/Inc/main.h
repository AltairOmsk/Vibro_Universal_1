/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define NO_USE_ESP_LOG
//#define USE_ESP_LOG  
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define HARDWARE_REV "Rev.1.0 13.06.2019"
#define FIRMWARE_REV "Rev.1.0 10.07.2019"   
  
  
  
#define UART1_RX_SIZE	512
#define UART2_RX_SIZE	1024
#define UART1_TX_SIZE	512
#define UART2_TX_SIZE	1024
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_PWR_Pin GPIO_PIN_2
#define LED_PWR_GPIO_Port GPIOE
#define LED_NET_Pin GPIO_PIN_3
#define LED_NET_GPIO_Port GPIOE
#define LED_SRVR_Pin GPIO_PIN_4
#define LED_SRVR_GPIO_Port GPIOE
#define LED_ANSW_Pin GPIO_PIN_5
#define LED_ANSW_GPIO_Port GPIOE
#define POWER_SENS_Pin GPIO_PIN_0
#define POWER_SENS_GPIO_Port GPIOC
#define CURRENT_B_Pin GPIO_PIN_1
#define CURRENT_B_GPIO_Port GPIOC
#define CURRENT_C_Pin GPIO_PIN_2
#define CURRENT_C_GPIO_Port GPIOC
#define CURRENT_A_Pin GPIO_PIN_3
#define CURRENT_A_GPIO_Port GPIOC
#define VOLT_B_Pin GPIO_PIN_0
#define VOLT_B_GPIO_Port GPIOA
#define VOLT_C_Pin GPIO_PIN_1
#define VOLT_C_GPIO_Port GPIOA
#define VOLT_A_Pin GPIO_PIN_2
#define VOLT_A_GPIO_Port GPIOA
#define AIN_0_Pin GPIO_PIN_3
#define AIN_0_GPIO_Port GPIOA
#define DAC_1_Pin GPIO_PIN_4
#define DAC_1_GPIO_Port GPIOA
#define DAC_2_Pin GPIO_PIN_5
#define DAC_2_GPIO_Port GPIOA
#define AIN_1_Pin GPIO_PIN_6
#define AIN_1_GPIO_Port GPIOA
#define AIN_2_Pin GPIO_PIN_7
#define AIN_2_GPIO_Port GPIOA
#define AIN_3_Pin GPIO_PIN_4
#define AIN_3_GPIO_Port GPIOC
#define AIN_4_Pin GPIO_PIN_5
#define AIN_4_GPIO_Port GPIOC
#define AIN_5_Pin GPIO_PIN_0
#define AIN_5_GPIO_Port GPIOB
#define AIN_6_Pin GPIO_PIN_1
#define AIN_6_GPIO_Port GPIOB
#define DOUT_0_Pin GPIO_PIN_2
#define DOUT_0_GPIO_Port GPIOB
#define DOUT_1_Pin GPIO_PIN_7
#define DOUT_1_GPIO_Port GPIOE
#define DOUT_2_Pin GPIO_PIN_8
#define DOUT_2_GPIO_Port GPIOE
#define SPEED_1_Pin GPIO_PIN_9
#define SPEED_1_GPIO_Port GPIOE
#define DOUT_3_Pin GPIO_PIN_10
#define DOUT_3_GPIO_Port GPIOE
#define SPEED_2_Pin GPIO_PIN_11
#define SPEED_2_GPIO_Port GPIOE
#define DOUT_4_Pin GPIO_PIN_12
#define DOUT_4_GPIO_Port GPIOE
#define CODEC_RESET_Pin GPIO_PIN_15
#define CODEC_RESET_GPIO_Port GPIOE
#define DOUT_5_Pin GPIO_PIN_11
#define DOUT_5_GPIO_Port GPIOB
#define DOUT_6_Pin GPIO_PIN_13
#define DOUT_6_GPIO_Port GPIOB
#define RS485_TX_Pin GPIO_PIN_8
#define RS485_TX_GPIO_Port GPIOD
#define RS485_RX_Pin GPIO_PIN_9
#define RS485_RX_GPIO_Port GPIOD
#define DIN_0_Pin GPIO_PIN_10
#define DIN_0_GPIO_Port GPIOD
#define DIN_1_Pin GPIO_PIN_11
#define DIN_1_GPIO_Port GPIOD
#define DIN_2_Pin GPIO_PIN_12
#define DIN_2_GPIO_Port GPIOD
#define DIN_3_Pin GPIO_PIN_13
#define DIN_3_GPIO_Port GPIOD
#define DIN_4_Pin GPIO_PIN_14
#define DIN_4_GPIO_Port GPIOD
#define DIN_5_Pin GPIO_PIN_15
#define DIN_5_GPIO_Port GPIOD
#define DIN_6_Pin GPIO_PIN_7
#define DIN_6_GPIO_Port GPIOC
#define DIN_7_Pin GPIO_PIN_8
#define DIN_7_GPIO_Port GPIOC
#define BT_Reset_Pin GPIO_PIN_11
#define BT_Reset_GPIO_Port GPIOA
#define RS485_DE_Pin GPIO_PIN_12
#define RS485_DE_GPIO_Port GPIOA
#define SPI3_CS1_Pin GPIO_PIN_15
#define SPI3_CS1_GPIO_Port GPIOA
#define SPI3_CS2_Pin GPIO_PIN_0
#define SPI3_CS2_GPIO_Port GPIOD
#define WiFi_Reset_Pin GPIO_PIN_7
#define WiFi_Reset_GPIO_Port GPIOD
#define BTN_SEND_Pin GPIO_PIN_1
#define BTN_SEND_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */


#define DEBUG(x) (HAL_UART_Transmit(&huart1, (uint8_t*)(x), strlen(x), 1000))




/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
