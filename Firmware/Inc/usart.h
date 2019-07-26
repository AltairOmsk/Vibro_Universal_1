#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx_hal.h"
#include "ring_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

	UART_HandleTypeDef *Uart;
	void (*ReInit)(void);
	RingBuffer_t RxBuffer;
	uint8_t RxIrqBuffer[4];
	uint32_t TxSize;
	uint32_t TxIndex;
	uint8_t *TxBuffer1;
	uint8_t *TxBuffer2;
	bool TxUse;
	volatile bool TxBusy;

} UART_Context_t;

int stdout_putchar(int ch);
void stdout_print(const uint8_t *msg);

//#define DEBUG(x)				stdout_print((uint8_t *)x)
#define stdout_puts(msg, size)	UART1_Send(msg, size)
#define stdout_putchar_int(ch)	UART1_Putc(ch)

void UART1_ReInit(void);
void UART2_ReInit(void);

// uint32_t UART1_ErrorCode(void);
// uint32_t UART1_ErrorCode(void);

int UART1_Get(void);
int UART2_Get(void);

void UART1_SetBaudrate(void);
void UART2_SetBaudrate(void);

bool UART1_TxBusy(void);
bool UART2_TxBusy(void);

bool UART1_Send(const uint8_t *msg, size_t size);
bool UART2_Send(const uint8_t *msg, size_t size);
bool UART2_SendNoEcho(const uint8_t *msg, size_t size);
int UART1_Putc(int ch);
int UART2_Putc(int ch);
void UART1_Puts(uint8_t *msg);
void UART2_Puts(uint8_t *msg);

void USART_Rx_Callback(register UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
