/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "work.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "esp.h"
#include "codec.h"
#include "com_control.h"
#include "ssd1306.h"
#include "fonts.h"
  
   
extern          MEAS_t M;



/*
Для того, что бы срабатывал колбэк для прерывания полного завершения передачи по ДМА
В файле stm32f4xx_hal_i2s_ex.c
В функции static void I2SEx_TxRxDMACplt(DMA_HandleTypeDef *hdma) добавить
вызов HAL_I2SEx_TxRxCpltCallback(hi2s);
*/

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;

DAC_HandleTypeDef hdac;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_i2s2_ext_rx;
DMA_HandleTypeDef hdma_spi2_tx;

SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim9;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart2_tx;

osThreadId defaultTaskHandle;
osThreadId ADC1_Data_RoutiHandle;
osThreadId ADC2_50Hz_RoutiHandle;
osThreadId CODEC_ADC_RoutiHandle;
osThreadId WIFI_SEND_RoutiHandle;
osThreadId OLED_OutHandle;
osSemaphoreId BinarySem_ADC1_ReadyHandle;
osSemaphoreId BinarySem_ADC2_50Hz_ReadyHandle;
osSemaphoreId BinarySem_Codec_ADC_ReadyHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_I2S2_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM5_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM9_Init(void);
void StartDefaultTask(void const * argument);
void Start_ADC1_Data_Routine(void const * argument);
void Start_ADC2_50Hz_Routine(void const * argument);
void Start_CODEC_ADC_Routine(void const * argument);
void Start_WIFI_SEND_Routine(void const * argument);
void Start_OLED_Out(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern int16_t  Codec_Rx[];                                                     // Временный буфер аудиокодека                                                          
extern int16_t  Codec_Tx[];

#define ADC1_BUF_LEN (ADC1_CH_CNT * 40 * 2)                                     // 11 каналов по 40 отсчетов 2 полубуфера = 1600 байт
#define ADC2_BUF_LEN (ADC2_CH_CNT * 40 * 2)                                     // 6 каналов пo 40 отсчетов 2 полубуфера = 1120 байт

uint16_t        ADC1_buf[ADC1_BUF_LEN];                                         // Рабочие буфера АЦП контроллера                            
uint16_t        ADC2_buf[ADC2_BUF_LEN]; 

extern char     TmpStr[];                                                       // Строка для вского

//******************************************************************************
//   ADC 1 DC
//******************************************************************************
void ADC1_DMA_XferHalfCpltCallback ( struct __DMA_HandleTypeDef * hdma){
  R.ADC1_Start_Point =  ADC1_buf;
  osSemaphoreRelease(BinarySem_ADC1_ReadyHandle);                               //Выдем семафор о принятии в буфер сообщения
}

void ADC1_DMA_XferCpltCallback ( struct __DMA_HandleTypeDef * hdma){
  R.ADC1_Start_Point = ADC1_buf + ADC1_BUF_LEN/2;
  osSemaphoreRelease(BinarySem_ADC1_ReadyHandle);                               //Выдем семафор о принятии в буфер сообщения
}

//******************************************************************************
//   ADC 2 50 Hz
//******************************************************************************
void ADC2_DMA_XferHalfCpltCallback ( struct __DMA_HandleTypeDef * hdma){
  R.ADC2_Start_Point =  ADC2_buf;
  osSemaphoreRelease(BinarySem_ADC2_50Hz_ReadyHandle);                          //Выдем семафор о принятии в буфер сообщения
}

void ADC2_DMA_XferCpltCallback ( struct __DMA_HandleTypeDef * hdma){
  R.ADC2_Start_Point = ADC2_buf + ADC2_BUF_LEN/2;
  osSemaphoreRelease(BinarySem_ADC2_50Hz_ReadyHandle);                          //Выдем семафор о принятии в буфер сообщения
}

void USART1_UART_Init()
{
	MX_USART1_UART_Init();
}

void USART2_UART_Init()
{
	MX_USART2_UART_Init();
}
void UART2_UART_InitBaud()
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 230400;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
	Error_Handler();
	}
}

extern HAL_TickFreqTypeDef uwTickFreq;
extern __IO uint32_t uwTick;
volatile uint32_t EspTimer = 0;
volatile uint32_t EspSendTimer = 0;

void HAL_IncTick(void)
{
static uint16_t Tick;

	uwTick += uwTickFreq;
	if (EspTimer != 0)
		--EspTimer;
	if (EspSendTimer != 0)
		--EspSendTimer;
        if (++Tick == 1000) Tick=0, M.UpTime++;
}


void HAL_Delay(uint32_t Delay)
{
	uint32_t tickstart = HAL_GetTick();
	uint32_t wait = Delay;

	/* Add a freq to guarantee minimum wait */
	if (wait < HAL_MAX_DELAY)
		wait += (uint32_t)(uwTickFreq);

	while ((HAL_GetTick() - tickstart) < wait)
	{
		__WFI();
	}
}


void ESP_HW_Reset (void) {
        __WIFI_RESET  (0);
        HAL_Delay(20);
        __WIFI_RESET  (1);
        HAL_Delay(200);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
//  MX_GPIO_Init();
//  MX_DMA_Init();
//  MX_ADC1_Init();
//  MX_DAC_Init();
//  MX_I2C1_Init();
//  MX_I2C3_Init();
//  MX_I2S2_Init();
//  MX_SPI3_Init();
//  MX_TIM1_Init();
//  MX_USART3_UART_Init();
//  MX_TIM5_Init();
//  MX_ADC2_Init();
//  
//  
//  
//  
//  
//#if 0
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_I2S2_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM5_Init();
  MX_ADC2_Init();
  MX_TIM9_Init();
  /* USER CODE BEGIN 2 */
//#endif 
  
        board_init();
  
        ESP_HW_Reset();

  	UART1_ReInit();
	UART2_ReInit();
        
	DEBUG("\nCONCENTRATOR \"UNIVERSAL-1\"\n");
        

  
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of BinarySem_ADC1_Ready */
  osSemaphoreDef(BinarySem_ADC1_Ready);
  BinarySem_ADC1_ReadyHandle = osSemaphoreCreate(osSemaphore(BinarySem_ADC1_Ready), 1);

  /* definition and creation of BinarySem_ADC2_50Hz_Ready */
  osSemaphoreDef(BinarySem_ADC2_50Hz_Ready);
  BinarySem_ADC2_50Hz_ReadyHandle = osSemaphoreCreate(osSemaphore(BinarySem_ADC2_50Hz_Ready), 1);

  /* definition and creation of BinarySem_Codec_ADC_Ready */
  osSemaphoreDef(BinarySem_Codec_ADC_Ready);
  BinarySem_Codec_ADC_ReadyHandle = osSemaphoreCreate(osSemaphore(BinarySem_Codec_ADC_Ready), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of ADC1_Data_Routi */
  osThreadDef(ADC1_Data_Routi, Start_ADC1_Data_Routine, osPriorityAboveNormal, 0, 128);
  ADC1_Data_RoutiHandle = osThreadCreate(osThread(ADC1_Data_Routi), NULL);

  /* definition and creation of ADC2_50Hz_Routi */
  osThreadDef(ADC2_50Hz_Routi, Start_ADC2_50Hz_Routine, osPriorityAboveNormal, 0, 128);
  ADC2_50Hz_RoutiHandle = osThreadCreate(osThread(ADC2_50Hz_Routi), NULL);

  /* definition and creation of CODEC_ADC_Routi */
  osThreadDef(CODEC_ADC_Routi, Start_CODEC_ADC_Routine, osPriorityAboveNormal, 0, 128);
  CODEC_ADC_RoutiHandle = osThreadCreate(osThread(CODEC_ADC_Routi), NULL);

  /* definition and creation of WIFI_SEND_Routi */
  osThreadDef(WIFI_SEND_Routi, Start_WIFI_SEND_Routine, osPriorityNormal, 0, 256);
  WIFI_SEND_RoutiHandle = osThreadCreate(osThread(WIFI_SEND_Routi), NULL);

  /* definition and creation of OLED_Out */
  osThreadDef(OLED_Out, Start_OLED_Out, osPriorityNormal, 0, 256);
  OLED_OutHandle = osThreadCreate(osThread(OLED_Out), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 50;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 11;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 8;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 9;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_VBAT;
  sConfig.Rank = 10;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 11;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = ENABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = ENABLE;
  hadc2.Init.NbrOfDiscConversion = 6;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc2.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T5_CC1;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 6;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
  */
  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = 6;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */
  /** DAC Initialization 
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config 
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT2 config 
  */
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 10500;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 100;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */
//  TIM_CCxChannelCmd(htim5.Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);
//  HAL_TIM_GenerateEvent(&htim5, TIM_EVENTSOURCE_CC1);
  //HAL_TIM_OC_Start(&htim5, TIM_CHANNEL_1);
  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 0;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 0;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_PWR_Pin|LED_NET_Pin|LED_SRVR_Pin|LED_ANSW_Pin 
                          |DOUT_1_Pin|DOUT_2_Pin|DOUT_3_Pin|DOUT_4_Pin 
                          |CODEC_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DOUT_0_Pin|DOUT_5_Pin|DOUT_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, BT_Reset_Pin|RS485_DE_Pin|SPI3_CS1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SPI3_CS2_Pin|WiFi_Reset_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_PWR_Pin LED_NET_Pin LED_SRVR_Pin LED_ANSW_Pin 
                           DOUT_1_Pin DOUT_2_Pin DOUT_3_Pin DOUT_4_Pin 
                           CODEC_RESET_Pin */
  GPIO_InitStruct.Pin = LED_PWR_Pin|LED_NET_Pin|LED_SRVR_Pin|LED_ANSW_Pin 
                          |DOUT_1_Pin|DOUT_2_Pin|DOUT_3_Pin|DOUT_4_Pin 
                          |CODEC_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : DOUT_0_Pin DOUT_5_Pin DOUT_6_Pin */
  GPIO_InitStruct.Pin = DOUT_0_Pin|DOUT_5_Pin|DOUT_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DIN_0_Pin DIN_1_Pin DIN_2_Pin DIN_3_Pin 
                           DIN_4_Pin DIN_5_Pin */
  GPIO_InitStruct.Pin = DIN_0_Pin|DIN_1_Pin|DIN_2_Pin|DIN_3_Pin 
                          |DIN_4_Pin|DIN_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : DIN_6_Pin DIN_7_Pin */
  GPIO_InitStruct.Pin = DIN_6_Pin|DIN_7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BT_Reset_Pin RS485_DE_Pin SPI3_CS1_Pin */
  GPIO_InitStruct.Pin = BT_Reset_Pin|RS485_DE_Pin|SPI3_CS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI3_CS2_Pin WiFi_Reset_Pin */
  GPIO_InitStruct.Pin = SPI3_CS2_Pin|WiFi_Reset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_SEND_Pin */
  GPIO_InitStruct.Pin = BTN_SEND_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_SEND_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
                                               
  
  CODEC_Init();
  
  TIM_CCxChannelCmd(htim5.Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);
  HAL_TIM_GenerateEvent(&htim5, TIM_EVENTSOURCE_CC1);
  HAL_TIM_Base_Start_IT(&htim5);                                                // 8 kHz for ADC
  
  /* Infinite loop */
  for(;;)
  {
    BT_Processing();
    meas_and_send();
    //char G[100];
    //sprintf(G,"V A=%.3f B=%.3f C=%.3f\r\n", M.Volt_Phase_A, M.Volt_Phase_B, M.Volt_Phase_C); M.VrefINT
    //sprintf(G,"Bat=%.2f Pwr=%.2f AIN0=%.3fV AIN1=%.3f\r\n", M.Vbat, M.Power_Sense, M.AIN_0, M.AIN_1);
    //DEBUG(G);
  
    osDelay(1);
  }
  /* USER CODE END 5 */ 
}

/* USER CODE BEGIN Header_Start_ADC1_Data_Routine */
/**
* @brief Function implementing the ADC1_Data_Routi thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_ADC1_Data_Routine */
void Start_ADC1_Data_Routine(void const * argument)
{
  /* USER CODE BEGIN Start_ADC1_Data_Routine */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC1_buf, ADC1_BUF_LEN);               // Запуск АЦП
    hdma_adc1.XferCpltCallback =     ADC1_DMA_XferCpltCallback;
    hdma_adc1.XferHalfCpltCallback = ADC1_DMA_XferHalfCpltCallback;
  /* Infinite loop */
  for(;;)
  {
    osSemaphoreWait(BinarySem_ADC1_ReadyHandle , osWaitForever);
    meas_AINx_DMA_Int_routine(R.ADC1_Start_Point);
  }
  /* USER CODE END Start_ADC1_Data_Routine */
}

/* USER CODE BEGIN Header_Start_ADC2_50Hz_Routine */
/**
* @brief Function implementing the ADC2_50Hz_Routi thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_ADC2_50Hz_Routine */
void Start_ADC2_50Hz_Routine(void const * argument)
{
  /* USER CODE BEGIN Start_ADC2_50Hz_Routine */
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)ADC2_buf, ADC2_BUF_LEN);
    hdma_adc2.XferCpltCallback =     ADC2_DMA_XferCpltCallback;
    hdma_adc2.XferHalfCpltCallback = ADC2_DMA_XferHalfCpltCallback;
  /* Infinite loop */
  for(;;)
  {
      osSemaphoreWait(BinarySem_ADC2_50Hz_ReadyHandle , osWaitForever);
      meas_50Hz_DMA_Int_routine(R.ADC2_Start_Point);
  }
  /* USER CODE END Start_ADC2_50Hz_Routine */
}

/* USER CODE BEGIN Header_Start_CODEC_ADC_Routine */
/**
* @brief Function implementing the CODEC_ADC_Routi thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_CODEC_ADC_Routine */
void Start_CODEC_ADC_Routine(void const * argument)
{
  /* USER CODE BEGIN Start_CODEC_ADC_Routine */
  // Запуск кодека 
  HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)Codec_Tx, (uint16_t*)Codec_Rx, 1000);
  __HAL_DMA_DISABLE_IT(&hdma_spi2_tx, DMA_IT_TC);
  __HAL_DMA_DISABLE_IT(&hdma_spi2_tx, DMA_IT_HT);
  
  /* Infinite loop */
  for(;;)
  {
    osSemaphoreWait(BinarySem_Codec_ADC_ReadyHandle , osWaitForever);
    codec_IT_routine();

  }
  /* USER CODE END Start_CODEC_ADC_Routine */
}

/* USER CODE BEGIN Header_Start_WIFI_SEND_Routine */
/**
* @brief Function implementing the WIFI_SEND_Routi thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_WIFI_SEND_Routine */
void Start_WIFI_SEND_Routine(void const * argument)
{
  /* USER CODE BEGIN Start_WIFI_SEND_Routine */
  while (!ESP_Init()){                                                          // Запуск ESP8266
        DEBUG("ESP init failure\n");
        HAL_Delay(2000);
  }
  /* Infinite loop */
  for(;;)
  {
    ESP_Send_Packet(R.ESP_PacketType_to_send);
    if (R.ESP_PacketType_to_send != GET_DEVIDSRV) osDelay(58000);
    osDelay(2000);
  }
  /* USER CODE END Start_WIFI_SEND_Routine */
}

/* USER CODE BEGIN Header_Start_OLED_Out */
/**
* @brief Function implementing the OLED_Out thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_OLED_Out */
void Start_OLED_Out(void const * argument)
{
  /* USER CODE BEGIN Start_OLED_Out */
  uint8_t i;
                ssd1306_Init();
              //HAL_Delay(1000);
              //ssd1306_Fill(White);
              //ssd1306_UpdateScreen();

              //HAL_Delay(1000);

              ssd1306_SetCursor(5,5);
              //ssd1306_WriteString("4ilo",Font_11x18,Black);
              //ssd1306_WriteString("4ilo",Font_7x10,Black);
              ssd1306_WriteString("IVA-S/W",Font_16x26,White);
              ssd1306_UpdateScreen();
              
              for (i=0;i<32;i++){
                ssd1306_DrawPixel(0, i, White);
                ssd1306_DrawPixel(127, i, White);
              }
              for (i=0;i<128;i++){
                ssd1306_DrawPixel(i, 0, White);
                ssd1306_DrawPixel(i, 31, White);
              }
              ssd1306_UpdateScreen();
              R.OLED_Refresh_Enable = true;
  /* Infinite loop */
  for(;;)
  {
    if(R.OLED_Refresh_Enable){
      
//          HAL_I2C_DeInit(&hi2c3);
//          HAL_I2C_Init(&hi2c3);
          
          ssd1306_Fill(Black);
              
          ssd1306_SetCursor(0,0);
          sprintf(TmpStr, "VA=%.1f", __VOLT_PhA);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          ssd1306_SetCursor(0,9);
          sprintf(TmpStr, "VB=%.1f", __VOLT_PhB);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          ssd1306_SetCursor(0,18);
          sprintf(TmpStr, "VC=%.1f", __VOLT_PhC);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          
          
          ssd1306_SetCursor(64,0);
          sprintf(TmpStr, "CA=%.1f", __CURRENT_PhA);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          ssd1306_SetCursor(64,9);
          sprintf(TmpStr, "CB=%.1f", __CURRENT_PhB);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          ssd1306_SetCursor(64,18);
          sprintf(TmpStr, "CC=%.1f", __CURRENT_PhC);
          ssd1306_WriteString(TmpStr,Font_7x10,White);
          
          ssd1306_UpdateScreen();
      }
      
      osDelay(300);
  }
  /* USER CODE END Start_OLED_Out */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
