#include "main.h"
#include "usart.h"

void USART1_UART_Init(void);
void USART2_UART_Init(void);

uint8_t UART1_RxBuffer [UART1_RX_SIZE];
uint8_t UART1_TxBuffer1[UART1_TX_SIZE];
uint8_t UART1_TxBuffer2[UART1_TX_SIZE];

uint8_t UART2_RxBuffer [UART2_RX_SIZE];
uint8_t UART2_TxBuffer1[UART2_TX_SIZE];
uint8_t UART2_TxBuffer2[UART2_TX_SIZE];

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

UART_Context_t UART1_Context = {
	.Uart = &huart1,
	.ReInit = &UART1_ReInit,
	.TxSize = UART1_TX_SIZE,
	.TxIndex = 0,
	.TxBuffer1 = &UART1_TxBuffer1[0],
	.TxBuffer2 = &UART1_TxBuffer2[0],
	.TxBusy = false,
	.TxUse = false
};

UART_Context_t UART2_Context = {
	.Uart = &huart2,
	.ReInit = &UART2_ReInit,
	.TxSize = UART2_TX_SIZE,
	.TxIndex = 0,
	.TxBuffer1 = &UART2_TxBuffer1[0],
	.TxBuffer2 = &UART2_TxBuffer2[0],
	.TxBusy = false,
	.TxUse = false
};

uint32_t UART1_ErrorCode()
{
	return UART1_Context.Uart->ErrorCode;
}
uint32_t UART2_ErrorCode()
{
	return UART2_Context.Uart->ErrorCode;
}

int UART1_Get()
{
	return RingBuffer_Get(&UART1_Context.RxBuffer);
}
int UART2_Get()
{
	return RingBuffer_Get(&UART2_Context.RxBuffer);
}

bool UART1_TxBusy()
{
	return UART1_Context.TxBusy;
}
bool UART2_TxBusy()
{
	return UART2_Context.TxBusy;
}

void USART_Rx_Callback(register UART_HandleTypeDef *huart)
{
	register UART_Context_t *context = (huart == &huart1) ? &UART1_Context : &UART2_Context;
	huart->pRxBuffPtr--;
	huart->RxXferCount++;
	RingBuffer_Put(&context->RxBuffer, *huart->pRxBuffPtr);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart1)
		UART1_Context.TxBusy = false;
	else if (huart == &huart2)
		UART2_Context.TxBusy = false;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	register UART_Context_t *context = (huart == &huart1) ? &UART1_Context : &UART2_Context;
	HAL_UART_TxCpltCallback(huart);
	HAL_UART_Receive_IT(context->Uart, &context->RxIrqBuffer[0], sizeof(context->RxIrqBuffer));
}

static void UART_ReInit(UART_Context_t *context)
{
	context->TxIndex = 0;
	context->TxBusy = false;
	context->TxUse = false;

	HAL_UART_Receive_IT(context->Uart, &context->RxIrqBuffer[0], sizeof(context->RxIrqBuffer));
}

void UART1_ReInit()
{
	register UART_Context_t *context = &UART1_Context;

	HAL_UART_DeInit(context->Uart);
	RingBuffer_Init(&context->RxBuffer, UART1_RxBuffer, UART1_RX_SIZE);
	USART1_UART_Init();
	UART_ReInit(context);
}

void UART2_SetBaudrate(void)
{
	register UART_Context_t *context = &UART2_Context;

	HAL_UART_DeInit(context->Uart);
	RingBuffer_Init(&context->RxBuffer, UART2_RxBuffer, UART2_RX_SIZE);
	UART2_UART_InitBaud();
	UART_ReInit(context);
}

void UART2_ReInit()
{
	register UART_Context_t *context = &UART2_Context;

	HAL_UART_DeInit(context->Uart);
	RingBuffer_Init(&context->RxBuffer, UART2_RxBuffer, UART2_RX_SIZE);
	USART2_UART_Init();
	UART_ReInit(context);
}

static HAL_StatusTypeDef UART_Send(const uint8_t *msg, size_t size, register UART_Context_t *context)
{
	register uint32_t timeout = 500;
	while (context->TxBusy && timeout != 0)
	{
		--timeout;
		HAL_Delay(10);
	}

	if (timeout == 0 || context->Uart->ErrorCode != HAL_UART_ERROR_NONE)
		context->ReInit();

	context->TxBusy = true;
	return HAL_UART_Transmit_DMA(
		context->Uart,
		(uint8_t *)msg,
		size
	);
}

bool UART1_Send(const uint8_t *msg, size_t size)
{
	return (HAL_OK != UART_Send(msg, size, &UART1_Context));
}

bool UART2_Send(const uint8_t *msg, size_t size)
{
#ifdef USE_ESP_LOG
	UART1_Send(msg, size);
#endif
	return (HAL_OK != UART_Send(msg, size, &UART2_Context));
}

//bool UART2_SendNoEcho(const uint8_t *msg, size_t size)
//{
//#ifdef USE_ESP_LOG
//	UART1_Send(msg, size);
//#endif
//	return (HAL_OK != UART_Send(msg, size, &UART2_Context));
//}


bool UART2_SendNoEcho(const uint8_t *msg, size_t size)
{
  return (HAL_OK != UART_Send(msg, size, &UART2_Context));
}

static int UART_Putc(int ch, register UART_Context_t *context)
{
	register uint8_t *dst = context->TxUse ? context->TxBuffer2 : context->TxBuffer1;
	dst[context->TxIndex++] = (uint8_t)ch;

	if (ch == '\n' || context->TxIndex >= context->TxSize)
	{
		UART_Send((const uint8_t *)dst, context->TxIndex, context);
		context->TxIndex = 0;
		context->TxUse = !context->TxUse;
	}
	return (ch);	
}

int UART1_Putc(int ch)
{
	return UART_Putc(ch, &UART1_Context);
}
int UART2_Putc(int ch)
{
	return UART_Putc(ch, &UART2_Context);
}
