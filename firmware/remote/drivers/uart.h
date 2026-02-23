/*============================================================================
 * drivers/uart.h — Interrupt-driven UART driver
 *
 * RX: interrupt-driven into a ring buffer.
 * TX: polled / blocking (simple and sufficient for this use case).
 *===========================================================================*/
#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void uart_init(uint32_t baud);

void uart_write(uint8_t b);
void uart_write_bytes(const uint8_t *data, size_t len);

bool uart_read_byte_nonblocking(uint8_t *out);

uint16_t uart_rx_available(void);

/* Called from ISR — pushes one received byte into the RX ring buffer. */
void isr_uart_rx_byte(uint8_t b);

#endif /* UART_H */
