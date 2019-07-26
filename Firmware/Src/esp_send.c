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
//void local_func1 (void);
//void local_func2 (void);
//...
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
                                return true;
                          } 
return false;      
}

bool get_DevID_cb (uint8_t *line) {

//{"data":{"deviceID":"d41ff596-ee5d-494f-992f-c90deb30909a"},"errors":[],"success":true}0,CLOSED
      if (strstr((char *)line, "{\"data\":{\"deviceID\":\"")){
                                memcpy(R.DeviceID, (char *)line+21, 36);
                                //*((char *)line + 21 + 36) = 0;
                                __LED_SERV_ACK(1);
                                DEBUG("\nDevice ID: ");
                                DEBUG((char*)R.DeviceID);
                                DEBUG("\n");
                                R.ESP_PacketType_to_send = TXT_FREQ;
                                M.ESP_MsgCnt++;
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
    
    // Установление связи с сервером :
    SendCmd("AT+CIPMUX=1\r\n");                                           
    sprintf(TmpStr,"AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
    if (SendCmd(TmpStr) == 0) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=0,%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, get_DevID_cb);

          SendCmd("AT+CIPCLOSE=0\r\n");                       // Закрытие соединения
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
                "\"deviceId\":\"%s\","\
                "\"hardwareRevision\":\"%s\","\
                "\"firmwareVersion\":\"%s\","\
                "\"messageSentReason\":\"%s\","\
                "\"messageNumber\":%ld,"\
                "\"upTime\":%ld,"\
                "\"upTimeUnit\":\"s\",",\
          R.DeviceID, HARDWARE_REV, FIRMWARE_REV, "time", M.ESP_MsgCnt, M.UpTime
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
    SendCmd("AT+CIPMUX=1\r\n");
    sprintf(TmpStr,"AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
    if (SendCmd(TmpStr) == 0) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=0,%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, send_ACK_test);
          
          SendCmd("AT+CIPCLOSE=0\r\n");                                         // Закрытие соединения 
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
                "\"deviceId\":\"%s\","\
                "\"hardwareRevision\":\"%s\","\
                "\"firmwareVersion\":\"%s\","\
                "\"messageSentReason\":\"%s\","\
                "\"messageNumber\":%ld,"\
                "\"upTime\":%ld,"\
                "\"upTimeUnit\":\"s\",",\
          R.DeviceID, HARDWARE_REV, FIRMWARE_REV, "time", M.ESP_MsgCnt, M.UpTime
       );
    
    strcat((char*)POST_Body, "\"onBoard\":[");
    add_json_onboard_inlet (POST_Body, "supplyVoltage",  M.Power_Sense, "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "onBoardBattery", M.Vbat,        "V");   strcat((char*)POST_Body, ",");
    add_json_onboard_inlet (POST_Body, "MCUTemperature", M.Temp_MCU,    "C");   
    strcat((char*)POST_Body, "],");
    strcat((char*)POST_Body, "\"inputs\":[");
  
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_A", "voltage", __VOLT_PhA, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_B", "voltage", __VOLT_PhB, "V"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "VOLT_C", "voltage", __VOLT_PhC, "V"); strcat((char*)POST_Body, ",");
    
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_A", "current", __CURRENT_PhA, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_B", "current", __CURRENT_PhB, "A"); strcat((char*)POST_Body, ",");
    add_json_ext_inlet (POST_Body, "AC_ANALOG_INPUT", "CURRENT_C", "current", __CURRENT_PhC, "A"); 
    
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
    SendCmd("AT+CIPMUX=1\r\n");
    sprintf(TmpStr,"AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n", S.Srv1.Domain, S.Srv1.Port);
    if (SendCmd(TmpStr) == 0) {
        __LED_SERV_CONNECT(1);  
    
          // Формирование команды на отправку данных с актуальной длиной передаваемого пакета
          //char TmpStr[32];
          sprintf(TmpStr, "AT+CIPSEND=0,%d\r\n", (strlen((char*)POST_Header)+strlen((char*)POST_Body)));
          SendCmd(TmpStr);
                                           
          // Отправка POST запроса
          SendCmd((char*)POST_Header);
          SendCmd_cb((char*)POST_Body, send_ACK_test);
          
          SendCmd("AT+CIPCLOSE=0\r\n");                                         // Закрытие соединения 
        __LED_SERV_CONNECT(0);
        __LED_SERV_ACK(0);
        
    };
    M.ESP_MsgCnt++;
}



//******************************************************************************
//   Частая отправка в текстовом формате
//******************************************************************************
void txt_freq_send(void){
  
  
  if (S.AC_In_Send_Enable)       { send_AC();           };
  if (S.DC_In_Send_Enable)       { send_DC();           };
//  if (S.Discrete_In_Send_Enable) { send_Discrete();     };
//  if (S.Speed_Send_Enable)       { send_Speed();        };
  
 
};

void txt_rare_send(void){

};

void bin_rare_send(void){
  memset(POST_Header, 0, sizeof(POST_Header));
  memset(POST_Body, 0,   sizeof(POST_Body));
};
//******************************************************************************
// ENF OF FILE
//******************************************************************************