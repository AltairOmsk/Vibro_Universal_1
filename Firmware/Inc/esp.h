#ifndef __ESP_H__
#define __ESP_H__

#include "usart.h"
#include "main.h"
#include "work.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  
#define STRING_SIZE(x)		(sizeof(x) - 1)

// Maxmium number of socket
#define	MAX_SOCKETS			4
#define MAX_IPD_SIZE		128
#define MAX_REQUEST_SIZE	1024

#define CONTENT_LAST_CHUNK	0xFFFF

// Positive values must be the the same as ESP_Responses
typedef enum {
	ESP_RESPONSE_OK		= 0,
	ESP_RESPONSE_ERROR	= 1,
	ESP_RESPONSE_FAIL	= 2,
	ESP_RESPONSE_IPD	= 3,
	ESP_ERR_TOOLONG		= -1,
	ESP_ERR_TIMEOUT		= -2
} ESP_Error_t;

typedef struct {
	const char *Response;
	const ESP_Error_t Code;
} ESP_Response_t;

static const ESP_Response_t ESP_Responses[] = {
	{ "OK",		ESP_RESPONSE_OK },
	{ "ERROR",	ESP_RESPONSE_ERROR },
	{ "FAIL",	ESP_RESPONSE_FAIL },
	{ NULL,		ESP_RESPONSE_OK }
};

/* Encryption modes */
typedef enum {
	ENC_TYPE_NONE = 0,
	ENC_TYPE_WEP = 1,
	ENC_TYPE_WPA_PSK = 2,
	ENC_TYPE_WPA2_PSK = 3,
	ENC_TYPE_WPA_WPA2_PSK = 4
} ESP_ENC_Type_t;

typedef enum {
	ESP_CB_BEFORE = 0,
	ESP_CB_PRESEND,
	ESP_CB_ERROR
} ESP_CB_TYPE_t;

extern volatile uint32_t EspTimer;
extern volatile uint32_t EspSendTimer;

typedef bool (*sendcmd_cb_t)(uint8_t *response);
typedef bool (*Content_cb_t)(ESP_CB_TYPE_t, void * content);

typedef struct {
	int Index;
	uint16_t HeaderLength;
	uint16_t ContentLength;
	Content_cb_t Content_cb;
	char Method[16];
	char Depth[16];
	char Uri[256];
	uint8_t Header[256];
	uint8_t Content[1024];
} ESP_Socket_t;

typedef enum {
	ESP_SEND_FREE,
	ESP_SEND_RESPONSE
} ESP_SendState_t;


 
  

extern REG_t R;  
extern MEAS_t M;
extern SETTINGS_t S;

extern uint32_t        DevID_Dw0;                                               // ”никальный ID контроллера
extern uint32_t        DevID_Dw1;
extern uint32_t        DevID_Dw2;

extern char            TmpStr[128];
  





  
bool            ESP_Init                (void);
void            ESP_DeInit              (void);
uint32_t        ESP_Version             (void);
void            ESP_Processing          (void);
void            ESP_Send_Packet         (SEND_TYPE_e Type);
ESP_Error_t     SendCmd                 (const char * cmd);
ESP_Error_t     SendCmd_cb              (const char * cmd, sendcmd_cb_t cb);
void            BT_Processing           (void);

#ifdef __cplusplus
}
#endif

#endif
