#include "main.h"
#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t * ring, uint8_t *buffer, uint16_t size)
{
	ring->Size = 0;
	ring->Head = 0;
	ring->Tail = 0;
	ring->Overflow = false;
	ring->Buffer = buffer;
	ring->Size = size;
}

bool RingBuffer_Put(RingBuffer_t * ring, uint8_t data)
{
	if (ring->Size != 0)
	{
		register uint16_t head = ring->Head;
		ring->Buffer[head] = data;
		head++;
		if (head == ring->Size)
			head = 0;
		if (head == ring->Tail)
		{
			ring->Overflow = true;
			return false;
		}
		ring->Head = head;
	}
	return true;
}

int RingBuffer_Get(RingBuffer_t * ring)
{
	register uint8_t data;
	register uint16_t tail = ring->Tail;

	if (ring->Size != 0)
	{
		if (tail != ring->Head)
		{
			data = ring->Buffer[tail];
			tail++;
			if (tail == ring->Size)
				tail = 0;
			ring->Tail = tail;
			return (int)data;
		}
	}
	return -1;
}
