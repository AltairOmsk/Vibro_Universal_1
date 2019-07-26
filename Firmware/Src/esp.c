#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "esp.h"
#include "usart.h"
#include "com_control.h"
#include "esp_send.h"



static uint32_t ESP_AT_Version;
static uint8_t ESP_Line[MAX_IPD_SIZE + MAX_REQUEST_SIZE];                       // Строка размером 1024+128
static int ESP_LineIndex = 0;
static int Esp_SendActive = 0;
static ESP_SendState_t Esp_SendState;

static uint8_t  BT_Line[MAX_IPD_SIZE + MAX_REQUEST_SIZE];                      // Строка размером 1024+128
static int      BT_LineIndex = 0;

ESP_Socket_t ESP_Sockets[MAX_SOCKETS];

ESP_Error_t ESP_WaitResponseEx(uint32_t timeout_ms, sendcmd_cb_t cb)
{
	register int data;
	register int length;
	register const ESP_Response_t *pattern;
	register uint8_t *end = ESP_Line;

	EspTimer = timeout_ms;
	while (EspTimer != 0)
	{
		if ((data = UART2_Get()) < 0)
		{
			//__WFI();
			continue;
		}

		//UART1_Putc(data);               // DEBUG Out
		if (data == '\r')
			continue;

		if (data == '\n')
		{
			*end = '\0';                                            // Последний символ = 0
			length = end - ESP_Line;                                // Длина строки
			if (length != 0)
			{
				if (cb != NULL)                                 // cb - возможно call back - в параметрах вызова функции
					if (cb(ESP_Line))
						return ESP_RESPONSE_OK;

				for (pattern = &ESP_Responses[0]; pattern->Response != NULL; pattern++)
				{
					if (strcmp(pattern->Response, (char *)ESP_Line) == 0)
						return pattern->Code;
				}
				end = ESP_Line;
			}
			continue;
		}

		*end++ = data;
		if ((end - ESP_Line) >= STRING_SIZE(ESP_Line))
		{	// Response too long
			return ESP_ERR_TOOLONG;
		}
	}
	return ESP_ERR_TIMEOUT;
}

bool ESP_PrepareResponse(
	register ESP_Socket_t * esp_socket,
	register const char * code,
	register const char * content_type,
	register const char * content,
	register const char * aux_headers)
{
	register int len = sizeof(ESP_Sockets[0].Header);
	register char * dst = (char *)esp_socket->Header;
	register int idx;

	for (;;)
	{
		idx = snprintf(dst, len, "HTTP/1.1 %s\r\n", code);
		if (idx < 0)
			break;
		len -= idx;
		dst += idx;
		if (len <= 0)
			break;

		if (content_type != NULL)
		{
			idx = snprintf(dst, len, "Content-Type:%s\r\n", content_type);
			if (idx < 0)
				break;
			len -= idx;
			dst += idx;
			if (len <= 0)
				break;
		}

		if (content != NULL)
		{
			esp_socket->ContentLength = strlen(content);
			strcpy((char *)esp_socket->Content, content);
		}
		if (esp_socket->ContentLength > 0)
		{
			idx = snprintf(dst, len, "Content-Length:%d\r\n", esp_socket->ContentLength);
			if (idx < 0)
				break;
			len -= idx;
			dst += idx;
			if (len <= 0)
				break;
		}

		if (aux_headers != NULL)
		{
			idx = snprintf(dst, len, "%s", aux_headers);
			if (idx < 0)
				break;
			len -= idx;
			dst += idx;
			if (len <= 0)
				break;
		}
		idx = snprintf(dst, len, "Connection:Close\r\n\r\n");
		if (idx < 0)
			break;
		len -= idx;
		dst += idx;
		if (len <= 0)
			break;

		esp_socket->HeaderLength = dst - (char *)esp_socket->Header;
		return true;
	}

	esp_socket->HeaderLength = 0;
	esp_socket->ContentLength = 0;
	esp_socket->Content_cb = NULL;
	DEBUG("ERROR: Output buffer too small\n");
	return false;
}

bool ESP_SetContent(register ESP_Socket_t *esp_socket, const char * content)
{
	int len = STRING_SIZE(ESP_Sockets[0].Content);
	len -= esp_socket->ContentLength;
	len = snprintf((char *)esp_socket->Content + esp_socket->ContentLength, len, "%s", content);
	if (len >= 0)
	{
		esp_socket->ContentLength += len;
		return true;
	}
	return false;
}

struct tm tmStr;
#define FAT_HOUR(time)		(time >> 11)
#define FAT_MINUTE(time)	((time >> 5) & 0x3F)
#define FAT_SECOND(time)	(2 * (time & 0x1F))
#define FAT_YEAR(date)		(1980 + (date >> 9))
#define FAT_MONTH(date)		((date >> 5) & 0xF)
#define FAT_DAY(date)		(date & 0x1F)

const char * months[]  = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char * wdays[]  = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};



const char * const Fail_Response_Folder =
	"<D:response>"
		"<D:href>/</D:href>"
		"<D:propstat>"
			"<D:status>HTTP/1.1 200 OK</D:status>"
			"<D:prop>"
				"<D:getlastmodified>Fri, 30 Nov 1812 00:00:00 GMT</D:getlastmodified>"
				"<D:getetag>\"3333333333333333333333333333333333333333\"</D:getetag>"
				"<D:resourcetype><D:collection/></D:resourcetype>"
			"</D:prop>"
		"</D:propstat>"
	"</D:response>";
const char * const Fail_Response_Folder2 =
	"<D:response>"
	"<D:href>/Failed to open SD</D:href>"
	"<D:propstat>"
		"<D:status>HTTP/1.1 200 OK</D:status>"
		"<D:prop>"
			"<D:getlastmodified>Fri, 01 Apr 1812 16:07:40 GMT</D:getlastmodified>"
			"<D:getetag>\"2222222222222222222222222222222222222222\"</D:getetag>"
			"<D:resourcetype/>"
			"<D:getcontentlength>0</D:getcontentlength>"
			"<D:getcontenttype>application/octet-stream</D:getcontenttype>"
		"</D:prop>"
	"</D:propstat>"
	"</D:response>";
const char * const Fail_Response_File =
	"<D:href>/Failed to open file</D:href>"
	"<D:propstat>"
		"<D:status>HTTP/1.1 200 OK</D:status>"
		"<D:prop>"
			"<D:getlastmodified>Fri, 01 Apr 1812 16:07:40 GMT</D:getlastmodified>"
			"<D:getetag>\"2222222222222222222222222222222222222222\"</D:getetag>"
			"<D:resourcetype/>"
			"<D:getcontentlength>0</D:getcontentlength>"
			"<D:getcontenttype>application/octet-stream</D:getcontenttype>"
		"</D:prop>"
	"</D:propstat>"
	"</D:response>";

bool ESP_ContentLastChunk(ESP_CB_TYPE_t cb_type, void * socket)
{
	if (cb_type != ESP_CB_BEFORE)
		return true;
	ESP_Socket_t * esp_socket = (ESP_Socket_t *)socket;
	esp_socket->ContentLength = CONTENT_LAST_CHUNK;
	return true;
}

bool ESP_GetContentChunk_3(ESP_CB_TYPE_t cb_type, void * socket)
{
	if (cb_type != ESP_CB_BEFORE)
		return true;
	ESP_Socket_t * esp_socket = (ESP_Socket_t *)socket;
	esp_socket->ContentLength = 0;
	ESP_SetContent(esp_socket, "</D:multistatus>");
	esp_socket->Content_cb = ESP_ContentLastChunk;
	return true;
}

bool ESP_GetHtmlPage_3(ESP_CB_TYPE_t cb_type, void * socket)
{
	if (cb_type != ESP_CB_BEFORE)
		return true;
	ESP_Socket_t * esp_socket = (ESP_Socket_t *)socket;
	esp_socket->ContentLength = 0;
	ESP_SetContent(esp_socket, "</table></body>");
	esp_socket->Content_cb = ESP_ContentLastChunk;
	return true;
}

void ESP_ProcessRequest(uint32_t i_socket, register char * request)
{

}

void ESP_ParseIPD(register char * line)
{
	register char * token = line;
	register int16_t state = 0;
	register int16_t sock = 0;
	register int16_t len = 0;
	register int ch;
	register bool skip = false;
	char * start = line;

	for (;;)
	{
		ch = *line++;
		if (ch == '\0')
			break;
		if (ch == ':')
		{
			if ((line - start) >= MAX_IPD_SIZE)
				skip = true;
			token = line;	//  <data>
			while (len != 0 && *line != '\0')
			{
				++line;
				--len;
			}
			if (len != 0)
			{	// Need load additional data
				EspTimer = 300;
				while (EspTimer != 0)
				{	// Load request data tail
					if ((ch = UART2_Get()) < 0)
					{
						__WFI();
						continue;
					}
#ifdef USE_ESP_LOG
					UART1_Putc(ch);
#endif
					EspTimer = 300;
					if (!skip)
						*line++ = ch;
					--len;
					if (len == 0)
					{	// Process multi line requests
						if (!skip)
						{
							*line = '\0';
							ESP_ProcessRequest(sock, token);
						}
						break;
					}
				}
			}
			else
			{	// Process single line requests
				DEBUG("RECV IPD SINGLE\r\n");
				if (!skip)
				{
					*line = '\0';
					ESP_ProcessRequest(sock, token);
				}
			}
			break;
		}
		if (ch != ',')
			continue;
		switch (state)
		{
		case 0:	// +IPD,
			break;
		case 1:	// <link ID>,
			sock = atoi(token);
			if (sock >= MAX_SOCKETS)
			{
				DEBUG("ERROR: Socket number too much\r\n");
				skip = true;
			}
			break;
		case 2:	// <len>,
			len = atoi(token);
			if (len >= MAX_REQUEST_SIZE)
			{
				DEBUG("ERROR: Request buffer too short\r\n");
				skip = true;
			}
			break;
		case 3:	// <remote IP>,<remote port>
			// Not need now
			break;
		}
		token = line;
		++state;
	}
}

/*
 *
 */
void ESP_SendProcessing(void)
{
	static uint8_t cmd[30];

	register int idx;
	register int len = 0;
	register ESP_Socket_t * esp_socket = ESP_Sockets;

	for (idx = 0; idx < MAX_SOCKETS; idx++)
	{
		if (esp_socket->HeaderLength != 0)
		{
			Esp_SendActive = idx;
			len = snprintf((char *)cmd, sizeof(cmd), "AT+CIPSEND=%d,%u\r\n", idx, esp_socket->HeaderLength);
			break;
		}
		if (esp_socket->ContentLength != 0)
		{
			Esp_SendActive = idx;
			len = snprintf((char *)cmd, sizeof(cmd), "AT+CIPSEND=%d,%u\r\n", idx, esp_socket->ContentLength);
			break;
		}
		if (esp_socket->Content_cb != NULL)
		{
			if (esp_socket->Content_cb(ESP_CB_BEFORE, esp_socket))
			{
				if (esp_socket->ContentLength != 0)
				{
					if (esp_socket->ContentLength == CONTENT_LAST_CHUNK)
					{	// Last chunk
						Esp_SendActive = idx;
						len = snprintf((char *)cmd, sizeof(cmd), "AT+CIPSEND=%d,%u\r\n", idx, 5);
					}
					else
					{
						len = sprintf((char *)esp_socket->Header, "%x\r\n", esp_socket->ContentLength);
						Esp_SendActive = idx;
						len = snprintf((char *)cmd, sizeof(cmd), "AT+CIPSEND=%d,%u\r\n", idx, esp_socket->ContentLength + len + 2);
					}
					break;
				}
			}
		}
		esp_socket++;
	}
	
	if (len > 0)
	{
		Esp_SendState = ESP_SEND_RESPONSE;
		UART2_SendNoEcho(cmd, len);
		EspSendTimer = 15000;
	}
}

void ESP_Processing()
{
	register int data;
	register char *line = (char *)ESP_Line;
	
	if (Esp_SendState == ESP_SEND_FREE)
		ESP_SendProcessing();
	else if (EspSendTimer == 0)
	{
		Esp_SendState = ESP_SEND_FREE;
		ESP_LineIndex = 0;
		while (!ESP_Init())
			HAL_Delay(5000);
	}

	while ((data = UART2_Get()) >= 0)
	{
#ifdef USE_ESP_LOG
		UART1_Putc(data);
#endif

		if (Esp_SendState == ESP_SEND_RESPONSE && ESP_LineIndex == 0 && data == '>')
		{
			register ESP_Socket_t * esp_socket = &ESP_Sockets[Esp_SendActive];
			if (esp_socket->HeaderLength != 0)
				UART2_SendNoEcho(esp_socket->Header, esp_socket->HeaderLength);
			else if (esp_socket->ContentLength != 0)
			{
				if (esp_socket->Content_cb != NULL && esp_socket->Content_cb(ESP_CB_PRESEND, esp_socket))
				{
					if (esp_socket->ContentLength == CONTENT_LAST_CHUNK)
					{
						UART2_SendNoEcho((uint8_t *)"0\r\n\r\n", 5);
						esp_socket->Content_cb = NULL;
					}
					else
					{
						UART2_SendNoEcho(esp_socket->Header, strlen((char *)esp_socket->Header));
						UART2_SendNoEcho(esp_socket->Content, esp_socket->ContentLength);
						UART2_SendNoEcho((uint8_t *)"\r\n", 2);
					}
				}
				else
					UART2_SendNoEcho(esp_socket->Content, esp_socket->ContentLength);
			}
			ESP_LineIndex = 0;
			continue;
		}
		if (ESP_LineIndex >= STRING_SIZE(ESP_Line))                     // Контролируем длину строки, защита от слишком большой
		{	// Skip long lines
			if (data == '\n')
				ESP_LineIndex = 0;
			continue;
		}

		line[ESP_LineIndex++] = data;
		if (data != '\n')                                               // Контроль окончания строки
			continue;

#ifdef USE_ESP_LOG
		UART1_Send(ESP_Line, ESP_LineIndex);
#endif
		line[ESP_LineIndex] = '\0';                                     // Строка кончилась
                
                
		if (Esp_SendState == ESP_SEND_RESPONSE)                         // Начинаем обрабатывать что принялось
		{
			register ESP_Socket_t * esp_socket = &ESP_Sockets[Esp_SendActive];
			if (ESP_LineIndex == 9 && strcmp((char *)line, "SEND OK\r\n") == 0)
			{
				if (esp_socket->HeaderLength != 0)
					esp_socket->HeaderLength = 0;
				else if (esp_socket->ContentLength != 0)
					esp_socket->ContentLength = 0;
				Esp_SendState = ESP_SEND_FREE;
				ESP_LineIndex = 0;
				continue;
			}
			else if ((ESP_LineIndex == 7 && strcmp((char *)line, "ERROR\r\n") == 0)
				||	(ESP_LineIndex == 11 && strcmp((char *)line, "SEND FAIL\r\n") == 0))
			{
				esp_socket->HeaderLength = 0;
				esp_socket->ContentLength = 0;
				if (esp_socket->Content_cb != NULL)
					esp_socket->Content_cb(ESP_CB_ERROR, esp_socket);
				esp_socket->Content_cb = NULL;

				Esp_SendState = ESP_SEND_FREE;
				ESP_LineIndex = 0;
				continue;
			}
		}

//		if (ESP_LineIndex > 5 && strncmp((char *)line, "+IPD,", STRING_SIZE("+IPD,")) == 0)
//		{
//			ESP_ParseIPD(line);
//		}
                if (ESP_LineIndex > 5 && strncmp((char *)line, "{\"dat", STRING_SIZE("{\"dat")) == 0)
		{
//                    if (strstr((char *)line, "{\"data\":{\"deviceID\":\"")){
//                          memcpy(R.DeviceID, (char *)line+21, 36);
//                          *((char *)line + 21 + 36) = 0;
//                          __no_operation();
//                          __no_operation();
//                          __no_operation();
//                          __no_operation();
//                          __no_operation(); 
//                    }   
		}
		else if (ESP_LineIndex == 11 && strcmp((char *)&line[1], ",CONNECT\r\n") == 0)
		{
		}
		else if (ESP_LineIndex == 10 && strcmp((char *)&line[1], ",CLOSED\r\n") == 0)
		{	// Clear socket
			uint32_t sock = atoi(line);
			if (sock < MAX_SOCKETS)
			{
				register ESP_Socket_t * esp_socket = &ESP_Sockets[sock];
				esp_socket->HeaderLength = 0;
				esp_socket->ContentLength = 0;
				if (esp_socket->Content_cb != NULL)
					esp_socket->Content_cb(ESP_CB_ERROR, esp_socket);
				esp_socket->Content_cb = NULL;
			}
		}
		else if (ESP_LineIndex == 15 && strcmp((char *)&line[1], ",CONNECT FAIL\r\n") == 0)
		{	// Clear socket
			uint32_t sock = atoi(line);
			if (sock < MAX_SOCKETS)
			{
				register ESP_Socket_t * esp_socket = &ESP_Sockets[sock];
				esp_socket->HeaderLength = 0;
				esp_socket->ContentLength = 0;
				if (esp_socket->Content_cb != NULL)
					esp_socket->Content_cb(ESP_CB_ERROR, esp_socket);
				esp_socket->Content_cb = NULL;
			}
		}
                
                
		ESP_LineIndex = 0;                                              // while ((data = UART2_Get()) >= 0)
	}			
}




void BT_Processing(void)
{
	register int data;
	register char *line = (char *)BT_Line;
	

	while ((data = UART1_Get()) >= 0)                                       // Забираем из буфера пока хвост не догонит голову
	{

		if (BT_LineIndex >= STRING_SIZE(BT_Line)){                       // Контролируем длину строки, защита от слишком большой
			if (data == '\n')                                       // Skip long lines
				BT_LineIndex = 0;
			continue;
		}

		line[BT_LineIndex++] = data;
		if (data != '\n') continue;                                     // На следующий символ, если строка не кончилась
			

		line[BT_LineIndex] = '\0';                                      // Строка кончилась, явно обозначаем ее конец
                

		if (BT_LineIndex > 4 && strncmp((char *)line, "AT+", STRING_SIZE("AT+")) == 0){
			BT_ParseCommand(line);
		}
                
                if (BT_LineIndex == 4 && strncmp((char *)line, "AT\r", STRING_SIZE("AT\r")) == 0){
			DEBUG("\nOK\n");
		}
                

                
		BT_LineIndex = 0;                                               // while ((data = UART2_Get()) >= 0)
	}			
}







ESP_Error_t ESP_WaitResponse()
{
	return ESP_WaitResponseEx(2000, NULL);
}
ESP_Error_t ESP_WaitResponse_cb(sendcmd_cb_t cb)
{
	return ESP_WaitResponseEx(2000, cb);
}

ESP_Error_t SendCmd(const char * cmd)
{
	int length = strlen(cmd);
	UART2_Send((uint8_t *)cmd, length);
	return ESP_WaitResponse();
}
ESP_Error_t SendCmdEx(const char * cmd, int timeout)
{
	int length = strlen(cmd);
	UART2_Send((uint8_t *)cmd, length);
	return ESP_WaitResponseEx(timeout, NULL);
}

ESP_Error_t SendCmd_cb(const char * cmd, sendcmd_cb_t cb)                       // Колбэк на ответ bool (*sendcmd_cb_t)(uint8_t *response);
{
	int length = strlen(cmd);
	UART2_Send((uint8_t *)cmd, length);
	return ESP_WaitResponse_cb(cb);
}

#define ESP_AT_VERSION	"AT version:"
bool ATGMR_Response(uint8_t * response)
{
	if (strncmp((const char *)response, ESP_AT_VERSION, STRING_SIZE(ESP_AT_VERSION)) == 0)
	{
		ESP_AT_Version = 0
			| response[STRING_SIZE(ESP_AT_VERSION) + 0] << 24
			| response[STRING_SIZE(ESP_AT_VERSION) + 2] << 16
			| response[STRING_SIZE(ESP_AT_VERSION) + 4] << 8
			| response[STRING_SIZE(ESP_AT_VERSION) + 6] << 0;
	}
	return false;
}

int ESP_StatusInt = 0;
#define ESP_STATUS		"STATUS:"
#define ESP_CIPSTATUS	"+CIPSTATUS:"
bool ATCIPSTATUS_Response(uint8_t * response)
{
	if (strncmp((char *)response, ESP_STATUS, STRING_SIZE(ESP_STATUS)) == 0)
	{
		ESP_StatusInt = atoi((char *)response + STRING_SIZE(ESP_STATUS));
	}
	else if (strncmp((char *)response, ESP_CIPSTATUS, STRING_SIZE(ESP_CIPSTATUS)) == 0)
	{
		
	}		
	return false;
}

/*
 *
 */
int ESP_Status()
{
	if (SendCmd_cb("AT+CIPSTATUS\r\n", ATCIPSTATUS_Response) != ESP_RESPONSE_OK)
		ESP_StatusInt = -1;
	return ESP_StatusInt;
}

/*
 *	Check ESP firmware version
 */
uint32_t ESP_Version()
{
	return ESP_AT_Version;
}

/*
 *
 */
bool ATRST_Response(uint8_t * response)
{
	return (strcmp((char *)response, "ready") == 0);
}

/*
 *
 */
bool ESP_Reset()
{
	if (SendCmd("AT+RST\r\n") == ESP_RESPONSE_OK)
		if (ESP_WaitResponseEx(5000, ATRST_Response) == ESP_RESPONSE_OK)
		{
			SendCmd("ATE0\r\n");
			SendCmd_cb("AT+GMR\r\n", ATGMR_Response);
			SendCmd("AT+CWMODE=1\r\n");
			HAL_Delay(200);
			SendCmd("AT+CIPMUX=1\r\n");
			SendCmd("AT+CIPDINFO=1\r\n");
			SendCmd("AT+CWAUTOCONN=0\r\n");
			SendCmd("AT+CWDHCP=1,1\r\n");
			HAL_Delay(200);
			return true;
		}
	return false;
}

/*
 *
 */
bool ESP_Connect()
{
    // connect to access point, use CUR mode to avoid connection at boot
        sprintf (TmpStr, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", S.AP1.SSID, S.AP1.Pass);
	return (SendCmdEx(TmpStr, 20000) == ESP_RESPONSE_OK);
}



//bool CWJAP?_Response(uint8_t * response)
//{
//	if (strncmp((const char *)response, ESP_AT_VERSION, STRING_SIZE(ESP_AT_VERSION)) == 0)
//	{
//		ESP_AT_Version = 0
//			| response[STRING_SIZE(ESP_AT_VERSION) + 0] << 24
//			| response[STRING_SIZE(ESP_AT_VERSION) + 2] << 16
//			| response[STRING_SIZE(ESP_AT_VERSION) + 4] << 8
//			| response[STRING_SIZE(ESP_AT_VERSION) + 6] << 0;
//	}
//	return false;
//}
//
///*
// *
// */
//bool ESP_Status(void){
//  
//        SendCmd_cb("AT+CWJAP_CUR?\r\n", CWJAP?_Response)
//          
//        if (SendCmd_cb("AT+CWJAP_CUR?\r\n", CWJAP?_Response) == ESP_RESPONSE_OK)
//              {
//                      __no_operation();
//                      return true;
//              }
//        return false;
//}


/*
 *
 */
bool ESP_StartAP()
{
    if (SendCmdEx("AT+CWMODE_CUR=2\r\n", 10000) == ESP_RESPONSE_OK)
	{
		if (SendCmdEx("AT+CWSAP_CUR=\"SDWIFI\",\"passWord\",10,4\r\n", 10000) == ESP_RESPONSE_OK)
		{
			SendCmd("AT+CWDHCP_CUR=0,1\r\n");
			return true;
		}
	}
	return false;
}

/*
 *
 */
static char localIP[16];
static char localMAC[18];

#define ESP_CIPAP	"+CIPAP:ip:\""
#define ESP_STAIP	"+CIFSR:STAIP,\""
#define ESP_STAMAC	"+CIFSR:STAMAC,\""
bool ATCIPAP_Response(uint8_t * response)
{
	register char * src = NULL;
	register char * dst = localIP;

	if (strncmp((char *)response, ESP_CIPAP, STRING_SIZE(ESP_CIPAP)) == 0)
		src = (char *)response + STRING_SIZE(ESP_CIPAP);
	else if (strncmp((char *)response, ESP_STAIP, STRING_SIZE(ESP_STAIP)) == 0)
		src = (char *)response + STRING_SIZE(ESP_STAIP);
	else if (strncmp((char *)response, ESP_STAMAC, STRING_SIZE(ESP_STAMAC)) == 0)
	{
		src = (char *)response + STRING_SIZE(ESP_STAMAC);
		dst = localMAC;
	}

	if (src != NULL)
	{
		while (*src != '\0' && *src != '\"')
			*dst++ = *src++;
		*dst = '\0';
	}
	return false;
}

char * ESP_LocalIP()
{
	if (SendCmd_cb("AT+CIPAP?\r\n", ATCIPAP_Response) != ESP_RESPONSE_OK)
		strcpy(localIP, "UNKNOWN");
	return localIP;
}

char * ESP_StationIP()
{
	if (SendCmd_cb("AT+CIFSR\r\n", ATCIPAP_Response) != ESP_RESPONSE_OK)
		strcpy(localIP, "UNKNOWN");
	return localIP;
}

/*
 *
 */
bool ESP_ConfigAP()
{
	SendCmd("AT+CWMODE_CUR=2\r\n");
	// disable station DHCP
	SendCmd("AT+CWDHCP_CUR=2,0\r\n");
	
	return (SendCmdEx("AT+CIPAP_CUR=\"192.168.20.1\"\r\n", 2000) == ESP_RESPONSE_OK);
}

/*
 *	Create server on port 80
 */
bool ESP_CreateServer()
{
	return (SendCmd("AT+CIPSERVER=1,80\r\n") == ESP_RESPONSE_OK);
}


//ESP_Error_t SendCmd_cb(const char * cmd, sendcmd_cb_t cb)
// bool (*sendcmd_cb_t)(uint8_t *response);

//bool get_IP (uint8_t *responce) {
//uint8_t *Ptr = 0;
//uint8_t ii;
//
//  R.Ptr = responce; // Это начало сообщения ответного +CIPDOMAIN:109.194.115.162.89
//  
//  Ptr = strstr(responce, "+CIPDOMAIN:");
//  if (Ptr) { 
//    Ptr += strlen("+CIPDOMAIN:");
//    
//    ii = 0;
//    while (*Ptr != '\r'){
//        if (ii>16) { break; }
//        R.ESP_IP_1[ii]=*Ptr;   Ptr++;   ii++;
//    }
//    R.ESP_IP_1[ii] = 0;
//    
//    
//      __no_operation();
//      __no_operation();
//      __no_operation();
//      __no_operation();
//      __no_operation();
//  }
//
//
//  
//return 0;
//}


bool ESP_Init()
{
	int retries = 5;
	bool set230 = true;
	memset(ESP_Sockets, 0, sizeof(ESP_Sockets));
	ESP_Sockets[0].Index = 0;
	ESP_Sockets[1].Index = 1;
	ESP_Sockets[2].Index = 2;
	ESP_Sockets[3].Index = 3;

	while (retries != 0)
	{
		if (SendCmd("AT\r\n") == ESP_RESPONSE_OK)
		{
			if (ESP_Reset())
			{
				if (set230)
				{
					set230 = false;
					DEBUG("SET 230400\n");
					if (SendCmd("AT+UART_CUR=230400,1,0,0\r\n") == ESP_RESPONSE_OK)
					{
						UART2_SetBaudrate();
						retries = 5;
						while (retries != 0)
						{
							if (SendCmd("AT\r\n") == ESP_RESPONSE_OK)
								break;
							--retries;
							HAL_Delay(1000);
						}
						if (retries == 0)
						{
							UART2_ReInit();
							retries = 5;
							continue;
						}
					}
				}
				
				ESP_Status();
				// Station
				while (ESP_StatusInt != 2 && ESP_StatusInt != 3 && ESP_StatusInt != 4)
				{
					printf("Connect to Access Point\n");
					if (ESP_Connect() == true) __LED_NET(1), R.WiFi_Connected = true;
                                        else __LED_NET(0), R.WiFi_Connected = false;
                                        
                                        
                                        SendCmdEx("AT+CIPDNS_CUR=1,\"208.67.222.222\"\r\n", 10000);
                                        //SendCmdEx("AT+CIPDNS_CUR?\r\n", 20000);
                                        
                                        //SendCmd_cb("AT+CIPDOMAIN=\"motor-diag.7bits.it\"\r\n", get_IP);
                                        
					HAL_Delay(500);
					ESP_Status();
				}
				printf("Station IP:%s\n", ESP_StationIP());
				if (ESP_CreateServer())
					return true;

				// AP
//				ESP_ConfigAP();
//				if (ESP_StartAP())
//				{
//					ESP_LocalIP();
//					if (ESP_CreateServer())
//						return true;
//				}

			}
		}
		--retries;
		HAL_Delay(500);
		UART2_Send((uint8_t *)"+++", 3);
		HAL_Delay(500);
		if (set230 == false)
		{
			DEBUG("SET 115200\n");
			UART2_SetBaudrate();
			SendCmd("AT+UART_CUR=115200,8,1,0,0\r\n");
			UART2_ReInit();
		}
		set230 = !set230;
	}
	return false;
}

/*
 *
 */
void ESP_DeInit()
{
	
}



bool get_DevID (uint8_t *line) {

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








// Отправить пакет с измерениями
void ESP_Send_Packet (SEND_TYPE_e Type){


  switch (Type) {                                   
      case NO_SEND:       /*osThreadYield();*/          break;
      case GET_DEVIDSRV:    get_DevID_from_server();    break;
      case TXT_FREQ:        txt_freq_send();            break;
      case TXT_RARE:        txt_rare_send();            break;
      case BIN_RARE:        bin_rare_send();            break;
      default:                                          break;
  }

}
