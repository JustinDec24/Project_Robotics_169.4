/*============================================================================
 * main.c — application firmware entry point.
 *
 * Both REMOTE (PC card) and ROBOT roles run the same code: a tiny shim
 * that brings up the board peripherals (board / uart / spi / timer /
 * interrupts) then hands off to the application state machine in
 * app/app.c. Role is selected at compile time via MODEM_ROLE.
 *
 * Air interface: 169.4 MHz, 2-GFSK, 4.8 kbps, 5 kHz dev, 100 kHz RX BW,
 * +14 dBm TX, 32-bit sync (SmartRF Studio config).
 *===========================================================================*/
#include <xc.h>
#include "config.h"
#include "board/board.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/timer.h"
#include "radio/cc1120.h"
#include "radio/cc1120_regs.h"

#include "app/app.h"
#if MODEM_ROLE == MODEM_ROLE_REMOTE
#include "protocol/protocol.h"
#endif

int main(void)
{
    board_init();
    uart_init(UART_BAUD_DEFAULT);
    spi_init();
    timer_init();
    global_int_enable();
    Delay_ms(200);

#if MODEM_ROLE == MODEM_ROLE_REMOTE
    {
        const char *p = "\r\n[remote] modem firmware booting\r\n";
        while (*p) uart_write((uint8_t)*p++);
    }
#endif

    app_init();
    for (;;) {
        app_task();
    }
    return 0;
}
