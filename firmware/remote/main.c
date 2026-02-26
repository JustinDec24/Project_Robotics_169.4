/*============================================================================
 * main.c — Dual-role modem entry point
 *
 * How to run:
 * - Build with MODEM_ROLE=MODEM_ROLE_REMOTE for the PC-connected modem.
 *   UART wiring: USB-UART dongle <-> modem UART (framed protocol in protocol/).
 * - Build with MODEM_ROLE=MODEM_ROLE_ROBOT for the SBC-connected modem.
 *   UART wiring: modem UART <-> SBC tty running getty/login shell.
 * - Session flow:
 *   1) PC sends SCAN_START to remote modem and receives SCAN_RESULT.
 *   2) PC sends CONNECT(robot_id), remote sends RF CONNECT_REQ, robot replies
 *      CONNECT_OK, remote reports CONNECTED to PC.
 *   3) In session: DATA_TX bytes are tunneled over RF DATA to robot UART; RF
 *      DATA from robot UART is reported to PC as DATA_RX; STATS are pushed and
 *      available via GET_STATS.
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

/* TODO: Choose UART baud rate suitable for your PIC clock and PC interface. */
#define UART_BAUD   9600u

int main(void)
{
    board_init();
    uart_init(UART_BAUD);
    spi_init();
    timer_init();
    global_int_enable();

    app_init();

    /* --- Super-loop -------------------------------------------------------*/
    for (;;) {
        app_task();
    }

    return 0;   /* never reached */
}
