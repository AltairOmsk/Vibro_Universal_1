#ifndef _WORK_H // Блокируем повторное включение этого модуля
#define _WORK_H
//******************************************************************************
// Секция include: здесь подключаются заголовочные файлы используемых модулей
//******************************************************************************
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include "cmsis_os.h"
#include "string.h"
#include "meas50.h"

//******************************************************************************
// Секция определения констант
//******************************************************************************
//#define MY_CONST1 1
//#define MY_CONST2 2

//******************************************************************************
// Секция определения типов
//******************************************************************************
typedef enum {                  // Что конкретно делает устройство  
  GetDeviceID,                  // Получаем ID на сервере
  Meas3Phase,                   // Измерение напряжения и тока по трем фазам 50 Гц
  Send_to_IP1                   // Отправка на первый сервер
}
MAIN_MODE_t;


typedef struct {                                                                // Адрес сервера для передачи данных
  bool                  Enable;                                                 // Используем ли для передачи
  char                  Domain[64];                                             // Доменное имя
  char                  IP[16];                                                 // IP адрес
  uint16_t              Port;                                                   // Порт
}

REMOTE_SERVER_t;

typedef struct {
  char                  SSID[64];
  char                  Pass[32];
  uint8_t               Channel;
  uint8_t               RSSI;
}
WIFIAP_t;

typedef struct {                                                                // Установки канала кодека
  int16_t               Gain_MICPGA;
  int16_t               Gain_ADC;
  int16_t               Gain_DAC;
  int16_t               Gain_HP;
}
CODEC_CH_t;

typedef enum {
  MIC_1,
  MIC_2,
  MIC_3,
  MIC_CNT
}
MIC_e;

typedef enum {
  PhA,
  PhB,
  PhC,
  PHASE_CNT
}
PHASE_e;

typedef struct {
  float K;
  float B;
  float R;
}
AC_CURRENTCh_t;

typedef struct {
  float K;
  float B;
  float R;
}
AC_VOLTCh_t;


//#pragma pack(1)
//#pragma pack (push, 1)
typedef struct {
  
  //---   WiFI   ---------------------------------------------------------------
  uint32_t              CRC32;
  char                  CompanyName[64];
  
  WIFIAP_t              AP1;
  WIFIAP_t              AP2;
  WIFIAP_t              AP3;
  
  REMOTE_SERVER_t       Srv1;
  REMOTE_SERVER_t       Srv2;
  REMOTE_SERVER_t       Srv3;
  char                  DNS1[16];                                               // Адрес DNS сервера для записи в ESP
  
  
  
  //---   Behavior   -----------------------------------------------------------
  uint16_t              Interval_Send_Freq;                                     // Интервал отправки аналоговых входов в секундах
  uint16_t              Interval_Send_Rare;                                     // Интервал отправки микрофонов и акселерометров в секундах
  
  bool                  AC_In_Send_Enable;
  bool                  DC_In_Send_Enable;
  bool                  Discrete_In_Send_Enable;
  bool                  Speed_Send_Enable;
  bool                  Mic1_Send_Enable;
  bool                  Mic2_Send_Enable;
  bool                  Mic3_Send_Enable;
  bool                  Ax1_Send_Enable;
  bool                  Ax2_Send_Enable;
  
  
  //---   MEASURE   ------------------------------------------------------------
  float Vref_EEPROM;            
  
  float Scale_AIN_0;
  float Scale_AIN_1;
  float Scale_AIN_2;
  float Scale_AIN_3;
  float Scale_AIN_4;
  float Scale_AIN_5;
  float Scale_AIN_6;
  float Scale_PowerSense;
  float Scale_Vbat;
  
  AC_VOLTCh_t           Volt    [PHASE_CNT];
  AC_CURRENTCh_t        Curr    [PHASE_CNT];
  
  
  
 //---   Codec   ---------------------------------------------------------------
  CODEC_CH_t    Mic[MIC_CNT];                                                   // В текущей конфигурации три микрофона
}
SETTINGS_t;
//#pragma pack(pop)


typedef struct {
  uint8_t       IN_0:1;
  uint8_t       IN_1:1;
  uint8_t       IN_2:1;
  uint8_t       IN_3:1;
  uint8_t       IN_4:1;
  uint8_t       IN_5:1;
  uint8_t       IN_6:1;
  uint8_t       IN_7:1;
}
DIN_t;

typedef enum {
  NO_SEND,
  GET_DEVIDSRV, // Получение DevID от сервера
  TXT_FREQ,     // Частая отправка в текстовом формате
  TXT_RARE,     // Редкая отправка в текстовом формате
  BIN_RARE      // Редкая отправка бинарных данных
}
SEND_TYPE_e;




typedef enum {                  // Что отправляем в виде бинарных данных
  SRC_fd48kHz_16bit,            // Весь спектр 0-24кГц 16 бит (сырой АЦП)
  SRC_fd8kHz_bw3kHz_16bit       // Кусочек спектра шириной 3 кГц 16 бит (SSB USB)
}
MicDataType_e;

typedef enum {          // Виды причины отправки сообщения
  NO_SEND_REASON,
  TIMER_FREQ_REASON,
  TIMER_RARE_REASON,
  BUTTON_REASON,
  DISCRETE_CHANGE_REASON,
  LEVEL_REASON,
  OTHER_REASON
}
SEND_REASON_e;


typedef struct {
  
  bool          Lock;
  uint32_t      UpTime;                                                         // Время прошедшее с момента включения устройства в мс (+1 в системном тике)
  uint32_t      ESP_MsgCnt;
  
  
  //---   ADC1   ---------------------------------------------------------------
  float         AIN_0;
  float         AIN_1;
  float         AIN_2;
  float         AIN_3;
  float         AIN_4;
  float         AIN_5;
  float         AIN_6;
  float         Power_Sense;
  float         Temp_MCU;
  float         Vbat;
  float         VrefINT;
  
  
  //---   ADC2   ---------------------------------------------------------------
  float         Volt_Phase_A;
  float         Volt_Phase_B;
  float         Volt_Phase_C;
  
  float         Volt_Phase_A_MAX;
  float         Volt_Phase_B_MAX;
  float         Volt_Phase_C_MAX;
  
  float         Volt_Phase_A_MIN;
  float         Volt_Phase_B_MIN;
  float         Volt_Phase_C_MIN;
  
  
  float         Current_Phase_A;
  float         Current_Phase_B;
  float         Current_Phase_C;
  
  float         Current_Phase_A_MAX;
  float         Current_Phase_B_MAX;
  float         Current_Phase_C_MAX;
  
  float         Current_Phase_A_MIN;
  float         Current_Phase_B_MIN;
  float         Current_Phase_C_MIN;
  
  //---   Discrete Inputs   ----------------------------------------------------
  DIN_t         Discrete;

}
MEAS_t;



typedef struct {
  
  uint8_t               DeviceID[50];                                           // Для ID который выдаст сервер
  bool                  DeviceID_OK;
  SEND_TYPE_e           ESP_PacketType_to_send;                                 // Что отправлем сейчас: Текст короткий, длиный, запрос ID, бинарник
  bool                  WiFi_Connected;                                         // Флаг для информации
  bool                  OLED_Refresh_Enable;
  
  uint32_t              Timer_Send_Freq;                                        // Таймер для отправки частых сообщений
  uint32_t              Timer_Send_Rare;                                        // Таймер для отправки редких сообщений
  bool                  Button_Send_Flag;                                       // Флаг будет поставлен в обработчик кнопки, очищен после отправки сообщения

  
  uint16_t             *ADC1_Start_Point;                                       // Адрес полубуфера готового к обработке
  uint16_t             *ADC2_Start_Point;                                       // Адрес полубуфера готового к обработке

  uint8_t               Codec_Ready_Buf;                                        // В перывании сохранит сюда источник прерывания - 0=HT, 1=TC
  bool                  Codec_Rec_Enable;                                       // Разрешение записи
  uint32_t              Rec_Samples_Cnt;                                        // Сколько уже записано отсчетов

}
REG_t;




enum{
  AIN_0,
  AIN_1,
  AIN_2,
  AIN_3,
  AIN_4,
  AIN_5,
  AIN_6,
  POWER_SENSE,
  TEMP_MCU,
  VBAT,
  VREFINT,
  ADC1_CH_CNT
};


enum{
  VOLT_B,
  CURRENT_B,
  VOLT_C,
  CURRENT_C,
  VOLT_A,
  CURRENT_A,
  ADC2_CH_CNT
};


//******************************************************************************
// Секция определения глобальных переменных
//******************************************************************************
extern osSemaphoreId BinarySem_ADC1_ReadyHandle;
extern osSemaphoreId BinarySem_ADC2_50Hz_ReadyHandle;
extern osSemaphoreId BinarySem_Codec_ADC_ReadyHandle;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;
extern DAC_HandleTypeDef hdac;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;
extern I2S_HandleTypeDef hi2s2;
extern DMA_HandleTypeDef hdma_i2s2_ext_rx;
extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim5;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;


//******************************************************************************
// Секция прототипов глобальных функций
//******************************************************************************
void            board_init                              (void);
void            bluetooth_init                          (void);
void            meas_and_send                           (void);
void            meas_50Hz_DMA_Int_routine               (uint16_t *Buf);
void            meas_AINx_DMA_Int_routine               (uint16_t *Buf);
void            scan_discrete_inputs                    (void);
void            scan_onboard_send_btn                   (void);
void            save_settings_to_EEPROM                 (SETTINGS_t *Data);
void            load_default_settings_to_EEPROM         (void);
SETTINGS_t      load_settings_from_EEPROM               (void);
void            ADC2_DMA_XferCpltCallback               (struct __DMA_HandleTypeDef *hdma);
void            ADC2_DMA_XferHalfCpltCallback           (struct __DMA_HandleTypeDef *hdma);
void            codec_IT_routine                        (void);
void            clear_peak_detector_AC                  (void);

//******************************************************************************
// Секция определения макросов
//******************************************************************************
#define __WIFI_RESET(PinState)  HAL_GPIO_WritePin(WiFi_Reset_GPIO_Port, WiFi_Reset_Pin,\
                                ((PinState == 0)?(GPIO_PIN_RESET):(GPIO_PIN_SET)))  
#define __BT_RESET(PinState)    HAL_GPIO_WritePin(BT_Reset_GPIO_Port, BT_Reset_Pin,\
                                ((PinState == 0)?(GPIO_PIN_RESET):(GPIO_PIN_SET)))
#define __CODEC_RESET(PinState) HAL_GPIO_WritePin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin,\
                                ((PinState == 0)?(GPIO_PIN_RESET):(GPIO_PIN_SET)))
#define __LED_PWR(PinState)     HAL_GPIO_WritePin(LED_PWR_GPIO_Port, LED_PWR_Pin,\
                                ((PinState == 0)?(GPIO_PIN_SET):(GPIO_PIN_RESET)))
#define __LED_NET(PinState)     HAL_GPIO_WritePin(LED_NET_GPIO_Port, LED_NET_Pin,\
                                ((PinState == 0)?(GPIO_PIN_SET):(GPIO_PIN_RESET)))
#define __LED_SERV_CONNECT(PinState) HAL_GPIO_WritePin(LED_SRVR_GPIO_Port, LED_SRVR_Pin,\
                                ((PinState == 0)?(GPIO_PIN_SET):(GPIO_PIN_RESET)))
#define __LED_SERV_ACK(PinState)  HAL_GPIO_WritePin(LED_ANSW_GPIO_Port, LED_ANSW_Pin,\
                                ((PinState == 0)?(GPIO_PIN_SET):(GPIO_PIN_RESET)))


#define __CURRENT_PhA           (M.Current_Phase_A     * S.Curr[PhA].K * S.Curr[PhA].R)
#define __CURRENT_PhB           (M.Current_Phase_B     * S.Curr[PhB].K * S.Curr[PhB].R)
#define __CURRENT_PhC           (M.Current_Phase_C     * S.Curr[PhC].K * S.Curr[PhC].R)

#define __CURRENT_PhA_MAX       (M.Current_Phase_A_MAX * S.Curr[PhA].K * S.Curr[PhA].R)
#define __CURRENT_PhB_MAX       (M.Current_Phase_B_MAX * S.Curr[PhB].K * S.Curr[PhB].R)
#define __CURRENT_PhC_MAX       (M.Current_Phase_C_MAX * S.Curr[PhC].K * S.Curr[PhC].R)

#define __CURRENT_PhA_MIN       (M.Current_Phase_A_MIN * S.Curr[PhA].K * S.Curr[PhA].R)
#define __CURRENT_PhB_MIN       (M.Current_Phase_B_MIN * S.Curr[PhB].K * S.Curr[PhB].R)
#define __CURRENT_PhC_MIN       (M.Current_Phase_C_MIN * S.Curr[PhC].K * S.Curr[PhC].R)

#define __VOLT_PhA              (M.Volt_Phase_A        * S.Volt[PhA].K * S.Volt[PhA].R)
#define __VOLT_PhB              (M.Volt_Phase_B        * S.Volt[PhB].K * S.Volt[PhB].R)
#define __VOLT_PhC              (M.Volt_Phase_C        * S.Volt[PhC].K * S.Volt[PhC].R)

#define __VOLT_PhA_MAX          (M.Volt_Phase_A_MAX    * S.Volt[PhA].K * S.Volt[PhA].R)
#define __VOLT_PhB_MAX          (M.Volt_Phase_B_MAX    * S.Volt[PhB].K * S.Volt[PhB].R)
#define __VOLT_PhC_MAX          (M.Volt_Phase_C_MAX    * S.Volt[PhC].K * S.Volt[PhC].R)

#define __VOLT_PhA_MIN          (M.Volt_Phase_A_MAX    * S.Volt[PhA].K * S.Volt[PhA].R)
#define __VOLT_PhB_MIN          (M.Volt_Phase_B_MIN    * S.Volt[PhB].K * S.Volt[PhB].R)
#define __VOLT_PhC_MIN          (M.Volt_Phase_C_MIN    * S.Volt[PhC].K * S.Volt[PhC].R)


#endif // Закрывающий #endif к блокировке повторного включения
//******************************************************************************
// ENF OF FILE
//******************************************************************************