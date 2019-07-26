#ifndef __RENG_BUFFER_H__
#define __RENG_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t *Buffer;
	uint16_t Head;
	uint16_t Tail;
	uint16_t Size;
	bool Overflow;
} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t * ring, uint8_t *buffer, uint16_t size);
bool RingBuffer_Put(RingBuffer_t * ring, uint8_t data);
int RingBuffer_Get(RingBuffer_t * ring);

#ifdef __cplusplus
}
#endif

#endif
