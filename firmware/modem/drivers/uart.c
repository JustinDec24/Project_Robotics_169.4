/*============================================================================
 * drivers/uart.c — Interrupt-driven UART driver implementation
 *===========================================================================*/
#include "uart.h"
#include "../board/board.h"
#include "../config.h"
#include "../util/ringbuf.h"

RINGBUF_DEF(uart_rx_rb, UART_RX_BUF_SIZE);
RINGBUF_DEF(uart_tx_rb, UART_TX_BUF_SIZE);

void uart_init(uint32_t baud)
{
    RINGBUF_CLEAR(uart_rx_rb);
    RINGBUF_CLEAR(uart_tx_rb);
    board_uart_hw_init(baud);
}

static void uart_tx_kick(void)
{
    /* If hardware TX register is currently empty, there is no pending TX
     * interrupt to fire. We must seed one byte and (re-)enable TXIE so the
     * ISR drains the rest. */
    uint8_t tok = board_int_save_disable();
    if (!RINGBUF_EMPTY(uart_tx_rb) && board_uart_tx_ready()) {
        uint8_t b;
        RINGBUF_POP(uart_tx_rb, b);
        board_uart_tx_byte_raw(b);
    }
    if (!RINGBUF_EMPTY(uart_tx_rb)) {
        board_uart_tx_irq_enable();
    } else {
        board_uart_tx_irq_disable();
    }
    board_int_restore(tok);
}

void uart_write(uint8_t b)
{
    /* Wait for room in the TX ring (busy-wait, but short: the ISR drains
     * one byte per UART character time = ~87 us at 115200). */
    while (1) {
        uint8_t tok = board_int_save_disable();
        bool full = (((uart_tx_rb.head + 1u) & (sizeof(uart_tx_rb.buf) - 1u))
                     == uart_tx_rb.tail);
        if (!full) {
            uart_tx_rb.buf[uart_tx_rb.head] = b;
            uart_tx_rb.head = (uart_tx_rb.head + 1u)
                              & (sizeof(uart_tx_rb.buf) - 1u);
            board_int_restore(tok);
            break;
        }
        board_int_restore(tok);
        /* Buffer full: yield — the ISR will drain. */
    }
    uart_tx_kick();
}

void uart_write_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uart_write(data[i]);
    }
}

bool uart_read_byte_nonblocking(uint8_t *out)
{
    uint8_t tok = board_int_save_disable();
    if (RINGBUF_EMPTY(uart_rx_rb)) {
        board_int_restore(tok);
        return false;
    }
    RINGBUF_POP(uart_rx_rb, *out);
    board_int_restore(tok);
    return true;
}

uint16_t uart_rx_available(void)
{
    uint16_t n;
    uint8_t  tok = board_int_save_disable();
    n = RINGBUF_COUNT(uart_rx_rb);
    board_int_restore(tok);
    return n;
}

uint16_t uart_tx_free(void)
{
    uint16_t n;
    uint8_t  tok = board_int_save_disable();
    n = (uint16_t)(sizeof(uart_tx_rb.buf) - 1u - RINGBUF_COUNT(uart_tx_rb));
    board_int_restore(tok);
    return n;
}

/* ---- ISR entry-points -----------------------------------------------------*/

void isr_uart_rx_byte(uint8_t b)
{
    /* Drop on overflow rather than blocking the ISR. */
    RINGBUF_PUSH(uart_rx_rb, b);
}

bool isr_uart_tx_next(uint8_t *b)
{
    if (RINGBUF_EMPTY(uart_tx_rb)) {
        return false;
    }
    RINGBUF_POP(uart_tx_rb, *b);
    return true;
}
