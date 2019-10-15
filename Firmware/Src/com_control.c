/*
* Copyright (C) 2018 Yuri Ryzhenko <Y.Ryzhenko@hi-tech.org>, Aleksey Kirsanov <a.kirsanov@iva-tech.ru>
* All rights reserved
*
* File Name  : com_control.c
* Description: DSP processing
*/
//******************************************************************************
// Секция include: здесь подключается заголовочный файл к модулю
//******************************************************************************
#include "com_control.h" // Включаем файл заголовка для нашего модуля
#include "stdio.h"
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
static char Tmp_buf[128];
static char *Ptr;
static uint8_t i, ii;
//******************************************************************************
// Секция прототипов локальных функций
//******************************************************************************

//******************************************************************************
// Секция описания функций (сначала глобальных, потом локальных)
//******************************************************************************
static void NetStatus_Send      (void);
static void Meas_Send           (void);

//******************************************************************************
// Вычисление К 
//******************************************************************************
float calculate_K_scale (float FactADC, uint16_t AMV){
float Out;
  M.Lock = true;
    if (FactADC == 0) return 1;                                                 // Делитьь на 0 нельзя
    Out = (float)AMV / FactADC;
  M.Lock = false;
  return Out;
}

float calculate_R_scale (float FactADC, float AMV){
float Out;
  M.Lock = true;
    if (FactADC == 0) return 1;                                                 // Делитьь на 0 нельзя
    Out = (float)AMV / FactADC;
  M.Lock = false;
  return Out;
}



//******************************************************************************
// Обработка новой строки в буфере
//******************************************************************************
void BT_ParseCommand (char *Line){

  //---------------------------------------------------------------------------- AT+CLBACCK=<Phase>,<ActualMeasuredValue>
  Ptr=strstr(Line, "AT+CLBACCK");                                            
  if (Ptr){
    Ptr +=strlen("AT+CLBACCK=");
    switch (*Ptr){
      case 'A':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhA].K = calculate_K_scale(M.Current_Phase_A, atoi(Tmp_buf));
      break;
      case 'B':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhB].K = calculate_K_scale(M.Current_Phase_B, atoi(Tmp_buf));
      break;
      case 'C':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhC].K = calculate_K_scale(M.Current_Phase_C, atoi(Tmp_buf));
      break;
    default:
      DEBUG("\nERROR\n");
      break;
    }
    save_settings_to_EEPROM(&S);
    S = load_settings_from_EEPROM();   
    sprintf(Tmp_buf, "Curr A: K=%f B=%f R=%f\n", S.Curr[PhA].K, S.Curr[PhA].B, S.Curr[PhA].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Curr B: K=%f B=%f R=%f\n", S.Curr[PhB].K, S.Curr[PhB].B, S.Curr[PhB].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Curr C: K=%f B=%f R=%f\n", S.Curr[PhC].K, S.Curr[PhC].B, S.Curr[PhC].R);  DEBUG(Tmp_buf);
  }
  
  


  //---------------------------------------------------------------------------- AT+CLBACCR=<Phase>,<ActualMeasuredValue>
  Ptr=strstr(Line, "AT+CLBACCR");                                            
  if (Ptr){
    Ptr +=strlen("AT+CLBACCR=");
    switch (*Ptr){
      case 'A':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhA].R = calculate_R_scale(M.Current_Phase_A * S.Curr[PhA].K, (float)atof(Tmp_buf));
      break;
      case 'B':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhB].R = calculate_R_scale(M.Current_Phase_B * S.Curr[PhB].K, (float)atof(Tmp_buf));
      break;
      case 'C':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhC].R = calculate_R_scale(M.Current_Phase_C * S.Curr[PhC].K, (float)atof(Tmp_buf));
      break;
      case 'X':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Curr[PhC].R = calculate_R_scale(M.Current_Phase_C * S.Curr[PhC].K, (float)atof(Tmp_buf));
        S.Curr[PhB].R = calculate_R_scale(M.Current_Phase_B * S.Curr[PhB].K, (float)atof(Tmp_buf));
        S.Curr[PhA].R = calculate_R_scale(M.Current_Phase_A * S.Curr[PhA].K, (float)atof(Tmp_buf));
      break;
    default:
      DEBUG("\nERROR\n");
      break;
    }
    save_settings_to_EEPROM(&S);
    S = load_settings_from_EEPROM();   
    sprintf(Tmp_buf, "Curr A: K=%f B=%f R=%f\n", S.Curr[PhA].K, S.Curr[PhA].B, S.Curr[PhA].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Curr B: K=%f B=%f R=%f\n", S.Curr[PhB].K, S.Curr[PhB].B, S.Curr[PhB].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Curr C: K=%f B=%f R=%f\n", S.Curr[PhC].K, S.Curr[PhC].B, S.Curr[PhC].R);  DEBUG(Tmp_buf);
  }


  
  //---------------------------------------------------------------------------- AT+CLBACVK=<Phase>,<ActualMeasuredValue>
  Ptr=strstr(Line, "AT+CLBACVK");                                            
  if (Ptr){
    Ptr +=strlen("AT+CLBACVK=");
    switch (*Ptr){
      case 'A':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhA].K = calculate_K_scale(M.Volt_Phase_A, atoi(Tmp_buf));
      break;
      case 'B':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhB].K = calculate_K_scale(M.Volt_Phase_B, atoi(Tmp_buf));
      break;
      case 'C':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhC].K = calculate_K_scale(M.Volt_Phase_C, atoi(Tmp_buf));
      break;
    default:
      DEBUG("\nERROR\n");
      break;
    }
    
    save_settings_to_EEPROM(&S);
    S = load_settings_from_EEPROM();
    sprintf(Tmp_buf, "Volt A: K=%f B=%f R=%f\n", S.Volt[PhA].K, S.Volt[PhA].B, S.Volt[PhA].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Volt B: K=%f B=%f R=%f\n", S.Volt[PhB].K, S.Volt[PhB].B, S.Volt[PhB].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Volt C: K=%f B=%f R=%f\n", S.Volt[PhC].K, S.Volt[PhC].B, S.Volt[PhC].R);  DEBUG(Tmp_buf);
  }
  
  
  
  
  //---------------------------------------------------------------------------- AT+CLBACVR=<Phase>,<ActualMeasuredValue>
  Ptr=strstr(Line, "AT+CLBACVR");                                            
  if (Ptr){
    Ptr +=strlen("AT+CLBACVR=");
    switch (*Ptr){
      case 'A':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhA].R = calculate_R_scale(M.Volt_Phase_A * S.Volt[PhA].K, (float)atof(Tmp_buf));
      break;
      case 'B':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhB].R = calculate_R_scale(M.Volt_Phase_B * S.Volt[PhB].K, (float)atof(Tmp_buf));
      break;
      case 'C':
        ii=0; Ptr++; Ptr++;
        while (*Ptr != '\r'){                                                   // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки                                       
        }
        S.Volt[PhC].R = calculate_R_scale(M.Volt_Phase_C * S.Volt[PhC].K, (float)atof(Tmp_buf));
      break;
    default:
      DEBUG("\nERROR\n");
      break;
    }
    save_settings_to_EEPROM(&S);
    S = load_settings_from_EEPROM(); 
    sprintf(Tmp_buf, "Volt A: K=%f B=%f R=%f\n", S.Volt[PhA].K, S.Volt[PhA].B, S.Volt[PhA].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Volt B: K=%f B=%f R=%f\n", S.Volt[PhB].K, S.Volt[PhB].B, S.Volt[PhB].R);  DEBUG(Tmp_buf);
    sprintf(Tmp_buf, "Volt C: K=%f B=%f R=%f\n", S.Volt[PhC].K, S.Volt[PhC].B, S.Volt[PhC].R);  DEBUG(Tmp_buf);
  }
  
  
  
  
  
  
  
      
     
      

  
  
  
  //---------------------------------------------------------------------------- AT+NETSET
  Ptr=strstr(Line, "AT+NETSET");                                            
  if (Ptr){
    NetStatus_Send();
  }
  
  //---------------------------------------------------------------------------- AT+MEAS
  Ptr=strstr(Line, "AT+MEAS");                                            
  if (Ptr){
    Meas_Send();
  }
  
  //---------------------------------------------------------------------------- AT+SEND
  Ptr=strstr(Line, "AT+SEND");                                            
  if (Ptr){
    DEBUG("\n+SEND Not implemented\n");
  }
  
  //---------------------------------------------------------------------------- AT+SETDEF    
  Ptr=strstr(Line, "AT+SETDEF");                                            
  if (Ptr){
    load_default_settings_to_EEPROM();
    DEBUG("\nDefault settings loaded to EEPROM\n");
  } 
  
  
  
  //---------------------------------------------------------------------------- AT+SETSERV=<SERV_NUM>,<DOMAIN>,<IP>,<PORT>
  Ptr=strstr(Line, "AT+SETSERV=");                                            
  if (Ptr){
    Ptr +=strlen("AT+SETSERV="); 
    switch (*(Ptr)){
    case '1':                                                                   // Профиль сервера 1
      ii=0; Ptr++; Ptr++;                                                       // Переход на доменное имя сервера
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.Srv1.Domain + ii) = *Ptr;   Ptr++;   ii++;                        // Копируем имя домена                   
          if (*(Ptr) == ',') *(S.Srv1.Domain + ii) = '\0';                      // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != ','){                                                      // Ищем до конца IP адреса 
          if (ii>62) { break; }
          *(S.Srv1.IP + ii) = *Ptr;   Ptr++;   ii++;                            // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.Srv1.IP + ii) = '\0';                          // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки 
      }
      S.Srv1.Port = atoi(Tmp_buf);                                              // Порт в число
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETSERV=1,%s,%s,%d\n", S.Srv1.Domain,S.Srv1.IP,S.Srv1.Port);
      DEBUG(Tmp_buf);
      break;
      
      
      
      
    case '2':                                                                   // Профиль сервера 2
      ii=0; Ptr++; Ptr++;                                                       // Переход на доменное имя сервера
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.Srv2.Domain + ii) = *Ptr;   Ptr++;   ii++;                        // Копируем имя домена                   
          if (*(Ptr) == ',') *(S.Srv2.Domain + ii) = '\0';                      // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != ','){                                                      // Ищем до конца IP адреса 
          if (ii>62) { break; }
          *(S.Srv2.IP + ii) = *Ptr;   Ptr++;   ii++;                            // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.Srv2.IP + ii) = '\0';                          // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки 
      }
      S.Srv2.Port = atoi(Tmp_buf);                                          // Порт в число
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETSERV=2,%s,%s,%d\n", S.Srv2.Domain,S.Srv2.IP,S.Srv2.Port);
      DEBUG(Tmp_buf);
      break;
      
      
      
    case '3':                                                                   // Профиль сервера 3
      ii=0; Ptr++; Ptr++;                                                       // Переход на доменное имя сервера
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.Srv3.Domain + ii) = *Ptr;   Ptr++;   ii++;                        // Копируем имя домена                   
          if (*(Ptr) == ',') *(S.Srv3.Domain + ii) = '\0';                      // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != ','){                                                      // Ищем до конца IP адреса 
          if (ii>62) { break; }
          *(S.Srv3.IP + ii) = *Ptr;   Ptr++;   ii++;                            // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.Srv3.IP + ii) = '\0';                          // Явно обозначаем конец строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>8) { break; }
          Tmp_buf[ii] = *Ptr;   Ptr++;   ii++;                                  // Копируем номер порта во временную строку                
          if (*(Ptr) == '\r') Tmp_buf[ii] = '\0';                               // Явно обозначаем конец строки 
      }
      S.Srv3.Port = atoi(Tmp_buf);                                          // Порт в число
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETSERV=3,%s,%s,%d\n", S.Srv3.Domain,S.Srv3.IP,S.Srv3.Port);
      DEBUG(Tmp_buf);
      break;
        default:
      break;
    }
  }
  
  
  

  //---------------------------------------------------------------------------- AT+SETAP=<APnum>,<SSID>,<PASS>
  Ptr=strstr(Line, "AT+SETAP=");                                            
  if (Ptr){
    Ptr +=strlen("AT+SETAP=");
    for (i=0;i<12;i++) { Tmp_buf[i]=0; } 
    switch (*(Ptr)){
    case '1':                                                                   // Профиль АР 1
      ii=0; Ptr++; Ptr++;                                                       // Переход на имя сети
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.AP1.SSID + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.AP1.SSID + ii) = '\0';                         // Явно обозначаем коней строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>30) { break; }
          *(S.AP1.Pass + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем пароль                   
          if (*(Ptr) == '\r') *(S.AP1.Pass + ii) = '\0';                        // Явно обозначаем коней строки             
      }
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETAP=1,%s,%s\n", S.AP1.SSID, S.AP1.Pass);
      DEBUG(Tmp_buf);
      break;
      
      
      
      
    case '2':                                                                   // Профиль АР 2
      ii=0; Ptr++; Ptr++;                                                       // Переход на имя сети
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.AP2.SSID + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.AP2.SSID + ii) = '\0';                         // Явно обозначаем коней строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>30) { break; }
          *(S.AP2.Pass + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем пароль                   
          if (*(Ptr) == '\r') *(S.AP2.Pass + ii) = '\0';                        // Явно обозначаем коней строки             
      }
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETAP=2,%s,%s\n", S.AP2.SSID, S.AP2.Pass);
      DEBUG(Tmp_buf);
      break;
      
      
      
    case '3':                                                                   // Профиль АР 3
      ii=0; Ptr++; Ptr++;                                                       // Переход на имя сети
      while (*Ptr != ','){                                                      // Ищем до конца имени сети 
          if (ii>62) { break; }
          *(S.AP3.SSID + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем имя сети                   
          if (*(Ptr) == ',') *(S.AP3.SSID + ii) = '\0';                         // Явно обозначаем коней строки                                                            // Перескок запятой
      }
      Ptr++; ii=0;                                                              // Перескакиваем запятую
      while (*Ptr != '\r'){                                                     // Ищем до конца строки
          if (ii>30) { break; }
          *(S.AP3.Pass + ii) = *Ptr;   Ptr++;   ii++;                           // Копируем пароль                   
          if (*(Ptr) == '\r') *(S.AP3.Pass + ii) = '\0';                        // Явно обозначаем коней строки             
      }
      save_settings_to_EEPROM(&S);
      sprintf (Tmp_buf, "\n+SETAP=3,%s,%s\n", S.AP3.SSID, S.AP3.Pass);
      DEBUG(Tmp_buf);
      break;
        default:
      break;
    }
  }
} // void BT_ParseCommand (char *Line)



//******************************************************************************
// Отправить в СОМ порт статус
//******************************************************************************
static void NetStatus_Send (void){

  sprintf(Tmp_buf, "+NETSET\r\n");
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Concentrator Universal-1\r\n");
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "FW Version %s\r\n", FIRMWARE_REV);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DivID: %s\r\n", R.DeviceID);
  DEBUG(Tmp_buf);
  
  sprintf(Tmp_buf, "AP1: SSID=%s, Pass=%s\r\n", S.AP1.SSID, S.AP1.Pass);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "AP2: SSID=%s, Pass=%s\r\n", S.AP2.SSID, S.AP2.Pass);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "AP3: SSID=%s, Pass=%s\r\n", S.AP3.SSID, S.AP3.Pass);
  DEBUG(Tmp_buf);
  
  sprintf(Tmp_buf, "DNS1=%s\r\n", S.DNS1);
  DEBUG(Tmp_buf);
  
  sprintf(Tmp_buf, "SRV1: Enable=%d, Domain=%s, IP=%s, Port=%d\r\n", S.Srv1.Enable, S.Srv1.Domain, S.Srv1.IP, S.Srv1.Port);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "SRV2: Enable=%d, Domain=%s, IP=%s, Port=%d\r\n", S.Srv2.Enable, S.Srv2.Domain, S.Srv2.IP, S.Srv2.Port);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "SRV3: Enable=%d, Domain=%s, IP=%s, Port=%d\r\n", S.Srv3.Enable, S.Srv3.Domain, S.Srv3.IP, S.Srv3.Port);
  DEBUG(Tmp_buf);

}  



//******************************************************************************
// Отправить в СОМ порт текущие показаний в измерительных каналах
//******************************************************************************
static void Meas_Send           (void){
  
  sprintf(Tmp_buf, "\n+MEAS\r\n");
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "HW: %s, FW: %s\r\n", HARDWARE_REV, FIRMWARE_REV);
  DEBUG(Tmp_buf);
  
  sprintf(Tmp_buf, "Ext power: %.1fV, VBAT: %.1fV\r\n", M.Power_Sense, M.Vbat);
  DEBUG(Tmp_buf);
  
  
  sprintf(Tmp_buf, "ADC A: %.3fV, %.3fA\r\n", M.Volt_Phase_A, M.Current_Phase_A);
  DEBUG(Tmp_buf);
  DEBUG("\r\n");
  sprintf(Tmp_buf, "Phase A:     %.1fV, %.2fA\r\n", __VOLT_PhA, __CURRENT_PhA);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase A MAX: %.1fV, %.2fA\r\n", __VOLT_PhA_MAX, __CURRENT_PhA_MAX);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase A MIN: %.1fV, %.2fA\r\n", __VOLT_PhA_MIN, __CURRENT_PhA_MIN);
  DEBUG(Tmp_buf);
  DEBUG("\r\n");
  sprintf(Tmp_buf, "Phase B    : %.1fV, %.2fA\r\n", __VOLT_PhB, __CURRENT_PhB);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase B_MAX: %.1fV, %.2fA\r\n", __VOLT_PhB_MAX, __CURRENT_PhB_MAX);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase B_MIN: %.1fV, %.2fA\r\n", __VOLT_PhB_MIN, __CURRENT_PhB_MIN);
  DEBUG(Tmp_buf);
  DEBUG("\r\n");
  sprintf(Tmp_buf, "Phase C    : %.1fV, %.2fA\r\n", __VOLT_PhC, __CURRENT_PhC);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase C_MAX: %.1fV, %.2fA\r\n", __VOLT_PhC_MAX, __CURRENT_PhC_MAX);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Phase C_MIN: %.1fV, %.2fA\r\n", __VOLT_PhC_MIN, __CURRENT_PhC_MIN);
  DEBUG(Tmp_buf);
  DEBUG("\r\n");
  
  clear_peak_detector_AC();
  
  sprintf(Tmp_buf, "DC In_0: %.3fV\r\n", M.AIN_0);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_1: %.3fV\r\n", M.AIN_1);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_2: %.3fV\r\n", M.AIN_2);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_3: %.3fV\r\n", M.AIN_3);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_4: %.3fV\r\n", M.AIN_4);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_5: %.3fV\r\n", M.AIN_5);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "DC In_6: %.3fV\r\n", M.AIN_6);
  DEBUG(Tmp_buf);
  
  DEBUG("\r\n");
  sprintf(Tmp_buf, "Discrete IN_0: %d\r\n", M.Discrete.IN_0);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_1: %d\r\n", M.Discrete.IN_1);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_2: %d\r\n", M.Discrete.IN_2);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_3: %d\r\n", M.Discrete.IN_3);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_4: %d\r\n", M.Discrete.IN_4);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_5: %d\r\n", M.Discrete.IN_5);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_6: %d\r\n", M.Discrete.IN_6);
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Discrete IN_7: %d\r\n", M.Discrete.IN_7);
  DEBUG(Tmp_buf);
  DEBUG("\r\n");
  
  sprintf(Tmp_buf, "Speed 1: >10000 ms\r\n");
  DEBUG(Tmp_buf);
  sprintf(Tmp_buf, "Speed 2: >10000 ms\r\n");
  DEBUG(Tmp_buf);

}


 

//******************************************************************************
// ENF OF FILE
//******************************************************************************
