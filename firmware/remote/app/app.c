/*============================================================================
 * app/app.c — Remote-node application state machine
 *
 * ==================== BRING-UP FLOW ====================
 * 1. Confirm SPI link: after cc1120_init_minimal(), read back a known
 *    register (e.g. IOCFG0) and verify the reset-default value.
 *    If mismatch -> SPI wiring or clock issue.  Log to PC.
 *
 * 2. Load RF configuration: write all SmartRF-generated registers
 *    inside cc1120_init_minimal().  Verify a few by reading back.
 *
 * 3. Functional test: from PC, send CMD_PING.  The Remote builds an
 *    RF_TYPE_PING packet and transmits it.  If the Robot node is running,
 *    it should reply with RF_TYPE_PING_REPLY (acts as ACK).  The Remote
 *    forwards the result as EVT_ACK_STATUS to PC.
 *
 * 4. Check RSSI/LQI: if CC1120 "append status" is enabled, the received
 *    packet will contain RSSI and LQI.  Forward them in EVT_TELEMETRY
 *    so you can assess link quality on the PC side.
 * ===========================================================
 *===========================================================================*/
#include "app.h"
#include "../config.h"
#include "../board/board.h"
#include "../drivers/uart.h"
#include "../drivers/timer.h"
#include "../radio/cc1120.h"
#include "../radio/cc1120_regs.h"
#include "../radio/radio_link.h"
#include "../protocol/protocol.h"
#include <string.h>

/* ---- Internal state ------------------------------------------------------*/
static app_state_t state;
static protocol_decoder_t uart_dec;

/* Pending TX command queue (one slot — simple for skeleton) */
static bool     tx_pending;
static uint8_t  tx_type;
static uint8_t  tx_payload[MAX_RF_PAYLOAD];
static uint8_t  tx_payload_len;
static uint8_t  tx_seq;
static uint8_t  tx_retries;
static uint32_t tx_deadline;

/* ===== Init ===============================================================*/

void app_init(void)
{
    state      = APP_INIT;
    tx_pending = false;
    tx_retries = 0;
    protocol_decoder_init(&uart_dec);
}

app_state_t app_get_state(void)
{
    return state;
}

/* ---- Forward declarations for internal helpers ---------------------------*/
static void handle_uart_rx(void);
static void handle_rf_rx(void);
static void enqueue_rf_cmd(uint8_t rf_type,
                           const uint8_t *payload, uint8_t len);

/* ===== Main task (non-blocking, call from super-loop) =====================*/

void app_task(void)
{
    switch (state) {

    /* ================================================================
     * BOOT SEQUENCE
     * ================================================================*/
    case APP_INIT:
        protocol_send_log("REMOTE: boot");
        state = APP_UART_READY;
        break;

    case APP_UART_READY:
        protocol_send_log("REMOTE: UART ok");
        state = APP_RF_INIT;
        break;

    case APP_RF_INIT: {
        bool ok = cc1120_init_minimal();
        if (ok) {
            /* Verify SPI by reading a register with known reset value */
            uint8_t iocfg0 = cc1120_reg_read(CC1120_IOCFG0);
            (void)iocfg0;
            /* TODO: check iocfg0 against expected value */
            protocol_send_log("REMOTE: RF init ok");
            radio_link_init();
            cc1120_events_clear_all();
            cc1120_enter_rx();
            state = APP_RX_LISTEN;
        } else {
            protocol_send_log("REMOTE: RF init FAIL");
            Delay_ms(500);
            /* Retry init */
        }
        break;
    }

    /* ================================================================
     * NORMAL OPERATION — IDLE / LISTEN
     * ================================================================*/
    case APP_RX_LISTEN:
        handle_uart_rx();
        cc1120_process_events();

        if (cc1120_events_get_and_clear(RADIO_EVT_RX_DONE)) {
            handle_rf_rx();
        }
        if (cc1120_events_get_and_clear(RADIO_EVT_RX_FIFO_ERR)) {
            state = APP_RX_ERROR_RECOVER;
            break;
        }

        /* If a command is queued for TX, start the TX path */
        if (tx_pending) {
            tx_retries = 0;
            state = APP_DUTY_CHECK;
        }
        break;

    /* ================================================================
     * TX PATH
     * ================================================================*/
    case APP_DUTY_CHECK:
        if (radio_link_tx_allowed()) {
            state = APP_BUILD_RF_PKT;
        }
        /* else: stay here, will be re-checked next app_task() call */
        break;

    case APP_BUILD_RF_PKT:
        tx_seq = radio_link_next_seq();
        state  = APP_TX_LOAD_FIFO;
        break;

    case APP_TX_LOAD_FIFO: {
        /* Build and load the frame into CC1120 FIFO */
        uint8_t pkt_len = 4 + tx_payload_len;
        uint8_t frame[1 + 4 + MAX_RF_PAYLOAD];
        uint8_t idx = 0;

        frame[idx++] = pkt_len;
        frame[idx++] = NET_ID_DEFAULT;
        frame[idx++] = ROBOT_DEV_ID_DEFAULT;
        frame[idx++] = tx_seq;
        frame[idx++] = tx_type;
        memcpy(&frame[idx], tx_payload, tx_payload_len);
        idx += tx_payload_len;

        cc1120_enter_idle();
        if (cc1120_get_txbytes() != 0)
            cc1120_flush_tx();

        cc1120_fifo_write(frame, idx);

        if (cc1120_get_txbytes() != idx) {
            state = APP_TX_ERROR_RECOVER;
            break;
        }
        state = APP_TX_STROBE;
        break;
    }

    case APP_TX_STROBE:
        cc1120_strobe(CC1120_STX);
        radio_link_mark_tx_done();
        tx_deadline = deadline_from_now(50);   /* TX should complete quickly */
        state = APP_TX_WAIT_DONE;
        break;

    case APP_TX_WAIT_DONE:
        cc1120_process_events();

        if (cc1120_events_get_and_clear(RADIO_EVT_TX_DONE)) {
            cc1120_enter_rx();
            tx_deadline = deadline_from_now(ACK_TIMEOUT_MS);
            state = APP_RX_WAIT_ACK;
            break;
        }
        if (cc1120_events_get_and_clear(RADIO_EVT_TX_FIFO_ERR)) {
            state = APP_TX_ERROR_RECOVER;
            break;
        }
        if (deadline_expired(tx_deadline)) {
            state = APP_TX_ERROR_RECOVER;
        }
        break;

    case APP_RX_WAIT_ACK:
        cc1120_process_events();

        if (cc1120_events_get_and_clear(RADIO_EVT_RX_DONE)) {
            rf_packet_t pkt;
            if (radio_link_receive(&pkt)) {
                if (pkt.type == RF_TYPE_ACK && pkt.seq == tx_seq) {
                    tx_pending = false;
                    protocol_send_ack_status(tx_seq, ACK_OK);
                    state = APP_RX_LISTEN;
                    break;
                }
                if (pkt.type == RF_TYPE_TELEMETRY) {
                    protocol_send_telemetry(pkt.payload, pkt.payload_len);
                }
            }
        }
        if (cc1120_events_get_and_clear(RADIO_EVT_RX_FIFO_ERR)) {
            state = APP_TX_ERROR_RECOVER;
            break;
        }
        if (deadline_expired(tx_deadline)) {
            tx_retries++;
            if (tx_retries >= MAX_RETRIES) {
                tx_pending = false;
                protocol_send_ack_status(tx_seq, ACK_TIMEOUT);
                protocol_send_log("REMOTE: TX max retries");
                cc1120_enter_rx();
                state = APP_RX_LISTEN;
            } else {
                state = APP_DUTY_CHECK;
            }
        }
        break;

    /* ================================================================
     * ERROR RECOVERY
     * ================================================================*/
    case APP_RX_ERROR_RECOVER:
        cc1120_enter_idle();
        cc1120_flush_rx();
        cc1120_events_clear_all();
        cc1120_enter_rx();
        protocol_send_log("REMOTE: RX err recover");
        state = APP_RX_LISTEN;
        break;

    case APP_TX_ERROR_RECOVER:
        cc1120_enter_idle();
        cc1120_flush_tx();
        cc1120_events_clear_all();
        cc1120_enter_rx();
        protocol_send_log("REMOTE: TX err recover");

        tx_retries++;
        if (tx_retries >= MAX_RETRIES) {
            tx_pending = false;
            protocol_send_ack_status(tx_seq, ACK_FAIL);
            state = APP_RX_LISTEN;
        } else {
            state = APP_DUTY_CHECK;
        }
        break;

    default:
        state = APP_RX_LISTEN;
        break;
    }
}

/* ===== UART RX processing =================================================*/

static void handle_uart_rx(void)
{
    uint8_t b;
    uart_frame_t frame;

    while (uart_read_byte_nonblocking(&b)) {
        if (protocol_decoder_feed(&uart_dec, b, &frame)) {
            switch (frame.cmd_type) {

            case CMD_MOVE:
                if (frame.payload_len == 2)
                    enqueue_rf_cmd(RF_TYPE_CMD, frame.payload, frame.payload_len);
                break;

            case CMD_STOP:
                enqueue_rf_cmd(RF_TYPE_CMD, (const uint8_t *)"\x00\x00", 2);
                break;

            case CMD_PING:
                enqueue_rf_cmd(RF_TYPE_PING, NULL, 0);
                break;

            case CMD_SET_PARAM:
                /* TODO: implement parameter forwarding */
                enqueue_rf_cmd(RF_TYPE_CMD, frame.payload, frame.payload_len);
                break;

            default:
                protocol_send_log("REMOTE: unknown cmd");
                break;
            }
        }
    }
}

/* ===== RF RX processing ===================================================*/

static void handle_rf_rx(void)
{
    rf_packet_t pkt;
    if (!radio_link_receive(&pkt))
        return;

    switch (pkt.type) {

    case RF_TYPE_TELEMETRY:
        protocol_send_telemetry(pkt.payload, pkt.payload_len);
        break;

    case RF_TYPE_PING_REPLY:
        protocol_send_ack_status(pkt.seq, ACK_OK);
        /* TODO: Forward RSSI/LQI as telemetry for link-quality display */
        break;

    case RF_TYPE_ACK:
        /* ACKs during RX_LISTEN are late/unexpected — ignore or log */
        break;

    default:
        break;
    }
}

/* ===== Enqueue a command for RF transmission ==============================*/

static void enqueue_rf_cmd(uint8_t rf_type,
                           const uint8_t *payload, uint8_t len)
{
    if (tx_pending) {
        protocol_send_log("REMOTE: TX busy, drop");
        return;
    }
    tx_type        = rf_type;
    tx_payload_len = (len <= MAX_RF_PAYLOAD) ? len : MAX_RF_PAYLOAD;
    if (payload && tx_payload_len > 0)
        memcpy(tx_payload, payload, tx_payload_len);
    tx_pending = true;
}
