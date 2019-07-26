#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#include "main.h"
#include "usart.h"

/**
  Put a character to the stdout
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/

int stdout_putchar(int ch)
{
	if (ch == '\n')
		stdout_putchar ('\r');

	stdout_putchar_int (ch);
	return (ch);
}

void stdout_print(const uint8_t *msg)
{
	while (*msg != 0)
		stdout_putchar(*msg++);
}

int fputc(int ch)
{
	stdout_putchar(ch);
	return ch;
}
