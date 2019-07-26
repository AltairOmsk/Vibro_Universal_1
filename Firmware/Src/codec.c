/*
* Copyright (C) 2018 Yuri Ryzhenko <Y.Ryzhenko@hi-tech.org>, Aleksey Kirsanov <a.kirsanov@iva-tech.ru>
* All rights reserved
*
* File Name  : codec.c
* Description: DSP processing
*/
//******************************************************************************
// Секция include: здесь подключается заголовочный файл к модулю
//******************************************************************************
#include "codec.h" // Включаем файл заголовка для нашего модуля
#include "cmsis_os.h"
//******************************************************************************
// Секция определения переменных, используемых в модуле
//******************************************************************************
//------------------------------------------------------------------------------
// Глобальные
//------------------------------------------------------------------------------
extern I2C_HandleTypeDef hi2c1;

//------------------------------------------------------------------------------
// Локальные
//------------------------------------------------------------------------------
uint8_t I2C_data_buf[2];

//******************************************************************************
// Секция прототипов локальных функций
//******************************************************************************
void codec_AGC_left  (uint8_t Enable);
void codec_AGC_right (uint8_t Enable);
void codec_DRC_left  (uint8_t Enable);

//******************************************************************************
// Секция описания функций (сначала глобальных, потом локальных)
//******************************************************************************
void     CODEC_Init     (void) {
 // static uint8_t temp;

  __CODEC_RESET_OFF;

  
  
      //--------------------------------------------------------------------------------------------------  
    I2C_Write (0x00, 0x00);             // Initialize to Page 0
    I2C_Write (0x01, 0x01);             // Initialize the device through software reset
    
    I2C_Write (0x12, 0x81);             // Power up NADC divider with value 1
    I2C_Write (0x13, 0x82);             // Power up MADC divider with value 2
    I2C_Write (0x14, 0x80);             // Program OSR for ADC to 128
 
    I2C_Write (0x0b, 0x81);             // Power up the NDAC divider with value 1
    I2C_Write (0x0c, 0x82);             // Power up the MDAC divider with value 2   
    I2C_Write (0x0d, 0x00);             // Program the OSR of DAC to 128 (CODEC default settings)
    I2C_Write (0x0e, 0x80);
    
    I2C_Write (0x1b, 0x00);             // Set the word length of Audio Interface to 16 bits PTM_P4
    I2C_Write (0x3c, 0x08);             // Set the DAC Mode to PRB_P8 (without DRC)   
          //I2C_Write (0x3c, 0x19);             // Set the DAC Mode to PRB_P25
    I2C_Write (0x3d, 0x01);             // Select ADC PRB_R1
    
    I2C_Write (0x36, 0x02);             // DIN is enabled for Primary Data Input or Digital Microphone Input or General Purpose Clock
    
    
    
    //--------------------------------------------------------------------------------------------------
    I2C_Write (0x00, 0x01);             // Select Page 1
    I2C_Write (0x01, 0x08);             // Disable Internal Crude AVdd in presence of external AVdd supply or before powering up internal AVdd LDO
    I2C_Write (0x02, 0x01);             // Enable Master Analog Power Control
    I2C_Write (0x7b, 0x01);             // Set the REF charging time to 40ms
    //I2C_Write (0x33, 0x40);             // Power-up MIC BIAS
    I2C_Write (0x14, 0x25);             /* HP soft stepping settings for optimal pop performance at power up
                                           Rpop used is 6k with N = 6 and soft step = 20usec. This should work with 47uF coupling
                                           capacitor. Can try N=5,6 or 7 time constants as well. Trade-off delay vs “pop” sound.*/
    I2C_Write (0x0a, 0x00);             // Set the Input Common Mode to 0.9V and Output Common Mode for Headphone to Input Common Mode

    I2C_Write (0x0c, 0x08);             // Route Left DAC to HPL
    //I2C_Write (0x0c, 0x02);           // MAL output is routed to HPL
    I2C_Write (0x0d, 0x08);             // Route Right DAC to HPR    
//  I2C_Write (0x0e, 0x08);             // Route Left DAC to LOL 
//   I2C_Write (0x0e, 0x10);             // Route Right DAC to  LOL
    I2C_Write (0x0f, 0x08);             // Route Right DAC to  LOR
    
    
    
    I2C_Write (0x03, 0x00);             // Set the DAC PTM mode to PTM_P3/4
    I2C_Write (0x04, 0x00);
    
    I2C_Write (0x10, 0x00);             // Set the HPL gain to 0dB   
    I2C_Write (0x11, 0x00);             // Set the HPR gain to 0dB
    
     
    I2C_Write (0x12, 0x00);             // Set the LOL gain to 0dB
    I2C_Write (0x13, 0x00);             // Set the LOR gain to 0dB
    
    I2C_Write (0x09, 0x3F);             // Power up HPL,HPR,LOL,LOR,MixAmp drivers
    
    I2C_Write (0x3d, 0x00);             // Select ADC PTM_R4
    I2C_Write (0x47, 0x32);             // Set MicPGA startup delay to 3.1ms
    I2C_Write (0x7b, 0x01);             // Set the REF charging time to 40ms
    
    

    // IN Right Channel
    I2C_Write (0x37, 0x40);             // IN1R is routed to Right MICPGA with 10k resistance
    I2C_Write (0x39, 0x10);             // IN1L is routed to Right MICPGA with 10k resistance    
    
    // IN Left Channel
    I2C_Write (0x34, 0x10);             // IN2L is routed to Left MICPGA with 10k resistance
    I2C_Write (0x36, 0x10);             // IN2R is routed to Left MICPGA with 10k resistance 
    
    

    I2C_Write (0x3b, 0x00);             // Unmute Left MICPGA, Gain 0dB
    I2C_Write (0x3c, 0x00);             // Unmute Right MICPGA, Gain 0dB

    
    osDelay(1000);                      // Wait for 2.5 sec for soft stepping to take effect
    
    
    //--------------------------------------------------------------------------------------------------
    I2C_Write (0x00, 0x00);             // Select Page 0      
    I2C_Write (0x3f, 0xd6);             // Power up the Left and Right DAC Channels with route the Left Audio digital data to
                                        // Left Channel DAC and Right Audio digital data to Right Channel DAC
    I2C_Write (0x40, 0x00);             // Unmute the DAC digital volume control
    I2C_Write (0x41, 0x00);             // Left DAC 0011 0000: Digital Volume Control = 0dB
    I2C_Write (0x42, 0x00);             // Right DAC 0011 0000: Digital Volume Control = 0dB
  
    I2C_Write (0x51, 0xc0);             // Power up Left and Right ADC Channels
    I2C_Write (0x52, 0x00);             // Unmute Left and Right ADC Digital Volume Control.
  
    
//    I2C_Write (0x1D, 0x10);             // LOOP I2S Stereo ADC output is routed to Stereo DAC input
//    I2C_Write (0x1D, 0x20);             // LOOP Audio Data in is routed to Audio Data out. (Works only when WCLK is configured as input.)

    
    set_RX_Gain_codec();
    set_TX_Gain_codec();
    
    
}



void codec_RX_mode (void){
/*
Правый канал не трогаем, MIC_PGA_R всегда подключен к входу IN1_R (MA)
Левый канал MIC_PGA_L на приеме подключен к IN1_L аналогично правому, настройки усиления PGA сделать свои
При переключении на передачу MIC_PGA_L подключается к IN_3 в дифференциальном виде

L_MIC_PGA_N -> CMN 10k
L_MIC_PGA_P -> IN1_L  
  
*/
    I2C_Write (0x00, 0x01);             // Select Page 1
    I2C_Write (0x34, 0x40);             // IN1L is routed to Left MICPGA with 10k resistance
    I2C_Write (0x36, 0x40);             // CM is routed to Left MICPGA via CM1L with 10k resistance  
    I2C_Write (0x3b, 0x3C);             // Unmute Left MICPGA, Gain 30dB (Max 0x5F) 
    

};



void codec_TX_mode (void){

// Микрофон на дифференциальный IN3L с входом 10 к
    I2C_Write (0x00, 0x01);             // Select Page 1
    I2C_Write (0x34, 0x04);             // IN3L is routed to Left MICPGA with 10k resistance
    I2C_Write (0x36, 0x04);             // IN3L is routed to Left MICPGA with 10k resistance 
    I2C_Write (0x3b, 0x00);             // Unmute Left MICPGA, Gain 0dB

};










void left_DAC_mute (void){
I2C_Write (0x00, 0x00);                                                         // Select Page 0
I2C_Write (0x40, (1 << 3));                                                     // Mute the Left DAC digital volume control
}

void right_DAC_mute (void){
I2C_Write (0x00, 0x00);                                                         // Select Page 0
I2C_Write (0x40, (1 << 2));                                                     // Mute theRight DAC digital volume control
}


void DAC_UNmute (void){
I2C_Write (0x00, 0x00);                                                         // Select Page 0
I2C_Write (0x40, 0x00);                                                         // Unmute the Left DAC digital volume control
}




//***   MICPGA Gain Control   **************************************************
void     Set_MICPGA_L_Gain (uint8_t gain)  {
  if (gain <= (47.5 / 0.5)){
    I2C_Write (00, 01);                   // Set Page 1
    I2C_Write (0x3b, gain);               // Unmute Left MICPGA, Gain 0dB
  }
}
void     Set_MICPGA_R_Gain (uint8_t gain)  {
  if (gain <= (47.5 / 0.5)){
    I2C_Write (00, 01);                   // Set Page 1
    I2C_Write (0x3c, gain);               // Unmute Right MICPGA, Gain 0dB
  }
}


//***   ADC Volume control   ***************************************************
void     Set_ADC_Vol_L     (int8_t Vol) {
  if ((Vol >= -24) && (Vol <= 40)) {
    I2C_Write (00, 00);                   // Set Page 0
    I2C_Write (0x53, (uint8_t)Vol);       // Left ADC Channel Volume
  }
}
void     Set_ADC_Vol_R     (int8_t Vol) {
  if ((Vol >= -24) && (Vol <= 40)) {
    I2C_Write (00, 00);                   // Set Page 0
    I2C_Write (0x54, (uint8_t)Vol);       // Right ADC Channel Volume
  }
}


//***   DAC Volume control   ***************************************************
void     Set_DAC_Vol_L     (int8_t Vol){    //Установить громкость ЦАП
  if ((Vol >= -127) && (Vol <= 48)) {
    I2C_Write (00, 00);                   // Set Page 0
    I2C_Write (0x41, (uint8_t)Vol);       // Left ADC Channel Volume
  }
}
void     Set_DAC_Vol_R     (int8_t Vol){    //Установить громкость ЦАП
  if ((Vol >= -127) && (Vol <= 48)) {
    I2C_Write (00, 00);                   // Set Page 0
    I2C_Write (0x42, (uint8_t)Vol);       // Right ADC Channel Volume
  }
}


//***   HPL Gain control   *****************************************************
void     Set_HPL_Gain    (int8_t Gain){   //Установить усиление усилителя наушников
  if ((Gain >= -6) && (Gain <= 29)){
    I2C_Write (00, 01);                   // Set Page 1
    Gain &= 0x3F;                         // Допустимы только младшие 6 бит
    I2C_Write (0x10, Gain);               // Unmute Left MICPGA, Gain 0dB
  }
}

//***   HPR Gain control   *****************************************************
void     Set_HPR_Gain    (int8_t Gain){   //Установить усиление усилителя наушников
  if ((Gain >= -6) && (Gain <= 29)){
    I2C_Write (00, 01);                   // Set Page 1
    Gain &= 0x3F;                         // Допустимы только младшие 6 бит
    I2C_Write (0x11, Gain);               // Unmute Right MICPGA, Gain 0dB
  }
}



//***   Загрузить настройки усиления в кодек   *********************************
void set_RX_Gain_codec (void) {
    Set_MICPGA_R_Gain   (0);              // Установить усиление MICPGA. В полудецибелах до 47.5 дБ
    Set_ADC_Vol_R       (0);              // -24  40 В полудецибелах
    Set_DAC_Vol_R       (0);              // -127 48 В полудецибелах
    Set_HPR_Gain        (0);              // -6 29  
}

void set_TX_Gain_codec (void) {
    Set_MICPGA_L_Gain   (0);              // Установить усиление MICPGA. В полудецибелах до 47.5 дБ
    Set_ADC_Vol_L       (0);              // -24  40 В полудецибелах
    Set_DAC_Vol_L       (0);              // -127 48 В полудецибелах
    Set_HPL_Gain        (0);              // -6 29  
}





void I2C_Write (uint8_t codec_register, uint8_t data){                    //Записать в кодек пл адресу регистра значение
uint8_t out_buf[2];
/**
  * @brief  Transmits in master mode an amount of data in blocking mode.
  * @param  hi2c : Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @param  DevAddress: Target device address
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be sent
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */

out_buf[0] = codec_register;
out_buf[1] = data;

HAL_I2C_Master_Transmit(&hi2c1, CODEC_I2C_ADRESS, out_buf, 2, 10000);

}  

//******************************************************************************
//   TX. Скоммутировать микрофон кодека на левый выход наушников кодека
//******************************************************************************
// Смотрим на контакты маски
//          O-Spk- 19 Ohm
//    Spk-*   *-mic
//          *-mic 145 Ohm 
void     codec_mic_to_left_phone        (void){
//    //Настройка кодека. Устанавливаем страницу регистров 1
//    I2C_Write (0x00, 0x01);             // Select Page 1
//    I2C_Write (0x0e, 0x02);             // Route Left  MAL to LOL (TX)
//    //I2C_Write (0x0e, 0x10);             // Route Right DAC to  LOR (RX)
//    I2C_Write (0x12, 0x00);             // LOL driver gain is 0 dB (TX)
    

    //I2C_Write (0x1D, 0x10);             // LOOP I2S Stereo ADC output is routed to Stereo DAC input
    //I2C_Write (0x1D, 0x20);             // LOOP Audio Data in is routed to Audio Data out. (Works only when WCLK is configured as input.)

  
}                                

 

//******************************************************************************
// RX. Скоммутировать вход 1R к АЦП, ЦАП на наушники правый и левый одинаково. 
//******************************************************************************
void     codec_IN1R_to_ADC              (void){
  
//        //Настройка кодека. Устанавливаем страницу регистров 1
//    I2C_Write (0x00, 0x01);             // Select Page 1
//    //I2C_Write (0x0e, 0x02);             // Route Left  MAL to LOL (TX)
//    I2C_Write (0x0e, 0x10);             // Route Right DAC to  LOR (RX)
//    I2C_Write (0x12, 0x1D);             // LOL driver gain is 29dB (RX)
  
  
  
  
  
        //Настройка кодека. Устанавливаем страницу регистров 1
        //I2C_Write (0x00, 0x01);             // Select Page 1
        //Устанавливаем усиление микрофонного усилителя 0 дБ
        //I2C_Write (0x3b, 0x14);             // Unmute Left  MICPGA, Gain 0dB
        //I2C_Write (0x3c, 0x00);             // Unmute Right MICPGA, Gain 0dB
        //Вход АЦП подключем к линейному входу 1 (так работало на испытаниях в Верблюжке)
//        I2C_Write (0x34, 0x40);             // IN1L is routed to Left MICPGA with 10k resistance
//        I2C_Write (0x36, 0x40);             // CM is routed to Left MICPGA via CM1L with 10k resistance  
        //I2C_Write (0x37, 0x40);             // IN1R is routed to Right MICPGA with 10k resistance
        //I2C_Write (0x39, 0x40);             // CM is routed to Right MICPGA via CM1R with 10k resistance   
               
//        //Вход АЦП подключем к линейному входу 2, аналогично каналу 1
//        I2C_Write (0x34, 0x10);             // IN2R is routed to Right MICPGA with 10k resistance
//        I2C_Write (0x36, 0x01);             // CM is routed to Left MICPGA via CM1L with 10k resistance  
//        I2C_Write (0x37, 0x10);             // IN1R is routed to Right MICPGA with 10k resistance
//        I2C_Write (0x39, 0x01);             // CM is routed to Right MICPGA via CM1R with 10k resistance 
        
        //Вход АЦП подключем к линейному входу 3 (микрофон) непонятно как работает, было сделано давно
//        I2C_Write (0x34, 0x04);             // IN3L is routed to Left MICPGA with 10k resistance
//        I2C_Write (0x36, 0x04);             // IN3R is routed to Left MICPGA with 10k resistance  
//        I2C_Write (0x37, 0x04);             // IN3R is routed to Right MICPGA with 10k resistance
//        I2C_Write (0x39, 0x04);             // IN3L is routed to Right MICPGA with 10k resistance
        
//        //Вход АЦП подключем дифференциально к входу 3 и коммутируем на правый канал АЦП
//        I2C_Write (0x34, 0x00);             // 
//        I2C_Write (0x36, 0x00);             //  
//        I2C_Write (0x37, 0x04);             // IN3R is routed to Right MICPGA with 10k resistance
//        I2C_Write (0x39, 0x04);             // IN3L is routed to Right MICPGA with 10k resistance
        
        
//        //Выход ЦАП подключаем на наушники
//        I2C_Write (0x0e, 0x00);             // UnRoute Left DAC to LOL
//        I2C_Write (0x0f, 0x00);             // UnRoute Right DAC to  LOR
//        //I2C_Write (0x0c, 0x08);             // Route Left DAC to HPL
//        I2C_Write (0x0c, 0x02);             // MAL output is routed to HPL
//        I2C_Write (0x0d, 0x08);             // Route Right DAC to HPR 
} 


//******************************************************************************
//   Codec I2C Read
//******************************************************************************
uint8_t I2C_Read  (uint8_t codec_register) {
uint8_t out;

/**
  * @brief  Receives in master mode an amount of data in blocking mode.
  * @param  hi2c : Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @param  DevAddress: Target device address
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be sent
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_I2C_Master_Transmit(&hi2c1, CODEC_I2C_ADRESS, &codec_register, 1, 10000);
HAL_I2C_Master_Receive (&hi2c1, CODEC_I2C_ADRESS, &out, 1, 10000);


return out;
}




//******************************************************************************
// Включение и настройка компрессора ЦАП. Передача на левом канале прием на правом
//******************************************************************************
void codec_DRC_left (uint8_t Enable){
//uint8_t Tmp;  
//
  if (Enable != 0) {

    // Включение пример из ДШ
    I2C_Write (0x00, 0x00);             // Initialize to Page 0
    I2C_Write (68, 0xF2);               // 
    I2C_Write (69, 0x00);               // 
    I2C_Write (70, 0xE2);               // 

  } else {
    // Выключение
    I2C_Write (0x00, 0x00);             // Initialize to Page 0
    I2C_Write (68, 0x00);               // 
    I2C_Write (69, 0x00);               // 
    I2C_Write (70, 0x00);               // 
  }
    


}

//******************************************************************************
// ENF OF FILE
//******************************************************************************
