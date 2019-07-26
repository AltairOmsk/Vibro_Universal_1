//******************************************************************************
// ������ include: ����� ������������ ������������ ���� � ������
//******************************************************************************
#include "work.h" // �������� ���� ��������� ��� ������ ������
//******************************************************************************
// ������ ����������� ����������, ������������ � ������
//******************************************************************************
//------------------------------------------------------------------------------
// ����������
//------------------------------------------------------------------------------
MAIN_MODE_t     MainMode;                                                       // ������� ������

uint32_t        DevID_Dw0;                                                      // ���������� ID �����������
uint32_t        DevID_Dw1;
uint32_t        DevID_Dw2;

char            TmpStr[128];                                                    // ������ ��� ������

REG_t           R;
MEAS_t          M;                                                              // ��� ���������
SETTINGS_t      S;                                                              // ��������� � ������������� ��������� ��� ���������� � EEPROM

#define         L_CODEC_CH                              0
#define         R_CODEC_CH                              1
#define         CODEC_BUF_SIZE                          1000                    // 1000 ��������� = 2000 ����. 250 ������ �������� � 2 ����������
int16_t         Codec_Rx[CODEC_BUF_SIZE];                                       // ��������� ����� �����������                                                          
int16_t         Codec_Tx[CODEC_BUF_SIZE];

#define         BIGBUF_SIZE                             40000                   // 40 000 ��������� == 80 000 ����
int16_t         BigBuf[BIGBUF_SIZE];                                            // ������� ����� ��� ������ � �������������. �� �������.


MEAS50_t        Meas50Ch[6];                                                    // ��������� �������� ��� ���������� � ���� 50 ��
LPF2_t          ADC1_Filter[ADC1_CH_CNT];                                       // ��������� �������� ��� ���������� ������





//------------------------------------------------------------------------------
// ���������
//------------------------------------------------------------------------------
//static char LocalVar1;
//static char LocalVar2;
//...
//******************************************************************************
// ������ ���������� ��������� �������
//******************************************************************************

//******************************************************************************
// ������ �������� ������� (������� ����������, ����� ���������)
//******************************************************************************

void bluetooth_init (void){
  __BT_RESET    (1);
  
  DevID_Dw0 = HAL_GetUIDw0();
  DevID_Dw1 = HAL_GetUIDw1();
  DevID_Dw2 = HAL_GetUIDw2();
  
  sprintf(TmpStr, "AT+ROLE0"); 
  //DEBUG(); 
  //HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(200);
  
  sprintf(TmpStr, "AT+NAMEVIB_%02d%02d", DevID_Dw0&0xFF, (DevID_Dw0>>16)&0xFF);    
  //HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(500);
  
  sprintf(TmpStr, "AT+RESET");      
  //HAL_UART_Transmit(&huart1, (uint8_t*)TmpStr, strlen(TmpStr), 500);  osDelay(100);
  HAL_Delay(10);
}


void get_DeviceID (void){
  //ESP_
}

void meas_3Ph_power (MEAS_t *M){
/*
������������� ���.
�������� ������ 5 ����������� ���. ��������� � ���. ��� �� 7 ��������.
�� ���� ��� ��������� 7 ��������� - ��� ����, ��� ����������, � ������� �������
� ���������� ��� ������������ �� �������� �������� �� ��� � ����� ����������� � MEAS.

����� ���� ������� �� ����� 8000 ������� ��������, ��� ������������ ���������� �� ��������.
*/  
}

#define EEPROM_ADRESS            (0x50 << 1)    /* A0 = A1 = A2 = 0 */
#define MEMORY_ADDRESS           0x00



//******************************************************************************
//   ���������� �������� � ������
//******************************************************************************
void save_settings_to_EEPROM(SETTINGS_t *Data){
HAL_StatusTypeDef status;
     
uint16_t i;
#define PAGE_SIZE 32

    R.OLED_Refresh_Enable = false;
    osDelay(200);


    for (i=0;i<sizeof(SETTINGS_t)/PAGE_SIZE; i++)  {                            // ����� ���������� �� 32 �����
      

        for(;;) { // wait...                                                    // �������� ���������� ������ � ������
          status = HAL_I2C_IsDeviceReady(&hi2c3, EEPROM_ADRESS, 1, 100);
          if(status == HAL_OK)
              break;
        }
        
        HAL_I2C_Mem_Write (&hi2c3,                                              // ������ ����� ��������
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
    
        HAL_I2C_Mem_Write (&hi2c3,                                              // ������ ��������� �������� ��������
                          (uint16_t)EEPROM_ADRESS, 
                          (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          I2C_MEMADD_SIZE_16BIT, 
                          ((uint8_t*)Data) + (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          sizeof(SETTINGS_t) - (sizeof(SETTINGS_t)/PAGE_SIZE) * PAGE_SIZE, 
                          2000);
    
    
    R.OLED_Refresh_Enable = true;

}


//******************************************************************************
//   ���������� �������� �� ��������� � ������
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
      .Freq_Send_Interval       = 0,                                            // �������� �������� ���������� ������ � ��������
      .Slow_Send_Interval       = 0,                                            // �������� �������� ���������� � �������������� � ��������
  
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
//   ������ �������� �� ������
//******************************************************************************
SETTINGS_t load_settings_from_EEPROM (void){
SETTINGS_t Out;
HAL_StatusTypeDef status;  

    R.OLED_Refresh_Enable = false;
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
    
    R.OLED_Refresh_Enable = true;
 
return Out;
}








//******************************************************************************
//   ������������� ��������� �����
//******************************************************************************
void board_init (void){
uint8_t i;

  __HAL_DBGMCU_FREEZE_TIM5();                                                   // ��� �� ������ �������������� ��� �������
  
//---   ��� ����������   -------------------------------------------------------
  bluetooth_init();
  __CODEC_RESET (1);                                                            // 0 - Reset, 1 - Work
  __LED_NET(0);
  __LED_SERV_CONNECT(0);
  __LED_SERV_ACK(0);
  

  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);                                          // ������ ���
  
  
  
  
//---   � �����������   -------------------------------------------------------- 
  
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
  
  
  
  meas_50Hz_filter_init(&Meas50Ch[VOLT_A],      1);                             // ������� ������ 50 ��
  meas_50Hz_filter_init(&Meas50Ch[VOLT_B],      1);
  meas_50Hz_filter_init(&Meas50Ch[VOLT_C],      1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_A],   1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_B],   1);
  meas_50Hz_filter_init(&Meas50Ch[CURRENT_C],   1);
  
  
  for (i=0; i< ADC1_CH_CNT; i++){                                               // ������� AIN
      ADC1_Filter[i].S1 = 6.1616576042876322e-07;
      ADC1_Filter[i].A2 = -1.9977785594429318;
      ADC1_Filter[i].A3 = 0.99778102410597347;
      ADC1_Filter[i].B2 = 2;
  }


}










//******************************************************************************
//   ��������� ��������� 50 ��
//******************************************************************************
void meas_50Hz_DMA_Int_routine (uint16_t *Buf){
uint8_t i;  


        if (M.Lock) return;                                                     // ���� ��� �� ������ ������, �� ���������
        
        for (i=0; i<40; i++){
          M.Volt_Phase_A = meas_50Hz (&Meas50Ch[VOLT_A], *(Buf + (ADC2_CH_CNT * i) + VOLT_A)); 
          M.Volt_Phase_B = meas_50Hz (&Meas50Ch[VOLT_B], *(Buf + (ADC2_CH_CNT * i) + VOLT_B));
          M.Volt_Phase_C = meas_50Hz (&Meas50Ch[VOLT_C], *(Buf + (ADC2_CH_CNT * i) + VOLT_C));
          
          M.Current_Phase_A = meas_50Hz (&Meas50Ch[CURRENT_A], *(Buf + (ADC2_CH_CNT * i) + CURRENT_A));
          M.Current_Phase_B = meas_50Hz (&Meas50Ch[CURRENT_B], *(Buf + (ADC2_CH_CNT * i) + CURRENT_B));
          M.Current_Phase_C = meas_50Hz (&Meas50Ch[CURRENT_C], *(Buf + (ADC2_CH_CNT * i) + CURRENT_C));
        }
}









//******************************************************************************
//   ��������� ��������� ����������� ����
//******************************************************************************
void meas_AINx_DMA_Int_routine (uint16_t *Buf){
/*
������������� ��� � �������� � ������������ ��� �����  
*/  
  //HAL_GPIO_TogglePin(LED_PWR_GPIO_Port, LED_PWR_Pin);
  
  
      if (M.Lock) return;                                                       // ���� ��� �� ������ ������, �� ���������

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
    M.Temp_MCU    = (S.Vref_EEPROM * M.Temp_MCU) / M.VrefINT;                         // ���������� �� ���������� � ��
    M.Temp_MCU    = ((V25 - M.Temp_MCU) / AVG_SLOPE) + 25;                      // ������� �� � ������� �������
    
    M.AIN_0       = (S.Vref_EEPROM * M.AIN_0) / M.VrefINT * S.Scale_AIN_0;
    M.AIN_1       = (S.Vref_EEPROM * M.AIN_1) / M.VrefINT * S.Scale_AIN_1;
    M.AIN_2       = (S.Vref_EEPROM * M.AIN_2) / M.VrefINT * S.Scale_AIN_2;
    M.AIN_3       = (S.Vref_EEPROM * M.AIN_3) / M.VrefINT * S.Scale_AIN_3;
    M.AIN_4       = (S.Vref_EEPROM * M.AIN_4) / M.VrefINT * S.Scale_AIN_4;
    M.AIN_5       = (S.Vref_EEPROM * M.AIN_5) / M.VrefINT * S.Scale_AIN_5;
    M.AIN_6       = (S.Vref_EEPROM * M.AIN_6) / M.VrefINT * S.Scale_AIN_6;
}


//******************************************************************************
//   �������� ��������� ���������� ������
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
//   ����������� 
//******************************************************************************


//******************************************************************************
//   � ���������� ������ ���� ����� ������
//******************************************************************************
void codec_IT_routine (void){
  
  // ����������� � �� �����
  if (R.Codec_Ready_Buf == 0){            // HT
    memcpy(Codec_Tx, Codec_Rx, CODEC_BUF_SIZE);
  } else {                              // TC
    memcpy(Codec_Tx+(CODEC_BUF_SIZE/2), Codec_Rx+(CODEC_BUF_SIZE/2), CODEC_BUF_SIZE);
  }

  
  // ������ � ������� �����
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
//   �������� �������
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

//******************************************************************************
// ENF OF FILE
//******************************************************************************