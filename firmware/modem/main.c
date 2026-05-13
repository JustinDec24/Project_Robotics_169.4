/*============================================================================
 * main.c — Dual-role modem entry point
 *
 * Build with MODEM_ROLE=MODEM_ROLE_REMOTE for the PC-connected modem,
 * MODEM_ROLE=MODEM_ROLE_ROBOT for the underwater-robot-connected modem.
 *
 * Session flow (REMOTE side perspective):
 *   1) PC sends SCAN_START; remote reports SCAN_RESULT for each beaconing robot.
 *   2) PC sends CONNECT(robot_id); remote sends RF CONNECT_REQ; robot replies
 *      CONNECT_OK; remote reports CONNECTED to PC.
 *   3) In session: DATA_TX bytes are tunneled over RF (with ACK/retransmit)
 *      to the robot's UART; RF DATA from the robot is reported to the PC as
 *      DATA_RX. STATS are pushed every 500 ms; GET_STATS forces an immediate
 *      stats frame.
 *===========================================================================*/
#include "config.h"
#include "board/board.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/timer.h"
#include "radio/cc1120.h"
#include "radio/radio_link.h"
#include "protocol/protocol.h"
#include "app/app.h"

int main(void)
{
    board_init();
    uart_init(UART_BAUD_DEFAULT);

    /* DEBUG: prove the UART is alive before anything else can hang the boot.
     * If you see "BOOT" in your terminal -> board_init + uart_init are fine,
     * any silence after this line points to spi/timer/radio init or app loop. */
    global_int_enable();
    uart_write_bytes((const uint8_t *)"BOOT\r\n", 6);

    spi_init();
    uart_write_bytes((const uint8_t *)"SPI ok\r\n", 8);

    timer_init();
    uart_write_bytes((const uint8_t *)"TIMER ok\r\n", 10);

    app_init();
    uart_write_bytes((const uint8_t *)"APP init ok, entering loop\r\n", 28);

    for (;;) {
        app_task();
    }

    return 0;   /* never reached */
}
