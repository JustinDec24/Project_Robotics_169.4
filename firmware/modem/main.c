/*============================================================================
 * main.c — PHASE 6: application layer wired in.
 *
 * Phase 5 confirmed that the carte PC can emit on 169.4 MHz. Now we hand
 * control to the application state machine in app/app.c so that the host
 * console (host_tools/modem_console) can drive the modem over the framed
 * UART protocol:
 *
 *   PC --(UART framing)--> Remote modem --(RF)--> Robot modem --(UART)--> SBC
 *
 * What this main does, regardless of role:
 *   1. board_init / uart_init / spi_init / timer_init / interrupts on
 *   2. small settle delay
 *   3. app_init() — clears state, prepares the role
 *   4. app_task() in a tight loop — drains UART, drains RF FIFO, runs
 *      periodics (beacons / connect timeout / ARQ tick / stats push)
 *
 * The CC1120 itself is initialised by app_task() the first time it runs
 * (state APP_INIT calls cc1120_init_minimal then radio_link_init).
 *===========================================================================*/
#include <xc.h>
#include "config.h"
#include "board/board.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/timer.h"
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

    /* Small wait so the FT231 USB-CDC link (or miniuart3 module on the
     * robot side) is up before we start spitting log frames. */
    Delay_ms(200);

#if MODEM_ROLE == MODEM_ROLE_REMOTE
    /* Send an unframed banner first so the user can see something
     * immediately when they open the console. The framed "REMOTE modem
     * ready" log will follow as soon as app_task() transitions to
     * APP_RF_READY. */
    {
        const char *banner =
            "\r\n[remote] modem firmware booting (Phase 6 — app layer)\r\n";
        const char *p = banner;
        while (*p) uart_write((uint8_t)*p++);
    }
#endif

    app_init();
    for (;;) {
        app_task();
    }

    return 0;
}
