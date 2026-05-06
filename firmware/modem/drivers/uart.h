/*============================================================================
 * drivers/uart.h — Interrupt-driven UART driver
 *
 * RX: interrupt-driven into a ring buffer.
 * TX: interrupt-driven from a ring buffer (non-blocking writes).
 *
 * uart_write* functions return immediately after queueing data. If the TX
 * ring buffer is full they spin-wait until space frees up (which only happens
 * if interrupts are enabled and the TX ISR drains the buffer).
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
uint16_t uart_tx_free(void);

/* ---- ISR entry-points (called from board.c interrupt vector) -------------*/
void isr_uart_rx_byte(uint8_t b);   /* one received byte                    */
bool isr_uart_tx_next(uint8_t *b);  /* fetch next byte to send; false=>idle */

#endif /* UART_H */
