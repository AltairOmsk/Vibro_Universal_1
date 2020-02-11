//******************************************************************************
// Секция include: здесь подключается заголовочный файл к модулю
//******************************************************************************
#include "esp_send.h" // Включаем файл заголовка для нашего модуля
#include "esp.h"
//******************************************************************************
// Секция определения переменных, используемых в модуле
//******************************************************************************
//------------------------------------------------------------------------------
// Глобальные
//------------------------------------------------------------------------------
//char GlobalVar1;
//char GlobalVar2;
//...
//------------------------------------------------------------------------------
// Локальные
//------------------------------------------------------------------------------
static uint8_t         POST_Header[256];
static uint8_t         POST_Body[2048];
//******************************************************************************
// Секция прототипов локальных функций
//******************************************************************************
void            txt_freq_send           (void);           
void            txt_rare_send           (void);           
void            bin_rare_send           (uint8_t *Buf, uint8_t Channel, MicDataType_e DataType);
bool            server_connect          (void);
//******************************************************************************
// Секция описания функций (сначала глобальных, потом локальных)
//******************************************************************************
/**
Копирует содержимое из исходной области памяти в целевую область память
\param[out] dest Целевая область памяти
\return
*/
bool send_ACK_test (uint8_t *line) {

//{"data":{},"errors":[],"success":true}0,CLOSED
      if (strstr((char *)line, "\"success\":true")){
                                __LED_SERV_ACK(1);
                                show_OLED_message("Success");
                                return true;
                          } 
return false;      
}

bool get_DevID_cb (uint8_t *line) {

//{"data":{"deviceID":"d41ff596-ee5d-494f-992f-c90deb30909a"},"errors":[],"success":true}0,CLOSED
      if (strstr((char *)line, "{\"data\":{\"deviceID\":\"")){
                                //memcpy(R.DeviceID, (char *)line+21, 36);
                                //*((char *)line + 21 + 36) = 0;
                                __LED_SERV_ACK(1);
                                DEBUG("\nDevice ID: ");
                                //DEBUG((char*)R.DeviceID);
                                DEBUG("\n");
                                show_OLED_message("ID received");
                                //R.DeviceID_OK = true;
                                R.ESP_PacketType_to_send = TXT_FREQ;
                                M.ESP_MsgCnt++;
                                return true;
                          }
return false;      
}






//******************************************************************************
//   Соединение с сервером
//******************************************************************************
/*
return true - connected
return false - error
*/
bool server_connect (void){
                                                                                // Попытка 1
  DEBUG("Try to connect 1\n");                                                  // Соединяемся. Если да - выходим с 1
  show_OLED_message("Try to connect 1");
  SendCmd("AT+CIPMUX=0\r\n");     
  SendCmd("AT+CIPMODE=0\r\n");
  sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
                  //sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"192.168.1.158\",8123\r\n");
  if (SendCmd(TmpStr) == 0) {
    show_OLED_message("Connected");
    return true;
  }
  show_OLED_message("Failed");
                                                                                // Попытка 2
                                                                                // Делаем рассоединение принудительное
  DEBUG("Try to connect 2\n");                                                  // Соединяемся. Если да - выходим с 1
  SendCmd("AT+CIPCLOSE\r\n");                                                   // Закрытие соединения
  SendCmd("AT+CIPMUX=0\r\n");  
  SendCmd("AT+CIPMODE=0\r\n");
  sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
                  //sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"192.168.1.158\",8123\r\n");
  if (SendCmd(TmpStr) == 0) {
    return true;
  }
  
                                                                                // Попытка 3
                                                                                // Аппаратный сброс ЕСП
                                                                                // Подкрючение к АР
                                                                                // Соединяемся. Если да, выходим с 1
                                                                                // Если не удалось - выходим с 0 
  DEBUG("Try to connect 3 with HW RESET WIFI module\n");

  __LED_NET(0);                                                                 // Гасим светодиод соединения с сервером
  R.WiFi_Connected = false;                                                     // Снимаем флаг соединения с wifi
  
  __WIFI_RESET  (0);                                                            // ESP HW reset
  HAL_Delay(20);
  __WIFI_RESET  (1);
  HAL_Delay(2000);
  
  
  DEBUG("WIFI Init\n");
  
  while (!ESP_Init()){                                                          // Запуск ESP8266
        DEBUG("ESP init failure\n");
        HAL_Delay(2000);
  }
  
  SendCmd("AT+CIPMUX=0\r\n");                                                   // Попытка соединения                                     
  sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
                  //sprintf(TmpStr,"AT+CIPSTART=\"TCP\",\"192.168.1.158\",8123\r\n");
  if (SendCmd(TmpStr) == 0) {
    return true;
  }
  
  
return false;
}






//******************************************************************************
//   GET Device ID
//******************************************************************************
void get_DevID_from_server(void) {
  
  memset(POST_Header, 0, sizeof(POST_Header));
  memset(POST_Body, 0, sizeof(POST_Body));
  
  
    sprintf((char*)POST_Body, \
      "{\"messageMapId\":\"post-new-device\","\
      "\"data\":{"\
      "\"vendorName\":\"ST\","\
      "\"vendorDeviceNumber\": \"%x%x%x\","\
      "\"hardwareRevision\": \"%s\","\
      "\"firmwareVersion\": \"%s\","\
      "\"companyID\": \"%s\""\
      "}}",\
      DevID_Dw2, DevID_Dw2, DevID_Dw0, HARDWARE_REV, FIRMWARE_REV, S.CompanyName\
    );
    
    
    sprintf((char*)POST_Header, "POST /sensors HTTP/1.1\r\n"\
                                "Host: motor-diag.7bits.it\r\n"\
                                "Accept: */*\r\n"\
                                "Content-Type: application/json\r\n"\
                                "Content-Length: %d\r\n"\
                                "Connection: Close\r\n"\
                                "\r\n",\
                                strlen((char*)POST_Body));
    
    // Установление связи с сервером и отправка:
    if (server_connect() == true) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, get_DevID_cb);

          SendCmd("AT+CIPCLOSE\r\n");                       // Закрытие соединения
        __LED_SERV_ACK(0);
        __LED_SERV_CONNECT(0);
    } ; 
}


void add_json_onboard_inlet (uint8_t *Str, const char *Name, float Val, const char *Unit){
  sprintf((char*)Str+strlen((char*)Str), "{\"name\":\"%s\","\
                                          "\"value\":%.3f,"\
                                          "\"valueUnit\":\"%s\"}",\
                                          Name, Val, Unit\
                                          );
}


void add_json_ext_inlet (uint8_t *Str, \
  const char *Type, const char *InNum, const char *MeasInf, float Val, const char *Unit){
  sprintf((char*)Str+strlen((char*)Str), "{\"inputType\":\"%s\","\
                                          "\"inputNumber\":\"%s\","\
                                          "\"measurableValue\":\"%s\","\
                                          "\"value\":%.3f,"\
                                          "\"valueUnit\":\"%s\"}",\
                                          Type, InNum, MeasInf, Val, Unit\
                                          );
}


void send_AC(void){
  memset(POST_Header, 0, sizeof(POST_Header));
  memset(POST_Body, 0,   sizeof(POST_Body));
      
  // Подготовка информационной части POST запроса 
    sprintf((char*)POST_Body, \
          "{"\
            "\"messageMapId\":\"post-device-data\","\
            "\"data\":{"\
                "\"inventoryNumber\":\"%s\","\
                "\"hardwareRevision\":\"%s\","\
                "\"firmwareVersion\":\"%s\","\
                "\"messageSentReason\":\"%s\","\
                "\"messageNumber\":%ld,"\
                "\"upTime\":%ld,"\
                "\"upTimeUnit\":\"s\",",\
          S.DeviceSerNum, HARDWARE_REV, FIRMWARE_REV, "time", M.ESP_MsgCnt, M.UpTime
       );
    
    strcat((char*)POST_Body, "\"onBoard\":[");
    add_json_onboard_inlet (POST_Body, "supplyVoltage",  M.Power_Sense, "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "onBoardBattery", M.Vbat,        "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "MCUTemperature", M.Temp_MCU,    "C");   
    strcat((char*)POST_Body, "],");
    strcat((char*)POST_Body, "\"inputs\":[");
  
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN0", "voltage", M.AIN_0, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN1", "voltage", M.AIN_1, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN2", "voltage", M.AIN_2, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN3", "voltage", M.AIN_3, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN4", "voltage", M.AIN_4, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN5", "voltage", M.AIN_5, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "DC_ANALOG_INPUT", "AIN6", "voltage", M.AIN_6, "V");   

    strcat((char*)POST_Body, "]}}");
    

    sprintf((char*)POST_Header, "POST /sensors HTTP/1.1\r\n"\
                                "Host: motor-diag.7bits.it\r\n"\
                                "Accept: */*\r\n"\
                                "Content-Type: application/json\r\n"\
                                "Content-Length: %d\r\n"\
                                "Connection: Close\r\n"\
                                "\r\n",\
                                strlen((char*)POST_Body));
    
    // Установление связи с сервером :
    if (server_connect() == true) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, send_ACK_test);
          
          SendCmd("AT+CIPCLOSE\r\n");                                         // Закрытие соединения 
        __LED_SERV_CONNECT(0);
        __LED_SERV_ACK(0);
        
    };
    M.ESP_MsgCnt++;
}




void send_DC (void){
  memset(POST_Header, 0, sizeof(POST_Header));
  memset(POST_Body, 0,   sizeof(POST_Body));
      
  // Подготовка информационной части POST запроса 
    sprintf((char*)POST_Body, \
          "{"\
            "\"messageMapId\":\"post-device-data\","\
            "\"data\":{"\
                "\"inventoryNumber\":\"%s\","\
                "\"hardwareRevision\":\"%s\","\
                "\"firmwareVersion\":\"%s\","\
                "\"messageSentReason\":\"%s\","\
                "\"messageNumber\":%ld,"\
                "\"upTime\":%ld,"\
                "\"upTimeUnit\":\"s\",",\
          S.DeviceSerNum, HARDWARE_REV, FIRMWARE_REV, "time", M.ESP_MsgCnt, M.UpTime
       );
    
    strcat((char*)POST_Body, "\"onBoard\":[");
    add_json_onboard_inlet (POST_Body, "supplyVoltage",  M.Power_Sense, "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "onBoardBattery", M.Vbat,        "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "MCUTemperature", M.Temp_MCU,    "C");   
    strcat((char*)POST_Body, "],");
    strcat((char*)POST_Body, "\"inputs\":[");
  
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_A", "voltage", M.Volt_Phase_A, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_B", "voltage", M.Volt_Phase_B, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_C", "voltage", M.Volt_Phase_C, "V"); strcat((char*)POST_Body, ",");
    
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_A", "current", M.Current_Phase_A, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_B", "current", M.Current_Phase_B, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_C", "current", M.Current_Phase_C, "A"); 
    
    strcat((char*)POST_Body, "]}}");
   

    sprintf((char*)POST_Header, "POST /sensors HTTP/1.1\r\n"\
                                "Host: motor-diag.7bits.it\r\n"\
                                "Accept: */*\r\n"\
                                "Content-Type: application/json\r\n"\
                                "Content-Length: %d\r\n"\
                                "Connection: Close\r\n"\
                                "\r\n",\
                                strlen((char*)POST_Body));
    
    // Установление связи с сервером :
    if (server_connect() == true) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, send_ACK_test);
          
          SendCmd("AT+CIPCLOSE\r\n");                                         // Закрытие соединения 
        __LED_SERV_CONNECT(0);
        __LED_SERV_ACK(0);
        
    };
    M.ESP_MsgCnt++;
}

//******************************************************************************
//   Отправка бинарного пакета
//******************************************************************************
/*
Отправка:
Готовим описательную часть запроса - там где описано что и от кого, без бинарного участка
Готовим заголовок

Отправляем заголовок, описательную часть и все бинарные части

Сложность в подсчете длины всего блока.
Длина бинарной части:
- определяем кол-во блоков (длину бинарного блока делим на 1200 байт) + остаток. Если остаток есть то +1 блок к частному
- к длине бинарного массива прибавляем длину начального и оконечного маркера * на кол-во блоков 

*/
void send_bin (uint16_t *Buf, uint32_t Len_byte){
  memset(POST_Header, 0, sizeof(POST_Header));
  memset(POST_Body, 0,   sizeof(POST_Body));
      
  // Подготовка описательной части POST запроса 
    sprintf((char*)POST_Body, \
          "{"\
            "\"messageMapId\":\"post-device-data\","\
            "\"data\":{"\
                "\"inventoryNumber\":\"%s\","\
                "\"hardwareRevision\":\"%s\","\
                "\"firmwareVersion\":\"%s\","\
                "\"messageSentReason\":\"%s\","\
                "\"messageNumber\":%ld,"\
                "\"upTime\":%ld,"\
                "\"upTimeUnit\":\"s\",",\
          S.DeviceSerNum, HARDWARE_REV, FIRMWARE_REV, "time", M.ESP_MsgCnt, M.UpTime
       );
    
    strcat((char*)POST_Body, "\"onBoard\":[");
    add_json_onboard_inlet (POST_Body, "supplyVoltage",  M.Power_Sense, "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "onBoardBattery", M.Vbat,        "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "MCUTemperature", M.Temp_MCU,    "C");   
    strcat((char*)POST_Body, "],");
    strcat((char*)POST_Body, "\"inputs\":[");
  
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_A", "voltage", M.Volt_Phase_A, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_B", "voltage", M.Volt_Phase_B, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_C", "voltage", M.Volt_Phase_C, "V"); strcat((char*)POST_Body, ",");
    
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_A", "current", M.Current_Phase_A, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_B", "current", M.Current_Phase_B, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_C", "current", M.Current_Phase_C, "A"); 
    
    strcat((char*)POST_Body, "]}}");
    
    // Подготовка заголовка 
    sprintf((char*)POST_Header, "POST /sensors HTTP/1.1\r\n"\
                                "Host: motor-diag.7bits.it\r\n"\
                                "Accept: */*\r\n"\
                                "Content-Type: application/json\r\n"\
                                "Content-Length: %d\r\n"\
                                "Connection: Close\r\n"\
                                "\r\n",\
                                strlen((char*)POST_Body));
    
    // Установление связи с сервером :
    if (server_connect() == true) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, send_ACK_test);
          
          SendCmd("AT+CIPCLOSE\r\n");                                         // Закрытие соединения 
        __LED_SERV_CONNECT(0);
        __LED_SERV_ACK(0);
        
    };
    M.ESP_MsgCnt++;
}             
             


//******************************************************************************
//   Частая отправка в текстовом формате
//******************************************************************************
void txt_freq_send(void){
  
  
  if (S.AC_In_Send_Enable)       { send_AC(); clear_peak_detector_AC(); };
  if (S.DC_In_Send_Enable)       { send_DC();           };
//  if (S.Discrete_In_Send_Enable) { send_Discrete();     };
//  if (S.Speed_Send_Enable)       { send_Speed();        };
  
 
};

void txt_rare_send(void){

};



void send_bin_part (uint8_t *Start, 
                    uint32_t Size, 
                    uint16_t Part_Num, 
                    uint8_t Channel,
                    MicDataType_e DataType){
                      
                      
//char Boundary_string[19];


//  snprintf(Boundary_string,   9, "%ld", DevID_Dw0);
//  snprintf(Boundary_string+9, 9, "%ld", DevID_Dw1);
//  Boundary_string[18] = 0;  
//  
//  memset(POST_Header, 0, sizeof(POST_Header));
//  memset(POST_Body, 0,   sizeof(POST_Body));
//  
//  // Подготовка заголовка 
//  sprintf((char*)POST_Header, "POST /sensors_bin HTTP/1.1\r\n"\
//                              "Host: motor-diag.7bits.it\r\n"\
//                              "Content-Type: multipart/form-data; boundary=%s\r\n"\
//                              "\r\n"\
//                              "--%s\r\n"\
//                              "Content-Disposition: form-data; name=\"messageMapID\"\r\n"\
//                              "\r\n"\
//                              "\"name of message map\"\r\n"\
//                              "--%s\r\n"\
//                              "Part №%d\r\n"\
//                              "Accept: */*\r\n"\
//                              "Content-Length: %d\r\n"\
//                              "Connection: Close\r\n"\
//                              "\r\n",\
//                              Boundary_string,\
//                              Boundary_string,\
//                              Boundary_string,\
//                              Part_Num,\
//                              strlen((char*)POST_Body)\
//                              );
//  
//  
//  // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
//  //char TmpStr[32];
//  sprintf(TmpStr, "AT+CIPSEND=0,%d\r\n", strlen((char*)POST_Header));
//  SendCmd(TmpStr);
//                                   
//  // Отправка POST запроса
//  SendCmd((char*)POST_Header);
//  //SendCmd_cb((char*)POST_Body, send_ACK_test);
  
  
//  sprintf(TmpStr, "AT+CIPSEND=0,%d\r\n", Size);
//  SendCmd(TmpStr);
  SendBin((uint8_t*)Start, Size);
  
}

//******************************************************************************
//   Отправка в бинарном формате редкая
//******************************************************************************
/*
Отправляем содержимое буфера по частям. Все размеры задаем вручную.
Имеем размер буфера заданный размером буфера. Скорее всего он будет ровно 100 000 байт.
Будем отправлять его кусками. Соединение открываем один раз на все 100 кБ.
Отправляем часть в два приема - сначала заголовок, потом bin тело.


*/
void bin_rare_send(uint8_t *Buf, uint8_t Channel, MicDataType_e DataType){
uint16_t Part_Cnt;

#define PART_CNT        40 //80
#define PART_SIZE       2000

            memset(Buf, 0, 80000);                                              // Заполнение буфера тестовыми значениями
            for (uint8_t i=0;i<80;i++){
              *((uint8_t*)Buf + (i*1000)) = i;
            }
  
  if (server_connect() == true) {                                               // Открываем соединение с сервером
      SendCmd("AT+CIPMODE=1\r\n");
      SendCmd("AT+CIPSEND\r\n");
      osDelay(300);
      for (Part_Cnt=0;Part_Cnt<PART_CNT;Part_Cnt++){                            // Отправка бинарного сообщения
        send_bin_part(Buf+(PART_SIZE * Part_Cnt), PART_SIZE, Part_Cnt, Channel, DataType);
        //osDelay(20);
      };
      osDelay(2000);
      DEBUG("+++\n");
      UART2_Send((uint8_t *)"+++", 3);
      osDelay(2000);
  }
  DEBUG("AT+CIPCLOSE\n");
  SendCmd("AT+CIPCLOSE\r\n");                                                   // Закрываем соединение с сервером
 
};





void all_data_send (void){}



//******************************************************************************
//   Проверка необходимости отправки и отправить
//******************************************************************************
void ESP_Send_Packet (void) {
  

      if (R.Timer_Send_Freq == 0) {                                       // Частая отправка (дискретные измерения)
        R.Timer_Send_Freq = S.Interval_Send_Freq;
        txt_freq_send();
      }
      
      if (R.Timer_Send_Rare == 0) {                                       // Редкая отправка (буфер 80 кБ)
        R.Timer_Send_Rare = S.Interval_Send_Rare;
        //bin_rare_send((uint8_t*)BigBuf, 0, SRC_fd48kHz_16bit);
      }
      
      if (R.Button_Send_Flag == true){                                    // Отправка всего по кнопке
        R.Button_Send_Flag = false;
        //txt_freq_send();
        bin_rare_send((uint8_t*)BigBuf, 0, SRC_fd48kHz_16bit);
      }
    
  
  


//  switch (Type) {                                   
//      case NO_SEND:       /*osThreadYield();*/                          break;
//      case GET_DEVIDSRV:    get_DevID_from_server();                    break;
//      case TXT_FREQ:        txt_freq_send();                            break;
//      case TXT_RARE:        txt_rare_send();                            break;
//      case BIN_RARE:        bin_rare_send(0, SRC_fd48kHz_16bit);        break;
//      default:                                                          break;
//  }

}
//******************************************************************************
// ENF OF FILE
//******************************************************************************