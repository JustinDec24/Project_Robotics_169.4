/*============================================================================
 * radio/radio_link.c — RF packet framing, ARQ, coalescing, metrics.
 *===========================================================================*/
#include "radio_link.h"
#include "cc1120.h"
#include <string.h>

/* ---- Identity ------------------------------------------------------------*/
static uint8_t local_id_;
static uint8_t net_id_;

/* ---- Sequence counters (per type, so PER metric isn't polluted) ----------*/
static uint8_t seq_other_;          /* BEACON / CONNECT_* / STATS / DISC    */
static uint8_t seq_data_tx_;        /* outgoing DATA                         */

/* ---- Outbound DATA coalescing buffer (UART -> RF) -----------------------*/
static uint8_t  data_buf_[MAX_RF_PAYLOAD];
static uint8_t  data_buf_len_;
static uint32_t data_buf_first_ts_;

/* ---- ARQ state (sender side, stop-and-wait) -----------------------------*/
typedef struct {
    bool     pending;
    uint8_t  dst_id;
    uint8_t  seq;
    uint8_t  retries;
    uint8_t  payload[MAX_RF_PAYLOAD];
    uint8_t  payload_len;
    uint32_t sent_ms;
    uint32_t backoff_ms;     /* random pre-retransmit jitter             */
} arq_tx_t;

static arq_tx_t arq_tx_;

/* ---- ARQ state (receiver side, last DATA seq for dedup) ------------------*/
static uint8_t  last_rx_data_seq_;
static bool     last_rx_data_seq_valid_;

/* ---- Metrics -------------------------------------------------------------*/
static int8_t   rssi_avg_;          /* signed dBm-like, smoothed             */
static bool     rssi_avg_valid_;
static uint32_t data_tx_ok_;        /* DATA frames acked                     */
static uint32_t data_tx_failed_;    /* DATA frames given up after retries    */
static uint16_t rtt_ms_;

/* ---- One-shot event flag, set when the most recent ARQ DATA was ACKed.
 * The app layer polls this once per remote_periodic() tick and surfaces
 * it to the host as a UART_MSG_TX_ACK frame. */
static volatile bool arq_tx_ack_event_ = false;

/* ---- Tiny LCG for backoff jitter ----------------------------------------*/
static uint16_t rand_state_;
static uint8_t rand_byte_(void)
{
    rand_state_ = (uint16_t)(rand_state_ * 1103u + 12345u);
    return (uint8_t)(rand_state_ >> 8);
}

/* ===== Init ===============================================================*/

void radio_link_init(uint8_t local_id, uint8_t net_id)
{
    local_id_              = local_id;
    net_id_                = net_id;
    seq_other_             = 0;
    seq_data_tx_           = 0;
    data_buf_len_          = 0;
    data_buf_first_ts_     = 0;
    arq_tx_.pending        = false;
    arq_tx_.retries        = 0;
    last_rx_data_seq_      = 0;
    last_rx_data_seq_valid_ = false;
    rssi_avg_              = 0;
    rssi_avg_valid_        = false;
    data_tx_ok_            = 0;
    data_tx_failed_        = 0;
    rtt_ms_                = 0;
    rand_state_            = 0xACE1u;
}

void radio_link_session_reset(void)
{
    data_buf_len_           = 0;
    data_buf_first_ts_      = 0;
    arq_tx_.pending         = false;
    arq_tx_.retries         = 0;
    last_rx_data_seq_       = 0;
    last_rx_data_seq_valid_ = false;
    rssi_avg_valid_         = false;
    data_tx_ok_             = 0;
    data_tx_failed_         = 0;
    rtt_ms_                 = 0;
}

/* ===== Low-level packet send (no ARQ) =====================================*/

static bool tx_raw(uint8_t dst_id, uint8_t type, uint8_t seq,
                   const uint8_t *payload, uint8_t payload_len)
{
    uint8_t frame[RF_MAX_PKT_SIZE];
    uint8_t idx = 0u;
    uint8_t i;

    if (payload_len > MAX_RF_PAYLOAD) {
        return false;
    }

    frame[idx++] = (uint8_t)(RF_HEADER_SIZE + payload_len);
    frame[idx++] = net_id_;
    frame[idx++] = local_id_;
    frame[idx++] = dst_id;
    frame[idx++] = type;
    frame[idx++] = seq;
    for (i = 0u; i < payload_len; i++) {
        frame[idx++] = payload[i];
    }

    cc1120_set_idle();
    if (cc1120_get_num_txbytes() != 0u) {
        cc1120_flush_tx();
    }
    cc1120_write_txfifo(frame, idx);

    if (cc1120_get_num_txbytes() != idx) {
        cc1120_flush_tx();
        return false;
    }

    cc1120_strobe_stx();
    return true;
}

bool radio_link_send(uint8_t dst_id, uint8_t type,
                     const uint8_t *payload, uint8_t payload_len)
{
    /* Non-DATA / non-ACK traffic uses the shared seq_other_ counter.
     * DATA goes through radio_link_arq_send(); DATA_ACK is sent directly
     * from radio_link_arq_handle_rx() to keep the ack-seq tied to the
     * received DATA packet. */
    uint8_t seq = seq_other_++;
    return tx_raw(dst_id, type, seq, payload, payload_len);
}

/* ===== Receive ============================================================*/

bool radio_link_receive(rf_packet_t *pkt)
{
    uint8_t body_len;
    uint8_t need;
    uint8_t raw[RF_MAX_PKT_SIZE + RF_APPEND_STATUS_BYTES];
    uint8_t i;
    uint8_t status_idx;

    /* Need at least 1 (LEN) byte to start. */
    if (cc1120_get_num_rxbytes() == 0u) {
        return false;
    }

    cc1120_read_rxfifo(raw, 1);
    body_len = raw[0];
    if (body_len < RF_HEADER_SIZE || body_len > RF_MAX_BODY_SIZE) {
        cc1120_flush_rx();
        return false;
    }

    need = (uint8_t)(body_len + RF_APPEND_STATUS_BYTES);
    if (cc1120_get_num_rxbytes() < need) {
        cc1120_flush_rx();
        return false;
    }

    cc1120_read_rxfifo(raw + 1, need);

    pkt->net_id      = raw[1];
    pkt->src_id      = raw[2];
    pkt->dst_id      = raw[3];
    pkt->type        = raw[4];
    pkt->seq         = raw[5];
    pkt->payload_len = (uint8_t)(body_len - RF_HEADER_SIZE);
    for (i = 0u; i < pkt->payload_len; i++) {
        pkt->payload[i] = raw[6u + i];
    }

    status_idx = (uint8_t)(1u + body_len);
    pkt->rssi   = (int8_t)raw[status_idx];
    pkt->lqi    = (uint8_t)(raw[status_idx + 1u] & 0x7Fu);
    pkt->crc_ok = (raw[status_idx + 1u] & 0x80u) != 0u;

    if (!pkt->crc_ok) {
        return false;
    }
    if (pkt->net_id != net_id_) {
        return false;
    }
    if ((pkt->dst_id != local_id_) && (pkt->dst_id != RF_BROADCAST_ID)) {
        return false;
    }

    radio_link_metrics_note_rssi(pkt->rssi);
    return true;
}

/* ===== ARQ ================================================================*/

static bool arq_send_now(uint32_t now_ms)
{
    bool ok = tx_raw(arq_tx_.dst_id, RF_TYPE_DATA, arq_tx_.seq,
                     arq_tx_.payload, arq_tx_.payload_len);
    arq_tx_.sent_ms = now_ms;
    /* On retry, add a small random jitter so two nodes don't keep colliding. */
    arq_tx_.backoff_ms = (uint32_t)(rand_byte_() % (ARQ_BACKOFF_MAX_MS + 1u));
    return ok;
}

bool radio_link_arq_send(uint8_t dst_id, const uint8_t *payload,
                         uint8_t payload_len, uint32_t now_ms)
{
    if (arq_tx_.pending) {
        return false;
    }
    if (payload_len == 0u || payload_len > MAX_RF_PAYLOAD) {
        return false;
    }

    arq_tx_.dst_id      = dst_id;
    arq_tx_.seq         = seq_data_tx_++;
    arq_tx_.payload_len = payload_len;
    memcpy(arq_tx_.payload, payload, payload_len);
    arq_tx_.retries     = 0;
    arq_tx_.pending     = true;
    arq_tx_.backoff_ms  = 0;

    (void)arq_send_now(now_ms);
    return true;
}

bool radio_link_arq_pending(void)
{
    return arq_tx_.pending;
}

bool radio_link_arq_take_ack_event(void)
{
    if (arq_tx_ack_event_) {
        arq_tx_ack_event_ = false;
        return true;
    }
    return false;
}

rx_data_action_t radio_link_arq_handle_rx(const rf_packet_t *pkt, uint32_t now_ms)
{
    (void)now_ms;

    if (pkt->type == RF_TYPE_DATA_ACK) {
        if (arq_tx_.pending && pkt->seq == arq_tx_.seq &&
            pkt->src_id == arq_tx_.dst_id) {
            arq_tx_.pending     = false;
            arq_tx_ack_event_   = true;
            data_tx_ok_++;
        }
        return RX_DATA_NOT_DATA;
    }

    if (pkt->type != RF_TYPE_DATA) {
        return RX_DATA_NOT_DATA;
    }

    /* Always ACK a valid DATA, whether duplicate or fresh. The acked seq
     * is carried in the RF header — payload is unused. */
    (void)tx_raw(pkt->src_id, RF_TYPE_DATA_ACK, pkt->seq, NULL, 0u);

    if (last_rx_data_seq_valid_ && pkt->seq == last_rx_data_seq_) {
        return RX_DATA_DUPLICATE;
    }
    last_rx_data_seq_       = pkt->seq;
    last_rx_data_seq_valid_ = true;
    return RX_DATA_DELIVER;
}

bool radio_link_arq_tick(uint32_t now_ms)
{
    uint32_t deadline;

    if (!arq_tx_.pending) {
        return false;
    }

    deadline = arq_tx_.sent_ms + ARQ_ACK_TIMEOUT_MS + arq_tx_.backoff_ms;
    if ((int32_t)(now_ms - deadline) < 0) {
        return false;       /* still waiting */
    }

    if (arq_tx_.retries >= ARQ_MAX_RETRIES) {
        arq_tx_.pending = false;
        data_tx_failed_++;
        return true;        /* gave up — link is bad */
    }

    arq_tx_.retries++;
    (void)arq_send_now(now_ms);
    return false;
}

/* ===== DATA coalescing ====================================================*/

void radio_link_data_buf_reset(void)
{
    data_buf_len_      = 0;
    data_buf_first_ts_ = 0;
}

uint8_t radio_link_data_buf_push(const uint8_t *data, uint8_t len, uint32_t now_ms)
{
    uint8_t accepted = 0u;
    uint8_t i;
    for (i = 0u; i < len; i++) {
        if (data_buf_len_ >= MAX_RF_PAYLOAD) {
            break;
        }
        if (data_buf_len_ == 0u) {
            data_buf_first_ts_ = now_ms;
        }
        data_buf_[data_buf_len_++] = data[i];
        accepted++;
    }
    return accepted;
}

bool radio_link_data_buf_should_flush(uint32_t now_ms)
{
    if (data_buf_len_ == 0u) {
        return false;
    }
    if (data_buf_len_ >= DATA_FLUSH_BYTES) {
        return true;
    }
    return ((uint32_t)(now_ms - data_buf_first_ts_) >= DATA_FLUSH_TIMEOUT_MS);
}

uint8_t radio_link_data_buf_pop(uint8_t *out_payload)
{
    uint8_t len = data_buf_len_;
    if (len > 0u) {
        memcpy(out_payload, data_buf_, len);
    }
    data_buf_len_      = 0;
    data_buf_first_ts_ = 0;
    return len;
}

uint8_t radio_link_data_buf_count(void)
{
    return data_buf_len_;
}

/* ===== Metrics ============================================================*/

void radio_link_metrics_note_rssi(int8_t rssi)
{
    if (!rssi_avg_valid_) {
        rssi_avg_       = rssi;
        rssi_avg_valid_ = true;
    } else {
        /* Exponential smoothing: avg = (3*avg + new) / 4. */
        int16_t a = (int16_t)rssi_avg_ * 3 + (int16_t)rssi;
        rssi_avg_ = (int8_t)(a / 4);
    }
}

void radio_link_metrics_set_rtt(uint16_t rtt_ms)
{
    rtt_ms_ = rtt_ms;
}

void radio_link_metrics_get(link_metrics_t *out)
{
    uint32_t total = data_tx_ok_ + data_tx_failed_;
    out->rssi_avg = rssi_avg_;
    if (total == 0u) {
        out->per_pct = 0u;
    } else {
        out->per_pct = (uint8_t)((data_tx_failed_ * 100u) / total);
    }
    out->rtt_ms = rtt_ms_;
}
