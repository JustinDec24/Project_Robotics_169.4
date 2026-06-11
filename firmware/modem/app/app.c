/*============================================================================
 * app/app.c — Dual-role modem state machines with ARQ on DATA.
 *
 * REMOTE: framed UART <-> RF tunnel to the connected ROBOT.
 * ROBOT : raw UART (login shell) <-> RF tunnel back to REMOTE.
 *
 * DATA frames are reliable (stop-and-wait ACK + retransmit). All other RF
 * frames are best-effort. The RF half-duplex collision risk is mitigated by
 * the random pre-retransmit jitter inside radio_link.
 *===========================================================================*/
#include "app.h"
#include "../config.h"
#include "../board/board.h"
#include "../drivers/timer.h"
#include "../drivers/uart.h"
#include "../radio/cc1120.h"
#include "../radio/cc1120_regs.h"
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
#define DISCONNECT_REASON_ARQ_FAILED      0x05u

typedef struct {
    bool     valid;
    uint8_t  robot_id;
    int8_t   rssi;
    uint32_t last_seen_ms;
} robot_cache_entry_t;

static app_state_t state_;
static app_state_t recover_return_state_;

#if MODEM_ROLE == MODEM_ROLE_REMOTE
static protocol_decoder_t uart_dec_;
static bool     scan_reporting_enabled_;
static uint8_t  connected_robot_id_;
static uint32_t connect_request_ms_;
static uint32_t connect_last_retry_ms_;
static uint8_t  connect_retry_count_;
static uint32_t last_robot_rx_ms_;
static uint32_t last_stats_push_ms_;
static uint32_t last_rtt_ping_ms_;
static uint32_t rtt_ping_sent_ms_;
static uint8_t  rtt_ping_token_;
static bool     rtt_ping_pending_;
static robot_cache_entry_t robot_cache_[MAX_DISCOVERED_ROBOTS];
/* Passthrough mode "sticky reconnect" — when the link is lost while in
 * PASSTHROUGH, we don't tear the session down (the TUI is closed, the user
 * is in PuTTY and has no way to react). Instead we keep firing CONNECT_REQ
 * once a second to the previously-connected robot until it answers
 * CONNECT_OK. The user sees banners written straight to the UART so they
 * know something happened. */
static bool     passthrough_reconnecting_;
static uint32_t passthrough_last_retry_ms_;

/* TUI session "sticky reconnect" — same idea for plain SESSION state. On a
 * marginal link, brief PER spikes can cause LINK_TIMEOUT or ARQ_FAILED
 * even after a successful connect; rather than dropping the user back to
 * IDLE_SCAN (forcing them to manually retype `connect 0x10`), we send the
 * DISCONNECTED frame to the TUI once, keep state_ = SESSION, and retry
 * CONNECT_REQ in the background until the robot answers — or until the
 * total budget (SESSION_RECONNECT_BUDGET_MS) is exhausted. When it
 * succeeds, a fresh CONNECTED frame is sent to the TUI so the Events log
 * narrates "DISCONNECTED -> ... -> CONNECTED" cleanly. */
static bool     session_reconnecting_;
static uint32_t session_reconnect_start_ms_;
static uint32_t session_last_retry_ms_;
#define SESSION_RECONNECT_BUDGET_MS   60000u   /* give up after 60 s */
#define SESSION_RECONNECT_INTERVAL_MS  1000u   /* one CONNECT_REQ / s */
#else
static uint8_t  connected_remote_id_;
static uint32_t last_beacon_ms_;
static uint32_t last_remote_rx_ms_;
#endif

/* ===== Helpers ============================================================*/

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
    state_                = recovery_state;
}

static bool app_send_rf(uint8_t dst_id, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len)
{
    return radio_link_send(dst_id, type, payload, payload_len);
}

/* Attempt to flush a coalesced DATA frame over RF if the radio_link ARQ
 * isn't already busy with a previous one. */
static void app_try_flush_data(uint8_t dst_id, uint32_t now_ms)
{
    uint8_t payload[MAX_RF_PAYLOAD];
    uint8_t len;

    if (radio_link_arq_pending()) {
        return;
    }
    if (!radio_link_data_buf_should_flush(now_ms)) {
        return;
    }
    len = radio_link_data_buf_pop(payload);
    if (len == 0u) {
        return;
    }
    (void)radio_link_arq_send(dst_id, payload, len, now_ms);
}

/* ===== REMOTE role ========================================================*/
#if MODEM_ROLE == MODEM_ROLE_REMOTE

static void remote_enter_idle_scan(void)
{
    connected_robot_id_ = 0;
    radio_link_session_reset();
    rtt_ping_pending_         = false;
    session_reconnecting_     = false;
    passthrough_reconnecting_ = false;
    state_                    = APP_REMOTE_IDLE_SCAN;
}

/* Called from SESSION when LINK_TIMEOUT or ARQ_FAILED fires. We keep
 * state_ = SESSION and start retrying CONNECT_REQ in the background. The
 * TUI is told once via a framed DISCONNECTED so the user knows the link
 * blipped; a fresh CONNECTED is sent if/when the retry succeeds. */
static void session_enter_reconnect(uint32_t now_ms, uint8_t reason)
{
    if (!session_reconnecting_) {
        protocol_send_disconnected(reason);
        session_reconnecting_       = true;
        session_reconnect_start_ms_ = now_ms;
        session_last_retry_ms_      = now_ms - SESSION_RECONNECT_INTERVAL_MS;
    }
    radio_link_session_reset();
    rtt_ping_pending_ = false;
    /* Suppress the watchdog while we're trying. */
    last_robot_rx_ms_ = now_ms;
}

/* Called from PASSTHROUGH when the link is lost (LINK_TIMEOUT or
 * ARQ_FAILED). We keep state_ = PASSTHROUGH and remember the robot_id, so
 * the periodic tick can keep firing CONNECT_REQ at it until it answers. A
 * one-shot banner is written straight to the UART so the user sitting in
 * PuTTY notices what happened. */
static void passthrough_enter_reconnect(uint32_t now_ms)
{
    if (!passthrough_reconnecting_) {
        static const char banner[] =
            "\r\n*** link lost, reconnecting... ***\r\n";
        uart_write_bytes((const uint8_t *)banner, sizeof(banner) - 1u);
        passthrough_reconnecting_  = true;
        passthrough_last_retry_ms_ = now_ms - 1000u;   /* fire immediately */
    }
    radio_link_session_reset();
    rtt_ping_pending_  = false;
    /* Refresh the watchdog so we don't keep re-triggering LINK_TIMEOUT on
     * the same stale last_robot_rx_ms_ value every tick. */
    last_robot_rx_ms_  = now_ms;
}

static void remote_send_stats(void)
{
    link_metrics_t m;
    radio_link_metrics_get(&m);
    /* protocol_send_stats expects unsigned RSSI byte; cast through uint8_t
     * preserves the bit pattern (PC side reinterprets as int8). */
    protocol_send_stats((uint8_t)m.rssi_avg, m.per_pct, m.rtt_ms);
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
    uint8_t  age_100ms;
    robot_cache_entry_t *slot = remote_find_robot_slot(pkt->src_id);

    if (slot->valid) {
        age_ms = now_ms - slot->last_seen_ms;
    }
    age_100ms = (age_ms >= 25500u) ? 255u : (uint8_t)(age_ms / 100u);

    slot->valid        = true;
    slot->robot_id     = pkt->src_id;
    slot->rssi         = pkt->rssi;
    slot->last_seen_ms = now_ms;

    /* SCAN_RESULT is only useful before a session is established — it
     * feeds the TUI's Discovered Robots panel. Once we are connected
     * (SESSION/CONNECTING/PASSTHROUGH) emitting framed bytes on the UART
     * either spams the TUI's event log or, worse, garbles a passthrough
     * shell. Gating on state_ == IDLE_SCAN keeps the cache update path
     * (used for the keep-alive watchdog) while suppressing the framed
     * output. */
    if (scan_reporting_enabled_ && state_ == APP_REMOTE_IDLE_SCAN) {
        protocol_send_scan_result(slot->robot_id, (uint8_t)slot->rssi, age_100ms);
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
        radio_link_session_reset();
        session_reconnecting_ = false;   /* explicit user action, restart fresh */
        if (app_send_rf(connected_robot_id_, RF_TYPE_CONNECT_REQ, NULL, 0u)) {
            connect_request_ms_    = now_ms;
            connect_last_retry_ms_ = now_ms;
            connect_retry_count_   = 0u;
            state_                 = APP_REMOTE_CONNECTING;
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

    case UART_MSG_PASSTHROUGH:
        /* Switch to raw shell-bridge mode. Only allowed during an active
         * session (need a connected robot to bridge to). From here on, the
         * host UART is read as raw bytes (no framing), all RF DATA arriving
         * is written raw to the host UART, and no STATS/LOG frames are
         * emitted (they would corrupt the shell display). If the link
         * drops we stay in PASSTHROUGH and auto-retry CONNECT_REQ every
         * second (the user is in PuTTY with no way to issue commands).
         * Power-cycle / reset is the only way out. */
        if (state_ == APP_REMOTE_SESSION) {
            passthrough_reconnecting_ = false;
            state_                    = APP_REMOTE_PASSTHROUGH;
        }
        break;

    case UART_MSG_DATA_TX:
        if (state_ == APP_REMOTE_SESSION && frame->payload_len > 0u) {
            uint8_t accepted = radio_link_data_buf_push(frame->payload,
                                                        frame->payload_len,
                                                        now_ms);
            if (accepted < frame->payload_len) {
                /* Coalescing buffer was full. The PC client should chunk
                 * its DATA_TX into <= MAX_RF_PAYLOAD bytes per frame. */
                protocol_send_log("DATA_TX truncated (RF buf full)");
            }
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
    static uint32_t last_rx_kick_ms_ = 0u;

    /* Defensive SRX — only re-arms RX if the chip has truly fallen
     * back to IDLE. With RFEND_CFG0.TXOFF_MODE = RX and
     * RFEND_CFG1.RXOFF_MODE = RX, the chip stays in RX through every
     * normal TX/RX cycle on its own, so this should almost never fire.
     * It exists as a safety net for the rare error path that leaves
     * the chip stuck (FIFO recovery, etc.). */
    if ((now_ms - last_rx_kick_ms_) >= 500u) {
        if (cc1120_get_marcstate() == MARCSTATE_IDLE) {
            cc1120_set_rx();
        }
        last_rx_kick_ms_ = now_ms;
    }

    /* Try to flush any coalesced DATA over the ARQ. Same for SESSION (typed
     * by user via DATA_TX frames) and PASSTHROUGH (typed raw on the UART). */
    if (state_ == APP_REMOTE_SESSION || state_ == APP_REMOTE_PASSTHROUGH) {
        app_try_flush_data(connected_robot_id_, now_ms);
    }

    /* Surface DATA ACKs as TX_ACK frames so the host can show a
     * "command delivered" confirmation after each successful send. In
     * PASSTHROUGH we always consume the event (to keep the flag clean)
     * but never emit the framed message — would garble the shell. */
    if (radio_link_arq_take_ack_event() && state_ != APP_REMOTE_PASSTHROUGH) {
        protocol_send_tx_ack();
    }

    /* ARQ tick — handles ACK timeout / retransmit / give-up. */
    if (radio_link_arq_tick(now_ms)) {
        if (state_ == APP_REMOTE_PASSTHROUGH) {
            passthrough_enter_reconnect(now_ms);
            return;
        }
        if (state_ == APP_REMOTE_SESSION) {
            session_enter_reconnect(now_ms, DISCONNECT_REASON_ARQ_FAILED);
            return;
        }
        protocol_send_disconnected(DISCONNECT_REASON_ARQ_FAILED);
        remote_enter_idle_scan();
        return;
    }

    /* CONNECT_REQ retry — on a marginal link a single CONNECT_REQ can be
     * lost. We retry every CONNECT_RETRY_INTERVAL_MS up to CONNECT_MAX_RETRIES
     * times before giving up. The whole budget is bounded by
     * CONNECT_TIMEOUT_MS so the TUI doesn't hang too long if the robot is
     * really unreachable. */
    if (state_ == APP_REMOTE_CONNECTING &&
        connect_retry_count_ < CONNECT_MAX_RETRIES &&
        (now_ms - connect_last_retry_ms_) >= CONNECT_RETRY_INTERVAL_MS) {
        if (app_send_rf(connected_robot_id_, RF_TYPE_CONNECT_REQ, NULL, 0u)) {
            connect_last_retry_ms_ = now_ms;
            connect_retry_count_++;
        }
    }

    if (state_ == APP_REMOTE_CONNECTING &&
        (now_ms - connect_request_ms_) >= CONNECT_TIMEOUT_MS) {
        protocol_send_disconnected(DISCONNECT_REASON_CONNECT_TIMEOUT);
        remote_enter_idle_scan();
        return;
    }

    if ((state_ == APP_REMOTE_SESSION || state_ == APP_REMOTE_PASSTHROUGH) &&
        (now_ms - last_robot_rx_ms_) >= LINK_LOST_TIMEOUT_MS) {
        if (state_ == APP_REMOTE_PASSTHROUGH) {
            passthrough_enter_reconnect(now_ms);
            return;
        }
        if (state_ == APP_REMOTE_SESSION) {
            session_enter_reconnect(now_ms, DISCONNECT_REASON_LINK_TIMEOUT);
            return;
        }
        protocol_send_disconnected(DISCONNECT_REASON_LINK_TIMEOUT);
        remote_enter_idle_scan();
        return;
    }

    /* Sticky-passthrough reconnect: re-fire CONNECT_REQ once per second
     * until the robot answers. Don't update last_robot_rx_ms_ aggressively
     * here — passthrough_enter_reconnect already did that to suppress the
     * watchdog while we're trying. */
    if (state_ == APP_REMOTE_PASSTHROUGH && passthrough_reconnecting_ &&
        (now_ms - passthrough_last_retry_ms_) >= 1000u) {
        (void)app_send_rf(connected_robot_id_, RF_TYPE_CONNECT_REQ, NULL, 0u);
        passthrough_last_retry_ms_ = now_ms;
        last_robot_rx_ms_          = now_ms;
    }

    /* Sticky-session reconnect: same idea but for the TUI mode. The user
     * stays watching the Events log; when the retry succeeds we surface
     * a CONNECTED frame and session traffic resumes transparently. */
    if (state_ == APP_REMOTE_SESSION && session_reconnecting_) {
        if ((now_ms - session_reconnect_start_ms_) >= SESSION_RECONNECT_BUDGET_MS) {
            remote_enter_idle_scan();
            return;
        }
        if ((now_ms - session_last_retry_ms_) >= SESSION_RECONNECT_INTERVAL_MS) {
            (void)app_send_rf(connected_robot_id_, RF_TYPE_CONNECT_REQ, NULL, 0u);
            session_last_retry_ms_ = now_ms;
            last_robot_rx_ms_      = now_ms;
        }
    }

    /* STATS push is for the TUI's Connection panel — pointless and
     * destructive in PASSTHROUGH (would inject framed bytes into the
     * shell output). */
    if (state_ == APP_REMOTE_SESSION &&
        (now_ms - last_stats_push_ms_) >= STATS_PUSH_INTERVAL_MS) {
        remote_send_stats();
        last_stats_push_ms_ = now_ms;
    }

    /* RTT pings — the heartbeat that keeps the link alive in SESSION /
     * PASSTHROUGH. Critically, we do NOT gate on rtt_ping_pending_: if a
     * single PONG is lost, gating would freeze the pending flag forever
     * and the link would die at LINK_LOST_TIMEOUT_MS (6 s). Sending a
     * fresh ping every interval regardless guarantees we keep
     * heartbeating; the token in the PONG payload still tells us which
     * ping it answers, so the RTT measurement remains correct. */
    if ((state_ == APP_REMOTE_SESSION || state_ == APP_REMOTE_PASSTHROUGH) &&
        ((now_ms - last_rtt_ping_ms_) >= RTT_PING_INTERVAL_MS)) {
        payload[0] = RF_STATS_PING_REQ;
        payload[1] = ++rtt_ping_token_;
        if (app_send_rf(connected_robot_id_, RF_TYPE_STATS, payload, 2u)) {
            rtt_ping_pending_  = true;
            rtt_ping_sent_ms_  = now_ms;
            last_rtt_ping_ms_  = now_ms;
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

/* PASSTHROUGH mode UART pump: skip the framing decoder entirely and push
 * raw bytes straight into the DATA coalescing buffer. The coalescing layer
 * (DATA_FLUSH_BYTES / DATA_FLUSH_TIMEOUT_MS) then batches them into RF DATA
 * packets so a single keystroke still gets out fast (~15 ms) while a paste
 * of a longer command flushes by size. We stop reading when the buffer
 * fills up; bytes stay in the UART RX ring (512 B) until the next tick,
 * providing natural backpressure when the air link can't keep up. */
static void remote_consume_uart_passthrough(uint32_t now_ms)
{
    uint8_t b;
    while (radio_link_data_buf_count() < MAX_RF_PAYLOAD) {
        if (!uart_read_byte_nonblocking(&b)) {
            return;
        }
        (void)radio_link_data_buf_push(&b, 1u, now_ms);
    }
}

static void remote_consume_rf(const rf_packet_t *pkt, uint32_t now_ms)
{
    rx_data_action_t action;

    if (pkt->type == RF_TYPE_BEACON) {
        remote_handle_beacon(pkt, now_ms);
        /* Beacons from the connected robot are also a keep-alive: every
         * 500 ms beacon prevents the LINK_LOST watchdog from firing even
         * if our RTT ping or DATA exchange happens to be stalled. */
        if ((state_ == APP_REMOTE_SESSION || state_ == APP_REMOTE_PASSTHROUGH) &&
            pkt->src_id == connected_robot_id_) {
            last_robot_rx_ms_ = now_ms;
        }
        return;
    }

    if (state_ == APP_REMOTE_CONNECTING && pkt->type == RF_TYPE_CONNECT_OK &&
        pkt->src_id == connected_robot_id_) {
        protocol_send_connected(connected_robot_id_);
        last_robot_rx_ms_   = now_ms;
        last_stats_push_ms_ = now_ms;
        last_rtt_ping_ms_   = now_ms;
        rtt_ping_pending_   = false;
        state_              = APP_REMOTE_SESSION;
        return;
    }

    /* Sticky-passthrough reconnect succeeded: write a confirmation banner
     * to the host UART so the user knows the shell is live again, and
     * resume normal passthrough operation. */
    if (state_ == APP_REMOTE_PASSTHROUGH && passthrough_reconnecting_ &&
        pkt->type == RF_TYPE_CONNECT_OK && pkt->src_id == connected_robot_id_) {
        static const char banner[] = "\r\n*** reconnected ***\r\n";
        uart_write_bytes((const uint8_t *)banner, sizeof(banner) - 1u);
        passthrough_reconnecting_ = false;
        last_robot_rx_ms_         = now_ms;
        last_rtt_ping_ms_         = now_ms;
        rtt_ping_pending_         = false;
        return;
    }

    /* Sticky-session reconnect succeeded: send a fresh CONNECTED frame so
     * the TUI's Events log shows "CONNECTED" again and the Connection
     * panel turns green. */
    if (state_ == APP_REMOTE_SESSION && session_reconnecting_ &&
        pkt->type == RF_TYPE_CONNECT_OK && pkt->src_id == connected_robot_id_) {
        protocol_send_connected(connected_robot_id_);
        session_reconnecting_ = false;
        last_robot_rx_ms_     = now_ms;
        last_stats_push_ms_   = now_ms;
        last_rtt_ping_ms_     = now_ms;
        rtt_ping_pending_     = false;
        return;
    }

    if ((state_ != APP_REMOTE_SESSION && state_ != APP_REMOTE_PASSTHROUGH) ||
        pkt->src_id != connected_robot_id_) {
        return;
    }

    last_robot_rx_ms_ = now_ms;

    if (pkt->type == RF_TYPE_DISCONNECT) {
        /* In passthrough we can't send framed messages without garbling
         * the shell display — just tear the session down silently. The
         * user will notice typing no longer echoes and either reset the
         * modem or re-open the TUI to reconnect. */
        if (state_ != APP_REMOTE_PASSTHROUGH) {
            protocol_send_disconnected(DISCONNECT_REASON_RF);
        }
        remote_enter_idle_scan();
        return;
    }

    /* DATA / DATA_ACK -> ARQ layer (also dedup duplicates). */
    if (pkt->type == RF_TYPE_DATA || pkt->type == RF_TYPE_DATA_ACK) {
        action = radio_link_arq_handle_rx(pkt, now_ms);
        if (action == RX_DATA_DELIVER && pkt->payload_len > 0u) {
            /* In PASSTHROUGH the host UART is a raw terminal — write the
             * payload directly without framing. In SESSION it goes via a
             * framed UART_MSG_DATA_RX so the TUI can decode it. */
            if (state_ == APP_REMOTE_PASSTHROUGH) {
                uart_write_bytes(pkt->payload, pkt->payload_len);
            } else {
                protocol_send_data_rx(pkt->payload, pkt->payload_len);
            }
        }
        return;
    }

    if (pkt->type == RF_TYPE_STATS && pkt->payload_len >= 2u &&
        pkt->payload[0] == RF_STATS_PING_RESP && rtt_ping_pending_ &&
        pkt->payload[1] == rtt_ping_token_) {
        radio_link_metrics_set_rtt((uint16_t)(now_ms - rtt_ping_sent_ms_));
        rtt_ping_pending_ = false;
    }
}

#else /* MODEM_ROLE == MODEM_ROLE_ROBOT */

/* ===== ROBOT role =========================================================*/

static void robot_consume_uart(uint32_t now_ms)
{
    uint8_t b;
    /* Drain UART into the coalescing buffer. If the buffer fills up we
     * stop reading; bytes stay in the UART RX ring (sized 512) until the
     * next iteration, providing natural backpressure. */
    while (radio_link_data_buf_count() < MAX_RF_PAYLOAD) {
        if (!uart_read_byte_nonblocking(&b)) {
            return;
        }
        (void)radio_link_data_buf_push(&b, 1u, now_ms);
    }
}

static void robot_consume_rf(const rf_packet_t *pkt, uint32_t now_ms)
{
    uint8_t rsp[2];
    rx_data_action_t action;

    if (state_ == APP_ROBOT_ADVERTISING) {
        if (pkt->type == RF_TYPE_CONNECT_REQ) {
            connected_remote_id_ = pkt->src_id;
            radio_link_session_reset();
            (void)app_send_rf(connected_remote_id_, RF_TYPE_CONNECT_OK, NULL, 0u);
            last_remote_rx_ms_ = now_ms;
            state_             = APP_ROBOT_CONNECTED;
        }
        return;
    }

    if (state_ != APP_ROBOT_CONNECTED || pkt->src_id != connected_remote_id_) {
        return;
    }

    last_remote_rx_ms_ = now_ms;

    switch (pkt->type) {
    case RF_TYPE_DATA:
    case RF_TYPE_DATA_ACK:
        action = radio_link_arq_handle_rx(pkt, now_ms);
        if (action == RX_DATA_DELIVER && pkt->payload_len > 0u) {
            uart_write_bytes(pkt->payload, pkt->payload_len);
        }
        break;

    case RF_TYPE_DISCONNECT:
        connected_remote_id_ = 0u;
        radio_link_session_reset();
        state_ = APP_ROBOT_ADVERTISING;
        break;

    case RF_TYPE_CONNECT_REQ:
        /* Re-ACK in case our previous CONNECT_OK was lost. */
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

#endif /* MODEM_ROLE */

/* ===== Common: drain RX FIFO + classify events ============================*/

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
    /* DO NOT strobe SRX after TX_DONE. RFEND_CFG1 = 0x1F has
     * TXOFF_MODE = RX, so the chip automatically transitions
     * TX -> cal -> settle -> RX with FS_AUTOCAL refreshing the
     * synth lock. Strobing SRX during that transition interrupts
     * the auto-cal and the chip enters RX with a partial cal —
     * subsequent beacons fail to demodulate. At high RSSI this
     * shows up as sporadic 6 s LINK_TIMEOUTs that don't correlate
     * with distance. The passive defensive in remote_periodic
     * still recovers if the chip ever truly falls back to IDLE. */
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

/* ===== Lifecycle ==========================================================*/

void app_init(void)
{
    state_                = APP_INIT;
    recover_return_state_ = APP_INIT;

#if MODEM_ROLE == MODEM_ROLE_REMOTE
    {
        uint8_t i;
        protocol_decoder_init(&uart_dec_);
        scan_reporting_enabled_ = true;
        connected_robot_id_     = 0u;
        connect_request_ms_     = 0u;
        last_robot_rx_ms_       = 0u;
        last_stats_push_ms_     = 0u;
        last_rtt_ping_ms_       = 0u;
        rtt_ping_sent_ms_       = 0u;
        rtt_ping_token_         = 0u;
        rtt_ping_pending_       = false;
        connect_last_retry_ms_     = 0u;
        connect_retry_count_       = 0u;
        session_reconnecting_      = false;
        session_reconnect_start_ms_ = 0u;
        session_last_retry_ms_     = 0u;
        passthrough_reconnecting_  = false;
        passthrough_last_retry_ms_ = 0u;
        for (i = 0u; i < MAX_DISCOVERED_ROBOTS; i++) {
            robot_cache_[i].valid        = false;
            robot_cache_[i].robot_id     = 0u;
            robot_cache_[i].rssi         = 0;
            robot_cache_[i].last_seen_ms = 0u;
        }
    }
#else
    connected_remote_id_ = 0u;
    last_beacon_ms_      = 0u;
    last_remote_rx_ms_   = 0u;
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
        remote_periodic(now_ms);
        break;

    case APP_REMOTE_PASSTHROUGH:
        /* In passthrough we skip the framing decoder — raw UART bytes go
         * straight into the DATA pipeline. Everything else (RF receive,
         * ARQ tick, keep-alive watchdog) still runs as in SESSION. */
        remote_consume_uart_passthrough(now_ms);
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        remote_periodic(now_ms);
        break;
#else
    case APP_ROBOT_ADVERTISING:
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        if ((now_ms - last_beacon_ms_) >= BEACON_INTERVAL_MS) {
            if (app_send_rf(RF_BROADCAST_ID, RF_TYPE_BEACON, NULL, 0u)) {
                led_toggle();
            }
            last_beacon_ms_ = now_ms;
        }
        break;

    case APP_ROBOT_CONNECTED:
        robot_consume_uart(now_ms);
        app_consume_rf(now_ms);
        if (state_ == APP_RX_ERROR_RECOVER || state_ == APP_TX_ERROR_RECOVER) {
            break;
        }
        if (radio_link_arq_tick(now_ms)) {
            /* ARQ gave up. Treat as link lost. */
            connected_remote_id_ = 0u;
            radio_link_session_reset();
            state_ = APP_ROBOT_ADVERTISING;
            break;
        }
        if ((now_ms - last_remote_rx_ms_) >= LINK_LOST_TIMEOUT_MS) {
            connected_remote_id_ = 0u;
            radio_link_session_reset();
            state_ = APP_ROBOT_ADVERTISING;
            break;
        }
        /* Keep beaconing even while connected — gives the REMOTE a strong
         * keep-alive signal (2 Hz) so a single dropped RTT pong doesn't
         * trigger a 6 s LINK_TIMEOUT. The broadcast beacon also helps a
         * third REMOTE rediscover this robot if it shows up later. Light
         * cost: ~10 ms air time every 500 ms = 2 % duty cycle. */
        if ((now_ms - last_beacon_ms_) >= BEACON_INTERVAL_MS) {
            if (app_send_rf(RF_BROADCAST_ID, RF_TYPE_BEACON, NULL, 0u)) {
                led_toggle();
            }
            last_beacon_ms_ = now_ms;
        }
        app_try_flush_data(connected_remote_id_, now_ms);
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
