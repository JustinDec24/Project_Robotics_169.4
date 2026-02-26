/*============================================================================
 * app/app.c — Dual-role modem state machines
 *===========================================================================*/
#include "app.h"
#include "../config.h"
#include "../drivers/timer.h"
#include "../drivers/uart.h"
#include "../radio/cc1120.h"
#include "../radio/radio_link.h"

#if MODEM_ROLE == MODEM_ROLE_REMOTE
#include "../protocol/protocol.h"
#endif

#include <string.h>

#define APP_EVENT_MASK_ALL (RADIO_EVT_RX_DONE | RADIO_EVT_TX_DONE | \
                            RADIO_EVT_RX_OVERFLOW | RADIO_EVT_TX_UNDERFLOW | \
                            RADIO_EVT_GDO_EDGE)

#define DISCONNECT_REASON_PC              0x01u
#define DISCONNECT_REASON_RF              0x02u
#define DISCONNECT_REASON_LINK_TIMEOUT    0x03u
#define DISCONNECT_REASON_CONNECT_TIMEOUT 0x04u

typedef struct {
    bool     valid;
    uint8_t  robot_id;
    uint8_t  rssi;
    uint32_t last_seen_ms;
} robot_cache_entry_t;

static app_state_t state_;
static app_state_t recover_return_state_;

#if MODEM_ROLE == MODEM_ROLE_REMOTE
static protocol_decoder_t uart_dec_;
static bool scan_reporting_enabled_;
static uint8_t connected_robot_id_;
static uint32_t connect_request_ms_;
static uint32_t last_robot_rx_ms_;
static uint32_t last_stats_push_ms_;
static uint32_t last_rtt_ping_ms_;
static uint32_t rtt_ping_sent_ms_;
static uint8_t rtt_ping_token_;
static bool rtt_ping_pending_;
static robot_cache_entry_t robot_cache_[MAX_DISCOVERED_ROBOTS];
#else
static uint8_t connected_remote_id_;
static uint32_t last_beacon_ms_;
static uint32_t last_remote_rx_ms_;
#endif

static uint8_t app_local_id(void)
{
#if MODEM_ROLE == MODEM_ROLE_REMOTE
    return REMOTE_MODEM_ID;
#else
    return ROBOT_MODEM_ID_DEFAULT;
#endif
}

static void set_recovery_state(app_state_t recovery_state)
{
    recover_return_state_ = state_;
    state_ = recovery_state;
}

static bool app_send_rf(uint8_t dst_id, uint8_t type, const uint8_t *payload, uint8_t payload_len)
{
    if (radio_link_send(dst_id, type, payload, payload_len)) {
        return true;
    }
    return false;
}

#if MODEM_ROLE == MODEM_ROLE_REMOTE
static void remote_enter_idle_scan(void)
{
    connected_robot_id_ = 0;
    radio_link_data_buf_reset();
    rtt_ping_pending_ = false;
    state_ = APP_REMOTE_IDLE_SCAN;
}

static void remote_send_stats(void)
{
    link_metrics_t m;
    radio_link_metrics_get(&m);
    protocol_send_stats(m.rssi_avg, m.per_pct, m.rtt_ms);
}

static robot_cache_entry_t *remote_find_robot_slot(uint8_t robot_id)
{
    uint8_t i;
    robot_cache_entry_t *free_slot = NULL;
    for (i = 0u; i < MAX_DISCOVERED_ROBOTS; i++) {
        if (robot_cache_[i].valid && robot_cache_[i].robot_id == robot_id) {
            return &robot_cache_[i];
        }
        if (!robot_cache_[i].valid && free_slot == NULL) {
            free_slot = &robot_cache_[i];
        }
    }
    return (free_slot != NULL) ? free_slot : &robot_cache_[0];
}

static void remote_handle_beacon(const rf_packet_t *pkt, uint32_t now_ms)
{
    uint32_t age_ms = 0;
    uint8_t age_100ms;
    robot_cache_entry_t *slot = remote_find_robot_slot(pkt->src_id);

    if (slot->valid) {
        age_ms = now_ms - slot->last_seen_ms;
    }
    age_100ms = (age_ms >= 25500u) ? 255u : (uint8_t)(age_ms / 100u);

    slot->valid = true;
    slot->robot_id = pkt->src_id;
    slot->rssi = (uint8_t)pkt->rssi;
    slot->last_seen_ms = now_ms;

    if (scan_reporting_enabled_) {
        protocol_send_scan_result(slot->robot_id, slot->rssi, age_100ms);
    }
}

static void remote_handle_pc_frame(const uart_frame_t *frame, uint32_t now_ms)
{
    switch (frame->msg_type) {
    case UART_MSG_SCAN_START:
        scan_reporting_enabled_ = true;
        break;

    case UART_MSG_SCAN_STOP:
        scan_reporting_enabled_ = false;
        break;

    case UART_MSG_CONNECT:
        if (frame->payload_len != 1u) {
            protocol_send_log("CONNECT expects robot_id");
            break;
        }
        connected_robot_id_ = frame->payload[0];
        if (app_send_rf(connected_robot_id_, RF_TYPE_CONNECT_REQ, NULL, 0u)) {
            connect_request_ms_ = now_ms;
            state_ = APP_REMOTE_CONNECTING;
        } else {
            protocol_send_log("CONNECT_REQ TX failed");
        }
        break;

    case UART_MSG_DISCONNECT:
        if (state_ == APP_REMOTE_CONNECTING || state_ == APP_REMOTE_SESSION) {
            (void)app_send_rf(connected_robot_id_, RF_TYPE_DISCONNECT, NULL, 0u);
            protocol_send_disconnected(DISCONNECT_REASON_PC);
            remote_enter_idle_scan();
        }
        break;

    case UART_MSG_GET_STATS:
        remote_send_stats();
        break;

    case UART_MSG_DATA_TX:
        if (state_ == APP_REMOTE_SESSION && frame->payload_len > 0u) {
            radio_link_data_buf_push(frame->payload, frame->payload_len, now_ms);
        }
        break;

    default:
        protocol_send_log("Unknown UART msg");
        break;
    }

}

static void remote_periodic(uint32_t now_ms)
{
    uint8_t payload[2];

    if (state_ == APP_REMOTE_SESSION && radio_link_data_buf_should_flush(now_ms)) {
        uint8_t data_payload[MAX_RF_PAYLOAD];
        uint8_t data_len = radio_link_data_buf_pop(data_payload);
        if (data_len > 0u) {
            (void)app_send_rf(connected_robot_id_, RF_TYPE_DATA, data_payload, data_len);
        }
    }

    if (state_ == APP_REMOTE_CONNECTING &&
        (now_ms - connect_request_ms_) >= CONNECT_TIMEOUT_MS) {
        protocol_send_disconnected(DISCONNECT_REASON_CONNECT_TIMEOUT);
        remote_enter_idle_scan();
    }

    if (state_ == APP_REMOTE_SESSION &&
        (now_ms - last_robot_rx_ms_) >= LINK_LOST_TIMEOUT_MS) {
        protocol_send_disconnected(DISCONNECT_REASON_LINK_TIMEOUT);
        remote_enter_idle_scan();
    }

    if (state_ == APP_REMOTE_SESSION &&
        (now_ms - last_stats_push_ms_) >= STATS_PUSH_INTERVAL_MS) {
        remote_send_stats();
        last_stats_push_ms_ = now_ms;
    }

    if (state_ == APP_REMOTE_SESSION && !rtt_ping_pending_ &&
        ((now_ms - last_rtt_ping_ms_) >= RTT_PING_INTERVAL_MS)) {
        payload[0] = RF_STATS_PING_REQ;
        payload[1] = ++rtt_ping_token_;
        if (app_send_rf(connected_robot_id_, RF_TYPE_STATS, payload, 2u)) {
            rtt_ping_pending_ = true;
            rtt_ping_sent_ms_ = now_ms;
            last_rtt_ping_ms_ = now_ms;
        }
    }
}

static void remote_consume_uart(uint32_t now_ms)
{
    uint8_t b;
    uart_frame_t frame;
    while (uart_read_byte_nonblocking(&b)) {
        if (protocol_decoder_feed(&uart_dec_, b, &frame)) {
            remote_handle_pc_frame(&frame, now_ms);
        }
    }
}

static void remote_consume_rf(const rf_packet_t *pkt, uint32_t now_ms)
{
    if (pkt->type == RF_TYPE_BEACON) {
        remote_handle_beacon(pkt, now_ms);
        return;
    }

    if (state_ == APP_REMOTE_CONNECTING && pkt->type == RF_TYPE_CONNECT_OK &&
        pkt->src_id == connected_robot_id_) {
        protocol_send_connected(connected_robot_id_);
        last_robot_rx_ms_ = now_ms;
        last_stats_push_ms_ = now_ms;
        last_rtt_ping_ms_ = now_ms;
        rtt_ping_pending_ = false;
        state_ = APP_REMOTE_SESSION;
        return;
    }

    if (state_ != APP_REMOTE_SESSION || pkt->src_id != connected_robot_id_) {
        return;
    }

    last_robot_rx_ms_ = now_ms;

    if (pkt->type == RF_TYPE_DISCONNECT) {
        protocol_send_disconnected(DISCONNECT_REASON_RF);
        remote_enter_idle_scan();
        return;
    }

    if (pkt->type == RF_TYPE_DATA && pkt->payload_len > 0u) {
        protocol_send_data_rx(pkt->payload, pkt->payload_len);
        return;
    }

    if (pkt->type == RF_TYPE_STATS && pkt->payload_len >= 2u &&
        pkt->payload[0] == RF_STATS_PING_RESP && rtt_ping_pending_ &&
        pkt->payload[1] == rtt_ping_token_) {
        radio_link_metrics_set_rtt((uint16_t)(now_ms - rtt_ping_sent_ms_));
        rtt_ping_pending_ = false;
    }
}
#else
static void robot_consume_uart(uint32_t now_ms)
{
    uint8_t b;
    while (uart_read_byte_nonblocking(&b)) {
        radio_link_data_buf_push(&b, 1u, now_ms);
    }
}

static void robot_consume_rf(const rf_packet_t *pkt, uint32_t now_ms)
{
    uint8_t rsp[2];

    if (state_ == APP_ROBOT_ADVERTISING) {
        if (pkt->type == RF_TYPE_CONNECT_REQ) {
            connected_remote_id_ = pkt->src_id;
            (void)app_send_rf(connected_remote_id_, RF_TYPE_CONNECT_OK, NULL, 0u);
            last_remote_rx_ms_ = now_ms;
            radio_link_data_buf_reset();
            state_ = APP_ROBOT_CONNECTED;
        }
        return;
    }

    if (state_ != APP_ROBOT_CONNECTED || pkt->src_id != connected_remote_id_) {
        return;
    }

    last_remote_rx_ms_ = now_ms;

    switch (pkt->type) {
    case RF_TYPE_DATA:
        if (pkt->payload_len > 0u) {
            uart_write_bytes(pkt->payload, pkt->payload_len);
        }
        break;

    case RF_TYPE_DISCONNECT:
        connected_remote_id_ = 0u;
        radio_link_data_buf_reset();
        state_ = APP_ROBOT_ADVERTISING;
        break;

    case RF_TYPE_CONNECT_REQ:
        (void)app_send_rf(connected_remote_id_, RF_TYPE_CONNECT_OK, NULL, 0u);
        break;

    case RF_TYPE_STATS:
        if (pkt->payload_len >= 2u && pkt->payload[0] == RF_STATS_PING_REQ) {
            rsp[0] = RF_STATS_PING_RESP;
            rsp[1] = pkt->payload[1];
            (void)app_send_rf(connected_remote_id_, RF_TYPE_STATS, rsp, 2u);
        }
        break;

    default:
        break;
    }
}
#endif

static void app_consume_rf(uint32_t now_ms)
{
    rf_packet_t pkt;
    uint8_t evt;
    uint8_t i;

    cc1120_process_events();
    evt = cc1120_events_get_and_clear(APP_EVENT_MASK_ALL);

    if ((evt & RADIO_EVT_RX_OVERFLOW) != 0u) {
        set_recovery_state(APP_RX_ERROR_RECOVER);
        return;
    }
    if ((evt & RADIO_EVT_TX_UNDERFLOW) != 0u) {
        set_recovery_state(APP_TX_ERROR_RECOVER);
        return;
    }
    if ((evt & RADIO_EVT_TX_DONE) != 0u) {
        cc1120_set_rx();
    }
    if ((evt & RADIO_EVT_RX_DONE) == 0u) {
        return;
    }

    for (i = 0u; i < 4u; i++) {
        if (!radio_link_receive(&pkt)) {
            break;
        }
#if MODEM_ROLE == MODEM_ROLE_REMOTE
        remote_consume_rf(&pkt, now_ms);
#else
        robot_consume_rf(&pkt, now_ms);
#endif
    }
}

void app_init(void)
{
    uint8_t i;
    state_ = APP_INIT;
    recover_return_state_ = APP_INIT;

#if MODEM_ROLE == MODEM_ROLE_REMOTE
    protocol_decoder_init(&uart_dec_);
    scan_reporting_enabled_ = true;
    connected_robot_id_ = 0u;
    connect_request_ms_ = 0u;
    last_robot_rx_ms_ = 0u;
    last_stats_push_ms_ = 0u;
    last_rtt_ping_ms_ = 0u;
    rtt_ping_sent_ms_ = 0u;
    rtt_ping_token_ = 0u;
    rtt_ping_pending_ = false;
    for (i = 0u; i < MAX_DISCOVERED_ROBOTS; i++) {
        robot_cache_[i].valid = false;
        robot_cache_[i].robot_id = 0u;
        robot_cache_[i].rssi = 0u;
        robot_cache_[i].last_seen_ms = 0u;
    }
#else
    (void)i;
    connected_remote_id_ = 0u;
    last_beacon_ms_ = 0u;
    last_remote_rx_ms_ = 0u;
#endif
}

app_state_t app_get_state(void)
{
    return state_;
}

void app_task(void)
{
    uint32_t now_ms = millis();

    switch (state_) {
    case APP_INIT:
        if (!cc1120_init_minimal()) {
#if MODEM_ROLE == MODEM_ROLE_REMOTE
            protocol_send_log("CC1120 init fail");
#endif
            return;
        }
        radio_link_init(app_local_id(), NET_ID_DEFAULT);
        cc1120_events_clear_all();
        cc1120_set_rx();
#if MODEM_ROLE == MODEM_ROLE_REMOTE
        protocol_send_log("REMOTE modem ready");
#else
        /* ROBOT role does not use framed UART protocol during session. */
#endif
        state_ = APP_RF_READY;
        break;

    case APP_RF_READY:
#if MODEM_ROLE == MODEM_ROLE_REMOTE
        state_ = APP_REMOTE_IDLE_SCAN;
#else
        state_ = APP_ROBOT_ADVERTISING;
#endif
        break;

#if MODEM_ROLE == MODEM_ROLE_REMOTE
    case APP_REMOTE_IDLE_SCAN:
    case APP_REMOTE_CONNECTING:
    case APP_REMOTE_SESSION:
        remote_consume_uart(now_ms);
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        /* Periodic tunnel flush, stats push and RTT probing. */
        remote_periodic(now_ms);
        break;
#else
    case APP_ROBOT_ADVERTISING:
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        if ((now_ms - last_beacon_ms_) >= BEACON_INTERVAL_MS) {
            (void)app_send_rf(RF_BROADCAST_ID, RF_TYPE_BEACON, NULL, 0u);
            last_beacon_ms_ = now_ms;
        }
        break;

    case APP_ROBOT_CONNECTED:
        robot_consume_uart(now_ms);
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        if ((now_ms - last_remote_rx_ms_) >= LINK_LOST_TIMEOUT_MS) {
            connected_remote_id_ = 0u;
            radio_link_data_buf_reset();
            state_ = APP_ROBOT_ADVERTISING;
            break;
        }
        if (radio_link_data_buf_should_flush(now_ms)) {
            uint8_t data_payload[MAX_RF_PAYLOAD];
            uint8_t data_len = radio_link_data_buf_pop(data_payload);
            if (data_len > 0u) {
                (void)app_send_rf(connected_remote_id_, RF_TYPE_DATA, data_payload, data_len);
            }
        }
        break;
#endif

    case APP_RX_ERROR_RECOVER:
        cc1120_strobe_sidle();
        cc1120_strobe_sfrx();
        cc1120_set_rx();
        state_ = recover_return_state_;
        break;

    case APP_TX_ERROR_RECOVER:
        cc1120_strobe_sidle();
        cc1120_strobe_sftx();
        cc1120_set_rx();
        state_ = recover_return_state_;
        break;

    default:
        state_ = APP_RF_READY;
        break;
    }
}
