/*============================================================================
 * app/app.h — Dual-role modem application state machine
 *===========================================================================*/
#ifndef APP_H
#define APP_H

#include <stdint.h>

/* Application states */
typedef enum {
    APP_INIT,
    APP_RF_READY,

    /* Remote role */
    APP_REMOTE_IDLE_SCAN,
    APP_REMOTE_CONNECTING,
    APP_REMOTE_SESSION,

    /* Robot role */
    APP_ROBOT_ADVERTISING,
    APP_ROBOT_CONNECTED,

    /* Shared recovery */
    APP_RX_ERROR_RECOVER,
    APP_TX_ERROR_RECOVER
} app_state_t;

void        app_init(void);
void        app_task(void);        /* call frequently from main loop */
app_state_t app_get_state(void);

#endif /* APP_H */
