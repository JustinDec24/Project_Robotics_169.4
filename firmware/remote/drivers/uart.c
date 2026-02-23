/*============================================================================
 * drivers/uart.c — Interrupt-driven UART driver implementation
 *===========================================================================*/
#include "uart.h"
#include "../board/board.h"
#include "../config.h"
#include "../util/ringbuf.h"

RINGBUF_DEF(uart_rx_rb, UART_RX_BUF_SIZE);

void uart_init(uint32_t baud)
{
    RINGBUF_CLEAR(uart_rx_rb);
    board_uart_hw_init(baud);
}

void uart_write(uint8_t b)
{
    board_uart_tx_byte(b);
}

void uart_write_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        board_uart_tx_byte(data[i]);
}

bool uart_read_byte_nonblocking(uint8_t *out)
{
    if (RINGBUF_EMPTY(uart_rx_rb))
        return false;
    RINGBUF_POP(uart_rx_rb, *out);
    return true;
}

uint16_t uart_rx_available(void)
{
    return RINGBUF_COUNT(uart_rx_rb);
}

void isr_uart_rx_byte(uint8_t b)
{
    RINGBUF_PUSH(uart_rx_rb, b);
}
