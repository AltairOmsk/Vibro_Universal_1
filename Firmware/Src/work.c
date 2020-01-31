/*
Задача отправки по WiFi постоянно мониторит необходимость отправки.
Таймеры перезапускается только после отправки.

*/
//******************************************************************************
// Секция include: здесь подключается заголовочный файл к модулю
//******************************************************************************
#include "work.h" // Включаем файл заголовка для нашего модуля
//******************************************************************************
// Секция определения переменных, используемых в модуле
//******************************************************************************
//------------------------------------------------------------------------------
// Глобальные
//------------------------------------------------------------------------------
MAIN_MODE_t     MainMode;                                                       // Текущая задача

uint32_t        DevID_Dw0;                                                      // Уникальный ID контроллера
uint32_t        DevID_Dw1;
uint32_t        DevID_Dw2;

char            TmpStr[128];                                                    // Строка для вского

REG_t           R;
MEAS_t          M;                                                              // Все измерения
SETTINGS_t      S;                                                              // Настройки и калибровочные константы для сохранения в EEPROM

#define         L_CODEC_CH                              0
#define         R_CODEC_CH                              1
#define         CODEC_BUF_SIZE                          1000                    // 1000 элементов = 2000 байт. 250 стерео отсчетов в 2 полубуфера
int16_t         Codec_Rx[CODEC_BUF_SIZE];                                       // Временный буфер аудиокодека                                                          
int16_t         Codec_Tx[CODEC_BUF_SIZE];

#define         BIGBUF_SIZE                             40000                   // 40 000 элементов == 80 000 байт
int16_t         BigBuf[BIGBUF_SIZE];                                            // Большой буфер для кодека и акселерометра. По очереди.


MEAS50_t        Meas50Ch[6];                                                    // Контексты фильтров для напряжения и тока 50 Гц
LPF2_t          ADC1_Filter[ADC1_CH_CNT];                                       // Контексты фильтров для аналоговых входов





//------------------------------------------------------------------------------
// Локальные
//------------------------------------------------------------------------------
//static char LocalVar1;
//static char LocalVar2;
//...
//******************************************************************************
// Секция прототипов локальных функций
//******************************************************************************


//******************************************************************************
// Секция описания функций (сначала глобальных, потом локальных)
//******************************************************************************

void bluetooth_init (void){
  __BT_RESET    (1);
  
  DevID_Dw0 = HAL_GetUIDw0();
  DevID_Dw1 = HAL_GetUIDw1();
  DevID_Dw2 = HAL_GetUIDw2();
  
  sprintf(TmpStr, "AT+ROLE0"); 
  //DEBUG(); 
  HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(200);
  
  sprintf(TmpStr, "AT+NAMEVIB_%02d%02d", DevID_Dw0&0xFF, (DevID_Dw0>>16)&0xFF);    
  HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(500);
  
  sprintf(TmpStr, "AT+RESET");      
  HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(10);
}


void get_DeviceID (void){
  //ESP_
}

void meas_3Ph_power (MEAS_t *M){
/*
Конфигурируем АЦП.
Включаем Таймер 5 запускающий АЦП. Принимаем в ДМА. Дма на 7 значений.
За один раз принимаем 7 показаний - три тока, три напряжения, и опорный уровень
В прерывании ДМА раскладываем по фильтрам принятое от АЦП и выход выкладываем в MEAS.

Всего надо сделать не менее 8000 вызовов фильтров, для установления напряжения на фильтрах.
*/  
}

#define EEPROM_ADRESS            (0x50 << 1)    /* A0 = A1 = A2 = 0 */
#define MEMORY_ADDRESS           0x00



//******************************************************************************
//   Сохранение настроек в ЕЕПРОМ
//******************************************************************************
void save_settings_to_EEPROM(SETTINGS_t *Data){
HAL_StatusTypeDef status;
     
uint16_t i;
#define PAGE_SIZE 32
    
    //while(R.OLED_Refresh_Enable);
    R.EEPROM_in_progress = true;
    osDelay(200);


    for (i=0;i<sizeof(SETTINGS_t)/PAGE_SIZE; i++)  {                            // Пишем страницами по 32 байта
      

        for(;;) { // wait...                                                    // Контроль готовности памяти к работе
          status = HAL_I2C_IsDeviceReady(&hi2c3, EEPROM_ADRESS, 1, 100);
          if(status == HAL_OK)
              break;
        }
        
        HAL_I2C_Mem_Write (&hi2c3,                                              // Запись одной страницы
                          (uint16_t)EEPROM_ADRESS, 
                          i*PAGE_SIZE, 
                          I2C_MEMADD_SIZE_16BIT, 
                          ((uint8_t*)Data) + i*PAGE_SIZE, 
                          PAGE_SIZE, 
                          2000);
    }
    
        for(;;) { // wait...
          status = HAL_I2C_IsDeviceReady(&hi2c3, EEPROM_ADRESS, 1, 100);
          if(status == HAL_OK)
              break;
        }
    
        HAL_I2C_Mem_Write (&hi2c3,                                              // Запись последней неполной страницы
                          (uint16_t)EEPROM_ADRESS, 
                          (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          I2C_MEMADD_SIZE_16BIT, 
                          ((uint8_t*)Data) + (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          sizeof(SETTINGS_t) - (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          2000);
    
    
    R.EEPROM_in_progress = false;

}


//******************************************************************************
//   Сохранение настроек по умолчанию в ЕЕПРОМ
//******************************************************************************
void load_default_settings_to_EEPROM (void) {
  SETTINGS_t Out = {
      .CompanyName         = "1",
    
      .AP1.SSID            = "HiTech_2G",
      .AP1.Pass            = "Crocodile102017",
      
      .AP2.SSID            = "",
      .AP2.Pass            = "",
      
      .AP3.SSID            = "",
      .AP3.Pass            = "",
    
      .DNS1                = "208.67.222.222",
      
      //-------------------------
      .Srv1.Enable         = true,
      .Srv1.Domain         = "motor-diag.7bits.it",
      .Srv1.IP             = "",
      .Srv1.Port           = 8123,
      
      .Srv2.Enable         = false,
      .Srv2.Domain         = "",
      .Srv2.IP             = "",
      .Srv2.Port           = 0,
      
      .Srv3.Enable         = false,
      .Srv3.Domain         = "",
      .Srv3.IP             = "",
      .Srv3.Port           = 0,
   
      
      //--------------------------
      .Interval_Send_Freq       = 60,                                           // Интервал отправки аналоговых входов в секундах
      .Interval_Send_Rare       = 600,                                          // Интервал отправки микрофонов и акселерометров в секундах
  
      .AC_In_Send_Enable        = true,
      .DC_In_Send_Enable        = true,
      .Discrete_In_Send_Enable  = false,
      .Speed_Send_Enable        = false,
      .Mic1_Send_Enable         = false,
      .Mic2_Send_Enable         = false,
      .Mic3_Send_Enable         = false,
      .Ax1_Send_Enable          = false,
      .Ax2_Send_Enable          = false,
      
      
      //--------------------------
      .Vref_EEPROM         = 1.19,                                              //1.18 ... 1.24
  
      .Scale_AIN_0         = 1,
      .Scale_AIN_1         = 1,
      .Scale_AIN_2         = 1,
      .Scale_AIN_3         = 1,
      .Scale_AIN_4         = 1,
      .Scale_AIN_5         = 1,
      .Scale_AIN_6         = 1,
      .Scale_PowerSense    = 1,
      .Scale_Vbat          = 1,
      
      .Volt[PhA].K         = 1,
      .Volt[PhA].B         = 0,
      .Volt[PhA].R         = 1,
      
      .Volt[PhB].K         = 1,
      .Volt[PhB].B         = 0,
      .Volt[PhB].R         = 1,
      
      .Volt[PhC].K         = 1,
      .Volt[PhC].B         = 0,
      .Volt[PhC].R         = 1,
      
      .Curr[PhA].K         = 1,
      .Curr[PhA].B         = 0,
      .Curr[PhA].R         = 1,
      
      .Curr[PhB].K         = 1,
      .Curr[PhB].B         = 0,
      .Curr[PhB].R         = 1,
      
      .Curr[PhC].K         = 1,
      .Curr[PhC].B         = 0,
      .Curr[PhC].R         = 1,
      
      .Mic[MIC_3].Gain_HP  = 20
  };
  
  save_settings_to_EEPROM(&Out);
  S = Out;
}









//******************************************************************************
//   Чтение настроек из ЕЕПРОМ
//******************************************************************************
SETTINGS_t load_settings_from_EEPROM (void){
SETTINGS_t Out;
HAL_StatusTypeDef status;  
    
    //while(R.OLED_Refresh_Enable);
    R.EEPROM_in_progress = true;
    osDelay(200);
    
    
    for(;;) { // wait...
        status = HAL_I2C_IsDeviceReady(&hi2c3, EEPROM_ADRESS, 1, HAL_MAX_DELAY);
        if(status == HAL_OK)
            break;
    }
    
    HAL_I2C_Mem_Read (&hi2c3, 
                     (uint16_t)EEPROM_ADRESS, 
                     (uint16_t)MEMORY_ADDRESS, 
                     I2C_MEMADD_SIZE_16BIT, 
                     (uint8_t*)&Out, 
                     sizeof(SETTINGS_t), 
                     2000);
    
    R.EEPROM_in_progress = false;
 
return Out;
}








//******************************************************************************
//   Инициализации периферии платы
//******************************************************************************
void board_init (void){
uint8_t i;

  __HAL_DBGMCU_FREEZE_TIM5();                                                   // Что бы таймер останавливался при отладке
  
//---   Без переменных   -------------------------------------------------------
  bluetooth_init();
  __CODEC_RESET (1);                                                            // 0 - Reset, 1 - Work
  __LED_NET(0);
  __LED_SERV_CONNECT(0);
  __LED_SERV_ACK(0);
  

  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);                                          // Запуск ЦАП
  
  
  
  
//---   С переменными   -------------------------------------------------------- 
  
  //memset(&BB_Rx, 0x01, sizeof(BB));

  S = load_settings_from_EEPROM();
  
  M.UpTime = 0;
  M.ESP_MsgCnt = 0;
  R.WiFi_Connected = false;
  R.ESP_PacketType_to_send = GET_DEVIDSRV;
          //R.ESP_PacketType_to_send = TXT_FREQ;
  
//  sprintf (S.DNS1, "208.67.222.222");                                           // SendCmdEx("AT+CIPDNS_CUR=1,\"208.67.222.222\"\r\n", 10000);
  
//  sprintf (S.Srv1.Domain, "motor-diag.7bits.it");
//           S.Srv1.Port    = 8123;
//           S.Srv1.Enable  = true;
  
  
  
  meas_50Hz_filter_init(&Meas50Ch[VOLT_A],      1);                             // Фильтрв канада 50 Гц
  meas_50Hz_filter_init(&Meas50Ch[VOLT_B],      1);
  meas_50Hz_filter_init(&Meas50Ch[VOLT_C],      1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_A],   1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_B],   1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_C],   1);
  
  
  for (i=0; i< ADC1_CH_CNT; i++){                                               // Фильтры AIN
      ADC1_Filter[i].S1 = 6.1616576042876322e-07;
      ADC1_Filter[i].A2 = -1.9977785594429318;
      ADC1_Filter[i].A3 = 0.99778102410597347;
      ADC1_Filter[i].B2 = 2;
  }


}


void clear_peak_detector_AC (void) {
    M.Volt_Phase_A_MAX = 0;
    M.Volt_Phase_B_MAX = 0;
    M.Volt_Phase_C_MAX = 0;
    
    M.Volt_Phase_A_MIN = 3.402823466e+38;
    M.Volt_Phase_B_MIN = 3.402823466e+38;
    M.Volt_Phase_C_MIN = 3.402823466e+38;
    
    M.Current_Phase_A_MAX = 0;
    M.Current_Phase_B_MAX = 0;
    M.Current_Phase_C_MAX = 0;
    
    M.Current_Phase_A_MIN = 3.402823466e+38;
    M.Current_Phase_B_MIN = 3.402823466e+38;
    M.Current_Phase_C_MIN = 3.402823466e+38;
}




//******************************************************************************
//   Пиковый детектор
//******************************************************************************
void min_max_peak_detect (float *Src, float *Min, float *Max) {
    if (*Src > *Max) *Max = *Src;
    if (*Src < *Min) *Min = *Src;
}


//******************************************************************************
//   Обработка измерений 50 Гц
//******************************************************************************
void meas_50Hz_DMA_Int_routine (uint16_t *Buf){
uint8_t i;  


        if (M.Lock) return;                                                     // Если кто то читает данные, не обновляем
        
        for (i=0; i<40; i++){                                                   // Фильтрация сырых отсчетов АЦП
          M.Volt_Phase_A_raw    = meas_50Hz (&Meas50Ch[VOLT_A],    *(Buf + (ADC2_CH_CNT * i) + VOLT_A)); 
          M.Volt_Phase_B_raw    = meas_50Hz (&Meas50Ch[VOLT_B],    *(Buf + (ADC2_CH_CNT * i) + VOLT_B));
          M.Volt_Phase_C_raw    = meas_50Hz (&Meas50Ch[VOLT_C],    *(Buf + (ADC2_CH_CNT * i) + VOLT_C));
          
          min_max_peak_detect(&M.Volt_Phase_A_raw, &M.Volt_Phase_A_MIN, &M.Volt_Phase_A_MAX);// Пиковый детектор на каждом отсчете
          min_max_peak_detect(&M.Volt_Phase_B_raw, &M.Volt_Phase_B_MIN, &M.Volt_Phase_B_MAX);
          min_max_peak_detect(&M.Volt_Phase_C_raw, &M.Volt_Phase_C_MIN, &M.Volt_Phase_C_MAX);
          
          
          M.Current_Phase_A_raw = meas_50Hz (&Meas50Ch[CURRENT_A], *(Buf + (ADC2_CH_CNT * i) + CURRENT_A));
          M.Current_Phase_B_raw = meas_50Hz (&Meas50Ch[CURRENT_B], *(Buf + (ADC2_CH_CNT * i) + CURRENT_B));
          M.Current_Phase_C_raw = meas_50Hz (&Meas50Ch[CURRENT_C], *(Buf + (ADC2_CH_CNT * i) + CURRENT_C));
          
          min_max_peak_detect(&M.Current_Phase_A_raw, &M.Current_Phase_A_MIN, &M.Current_Phase_A_MAX);
          min_max_peak_detect(&M.Current_Phase_B_raw, &M.Current_Phase_B_MIN, &M.Current_Phase_B_MAX);
          min_max_peak_detect(&M.Current_Phase_C_raw, &M.Current_Phase_C_MIN, &M.Current_Phase_C_MAX);
        }
        
        //---------------------------------------------------------------------- в Вольты
        M.Volt_Phase_A =        (M.Volt_Phase_A_raw        * S.Volt[PhA].K * S.Volt[PhA].R);
        M.Volt_Phase_B =        (M.Volt_Phase_B_raw        * S.Volt[PhB].K * S.Volt[PhB].R);
        M.Volt_Phase_C =        (M.Volt_Phase_C_raw        * S.Volt[PhC].K * S.Volt[PhC].R);

        M.Volt_Phase_A_MAX =    (M.Volt_Phase_A_MAX    * S.Volt[PhA].K * S.Volt[PhA].R);
        M.Volt_Phase_B_MAX =    (M.Volt_Phase_B_MAX    * S.Volt[PhB].K * S.Volt[PhB].R);
        M.Volt_Phase_C_MAX =    (M.Volt_Phase_C_MAX    * S.Volt[PhC].K * S.Volt[PhC].R);

        M.Volt_Phase_A_MAX =    (M.Volt_Phase_A_MAX    * S.Volt[PhA].K * S.Volt[PhA].R);
        M.Volt_Phase_A_MAX =    (M.Volt_Phase_B_MIN    * S.Volt[PhB].K * S.Volt[PhB].R);
        M.Volt_Phase_A_MAX =    (M.Volt_Phase_C_MIN    * S.Volt[PhC].K * S.Volt[PhC].R);
          
        //---------------------------------------------------------------------- в Амперы  
        M.Current_Phase_A =     (M.Current_Phase_A_raw     * S.Curr[PhA].K * S.Curr[PhA].R);
        M.Current_Phase_B =     (M.Current_Phase_B_raw     * S.Curr[PhB].K * S.Curr[PhB].R);
        M.Current_Phase_C =     (M.Current_Phase_C_raw     * S.Curr[PhC].K * S.Curr[PhC].R);

        M.Current_Phase_A_MAX = (M.Current_Phase_A_MAX * S.Curr[PhA].K * S.Curr[PhA].R);
        M.Current_Phase_B_MAX = (M.Current_Phase_B_MAX * S.Curr[PhB].K * S.Curr[PhB].R);
        M.Current_Phase_C_MAX = (M.Current_Phase_C_MAX * S.Curr[PhC].K * S.Curr[PhC].R);

        M.Current_Phase_A_MIN = (M.Current_Phase_A_MIN * S.Curr[PhA].K * S.Curr[PhA].R);
        M.Current_Phase_B_MIN = (M.Current_Phase_B_MIN * S.Curr[PhB].K * S.Curr[PhB].R);
        M.Current_Phase_C_MIN = (M.Current_Phase_C_MIN * S.Curr[PhC].K * S.Curr[PhC].R);          
        
}









//******************************************************************************
//   Обработка измерений постоянного тока
//******************************************************************************
void meas_AINx_DMA_Int_routine (uint16_t *Buf){
/*
Пофильтровать ФНЧ и привести к милливольтам все входы  
*/  
  //HAL_GPIO_TogglePin(LED_PWR_GPIO_Port, LED_PWR_Pin);
  
  
      if (M.Lock) return;                                                       // Если кто то читает данные, не обновляем

                                                     //1.18 ... 1.24
#define POWER_SENSE_DIVIDER     11.86952
#define V25                     0.76
#define AVG_SLOPE               2.5      
      
  for (uint8_t i=0; i<40; i++) {
    M.VrefINT     = lpf_2(&ADC1_Filter[VREFINT],     *(Buf + (ADC1_CH_CNT * i) + VREFINT));
    M.Vbat        = lpf_2(&ADC1_Filter[VBAT],        *(Buf + (ADC1_CH_CNT * i) + VBAT));
    M.Power_Sense = lpf_2(&ADC1_Filter[POWER_SENSE], *(Buf + (ADC1_CH_CNT * i) + POWER_SENSE));
    M.Temp_MCU    = lpf_2(&ADC1_Filter[TEMP_MCU],    *(Buf + (ADC1_CH_CNT * i) + TEMP_MCU));
    
    M.AIN_0       = lpf_2(&ADC1_Filter[AIN_0],        *(Buf + (ADC1_CH_CNT * i) + AIN_0));
    M.AIN_1       = lpf_2(&ADC1_Filter[AIN_1],        *(Buf + (ADC1_CH_CNT * i) + AIN_1));
    M.AIN_2       = lpf_2(&ADC1_Filter[AIN_2],        *(Buf + (ADC1_CH_CNT * i) + AIN_2));
    M.AIN_3       = lpf_2(&ADC1_Filter[AIN_3],        *(Buf + (ADC1_CH_CNT * i) + AIN_3));
    M.AIN_4       = lpf_2(&ADC1_Filter[AIN_4],        *(Buf + (ADC1_CH_CNT * i) + AIN_4));
    M.AIN_5       = lpf_2(&ADC1_Filter[AIN_5],        *(Buf + (ADC1_CH_CNT * i) + AIN_5));
    M.AIN_6       = lpf_2(&ADC1_Filter[AIN_6],        *(Buf + (ADC1_CH_CNT * i) + AIN_6));
  }
  
  
    M.Vbat        = (S.Vref_EEPROM * M.Vbat * 2) / M.VrefINT  * S.Scale_Vbat;
    M.Power_Sense = (S.Vref_EEPROM * M.Power_Sense * POWER_SENSE_DIVIDER) / M.VrefINT * S.Scale_PowerSense;
    M.Temp_MCU    = (S.Vref_EEPROM * M.Temp_MCU) / M.VrefINT;                         // Напряжение на термометре в мВ
    M.Temp_MCU    = ((V25 - M.Temp_MCU) / AVG_SLOPE) + 25;                      // Перевод мВ в градусы Цельсия
    
    M.AIN_0       = (S.Vref_EEPROM * M.AIN_0) / M.VrefINT * S.Scale_AIN_0;
    M.AIN_1       = (S.Vref_EEPROM * M.AIN_1) / M.VrefINT * S.Scale_AIN_1;
    M.AIN_2       = (S.Vref_EEPROM * M.AIN_2) / M.VrefINT * S.Scale_AIN_2;
    M.AIN_3       = (S.Vref_EEPROM * M.AIN_3) / M.VrefINT * S.Scale_AIN_3;
    M.AIN_4       = (S.Vref_EEPROM * M.AIN_4) / M.VrefINT * S.Scale_AIN_4;
    M.AIN_5       = (S.Vref_EEPROM * M.AIN_5) / M.VrefINT * S.Scale_AIN_5;
    M.AIN_6       = (S.Vref_EEPROM * M.AIN_6) / M.VrefINT * S.Scale_AIN_6;
}


//******************************************************************************
//   Получить состояние дискретных входов
//******************************************************************************
void scan_discrete_inputs (void){
  
  M.Discrete.IN_0 = HAL_GPIO_ReadPin(DIN_6_GPIO_Port, DIN_6_Pin);
  M.Discrete.IN_1 = HAL_GPIO_ReadPin(DIN_7_GPIO_Port, DIN_7_Pin);
  M.Discrete.IN_2 = HAL_GPIO_ReadPin(DIN_4_GPIO_Port, DIN_4_Pin);
  M.Discrete.IN_3 = HAL_GPIO_ReadPin(DIN_5_GPIO_Port, DIN_5_Pin);
  M.Discrete.IN_4 = HAL_GPIO_ReadPin(DIN_2_GPIO_Port, DIN_2_Pin);
  M.Discrete.IN_5 = HAL_GPIO_ReadPin(DIN_3_GPIO_Port, DIN_3_Pin);
  M.Discrete.IN_6 = HAL_GPIO_ReadPin(DIN_0_GPIO_Port, DIN_0_Pin);
  M.Discrete.IN_7 = HAL_GPIO_ReadPin(DIN_1_GPIO_Port, DIN_1_Pin);

}


//******************************************************************************
//   Обработка нажатия кнопки на плате
//******************************************************************************
/*
  Функция неблокирующая, желательно вызывать с периодом в 1 мс
*/
void scan_onboard_send_btn  (void){
static uint16_t Delay_Cnt=0;

#define MAX_DELAY                       6000
#define DELAY_SHORT_PRESS               100
#define DELAY_LONG_PRESS                3000


  if (HAL_GPIO_ReadPin(BTN_SEND_GPIO_Port, BTN_SEND_Pin) == GPIO_PIN_RESET) {   // Если есть нажатие
    if (Delay_Cnt < MAX_DELAY) Delay_Cnt++;
  }
  else {                                                                        // Кнопка отпущена
    if (Delay_Cnt) Delay_Cnt--;
    
          if (Delay_Cnt == DELAY_SHORT_PRESS){                                  // Короткое нажатие
            Delay_Cnt = 0;
            R.Button_Send_Flag = true;
          } 
          
          if (Delay_Cnt == DELAY_LONG_PRESS){                                   // Долгое нажатие
            Delay_Cnt = 0;
            R.Button_Send_Flag = true;
          }
  }
}


//******************************************************************************
//   Копирование 
//******************************************************************************


//******************************************************************************
//   В полубуфере кодека есть новые данные
//******************************************************************************
void codec_IT_routine (void){
  
  // Копирование в ТХ буфер
  if (R.Codec_Ready_Buf == 0){            // HT
    memcpy(Codec_Tx, Codec_Rx, CODEC_BUF_SIZE);
  } else {                              // TC
    memcpy(Codec_Tx+(CODEC_BUF_SIZE/2), Codec_Rx+(CODEC_BUF_SIZE/2), CODEC_BUF_SIZE);
  }

  
  // Запись в большой буфер
  if (R.Codec_Rec_Enable){
          if (R.Codec_Ready_Buf == 0){            // HT
                for(uint16_t i=0;i<(CODEC_BUF_SIZE/4);i++){ // 0...500
                    BigBuf[i + R.Rec_Samples_Cnt] = Codec_Rx[i*2 + R_CODEC_CH];
                }
          } else {                              // TC
                for(uint16_t i=0;i<(CODEC_BUF_SIZE/4);i++){ // 0...250
                    BigBuf[i + (R.Rec_Samples_Cnt)] = Codec_Rx[(CODEC_BUF_SIZE/2) + i*2 + R_CODEC_CH];
                }
                
          }
          R.Rec_Samples_Cnt += (CODEC_BUF_SIZE/4);
          if (R.Rec_Samples_Cnt >= BIGBUF_SIZE) {
                R.Codec_Rec_Enable = 0;
                R.Rec_Samples_Cnt  = 0;
          }
  }
  
  
  
}



//******************************************************************************
//   Основная функция
//******************************************************************************
void meas_and_send (void) {
  
  scan_discrete_inputs ();
  
//  switch (MainMode) {
//   case GetDeviceID:        get_DeviceID();                     break;
//   case Meas3Phase:         meas_3Ph_power(&M);                 break;
//   case Send_to_IP1:                                            break;
//  default:
//    break;
//  }
}


void create_screen (SCREEN_e Scr){
char TmpStr[64];  
  switch (Scr){
  case SCREEN_MEAS_ALL_CURRENT:                                                 // Токи по фазам
    ssd1306_Fill(Black);
        
    ssd1306_SetCursor(0,0);
    sprintf(TmpStr, "Tok A  %.0fA", M.Current_Phase_A);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,9);
    sprintf(TmpStr, "Tok B  %.0fA", M.Current_Phase_B);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,18);
    sprintf(TmpStr, "Tok C  %.0fA", M.Current_Phase_C);
    ssd1306_WriteString(TmpStr,Font_7x10,White);
    break;
  //----------------------------------------------------------------------------  
  case SCREEN_MEAS_ALL_VOLTAGE:                                                 // Напряжения по фазам
    ssd1306_Fill(Black);
    
    ssd1306_SetCursor(0,0);
    sprintf(TmpStr, "Hanp A  %.0fV", M.Volt_Phase_A);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,9);
    sprintf(TmpStr, "Hanp B  %.0fV", M.Volt_Phase_B);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,18);
    sprintf(TmpStr, "Hanp C  %.0fV", M.Volt_Phase_C);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    break;
  //----------------------------------------------------------------------------
  case SCREEN_MEAS_TEMP_0_1_2:                                                  // Температуры 0 1 2
    ssd1306_Fill(Black);
    
    ssd1306_SetCursor(0,0);
    sprintf(TmpStr, "Temn 1  %.0f*", M.AIN_0 * 100);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,9);
    sprintf(TmpStr, "Temn 2  %.0f*", M.AIN_1 * 100);
    ssd1306_WriteString(TmpStr,Font_7x10,White);

    ssd1306_SetCursor(0,18);
    sprintf(TmpStr, "Temn 3  %.0f*", M.AIN_2 * 100);
    ssd1306_WriteString(TmpStr,Font_7x10,White);
    break; 
  //----------------------------------------------------------------------------
  case SCREEN_MSG:                                                              // Текстовое сообщение
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(R.OLED_msg, Font_7x10,White);

    break;
  }
}


/*
Сообщение на ОЛЕД экран. Выводится из своего буфера в реестре, длина 256 знаков.
Выводить так:
Формируем сообщение в буфер.
Сброс таймера времени показа
Переключение экрана
Разрешение обновления
Сообщение будет показано стандартное время, потом опять идет смена экранов
*/
void show_OLED_message(char *Msg){
  sprintf(R.OLED_msg, Msg);
  R.OLED_Screen_Show_Timer = 0;
  R.OLED_Screen = SCREEN_MSG;
  R.OLED_Refresh_Enable = true;
}



//******************************************************************************
// ENF OF FILE
//******************************************************************************