/*============================================================================
 * main.c — Remote node entry point
 *
 * Project : 169.4 MHz RF link  (CC1120 + PIC)
 * Node    : REMOTE  (PC-connected controller)
 *
 * Flow:
 *   PC <-> UART <-> PIC <-> SPI <-> CC1120 <-> RF <-> Robot
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
