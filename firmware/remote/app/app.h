/*============================================================================
 * app/app.h — Remote-node application state machine
 *===========================================================================*/
#ifndef APP_H
#define APP_H

#include <stdint.h>

/* Application states */
typedef enum {
    /* Boot sequence */
    APP_INIT,
    APP_UART_READY,
    APP_RF_INIT,

    /* Normal operation */
    APP_RX_LISTEN,

    /* TX path */
    APP_DUTY_CHECK,
    APP_BUILD_RF_PKT,
    APP_TX_LOAD_FIFO,
    APP_TX_STROBE,
    APP_TX_WAIT_DONE,
    APP_RX_WAIT_ACK,

    /* Error recovery */
    APP_RX_ERROR_RECOVER,
    APP_TX_ERROR_RECOVER
} app_state_t;

void        app_init(void);
void        app_task(void);        /* call frequently from main loop */
app_state_t app_get_state(void);

#endif /* APP_H */
