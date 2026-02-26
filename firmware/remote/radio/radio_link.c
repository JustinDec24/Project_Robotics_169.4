/*============================================================================
 * radio/radio_link.c — RF packet, DATA coalescing and metrics helpers
 *===========================================================================*/
#include "radio_link.h"
#include "cc1120.h"
#include <string.h>

static uint8_t local_id_;
static uint8_t net_id_;
static uint8_t seq_counter_;

/* DATA coalescing buffer. */
static uint8_t  data_buf_[MAX_RF_PAYLOAD];
static uint8_t  data_buf_len_;
static uint32_t data_buf_first_ts_;

/* Metrics placeholders. */
static uint8_t  rssi_avg_;
static uint32_t data_rx_ok_;
static uint32_t data_rx_missing_;
static uint8_t  expected_data_seq_;
static bool     expected_seq_valid_;
static uint16_t rtt_ms_;

/* ===== Init ===============================================================*/

void radio_link_init(uint8_t local_id, uint8_t net_id)
{
    local_id_            = local_id;
    net_id_              = net_id;
    seq_counter_         = 0;
    data_buf_len_        = 0;
    data_buf_first_ts_   = 0;
    rssi_avg_            = 0;
    data_rx_ok_          = 0;
    data_rx_missing_     = 0;
    expected_data_seq_   = 0;
    expected_seq_valid_  = false;
    rtt_ms_              = 0;
}

/* ===== Sequence number ====================================================*/

uint8_t radio_link_next_seq(void)
{
    return seq_counter_++;
}

/* ===== Send packet ========================================================*/
bool radio_link_send(uint8_t dst_id, uint8_t type,
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
    frame[idx++] = radio_link_next_seq();

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

/* ===== Receive packet =====================================================*/

bool radio_link_receive(rf_packet_t *pkt)
{
    uint8_t body_len;
    uint8_t raw[RF_MAX_PKT_SIZE];
    uint8_t i;
    uint8_t seq_gap;

    if (cc1120_get_num_rxbytes() == 0u) {
        return false;
    }

    cc1120_read_rxfifo(raw, 1);
    body_len = raw[0];
    if (body_len < RF_HEADER_SIZE || body_len > RF_MAX_BODY_SIZE) {
        cc1120_flush_rx();
        return false;
    }
    cc1120_read_rxfifo(raw + 1, body_len);

    pkt->net_id      = raw[1];
    pkt->src_id      = raw[2];
    pkt->dst_id      = raw[3];
    pkt->type        = raw[4];
    pkt->seq         = raw[5];
    pkt->payload_len = (uint8_t)(body_len - RF_HEADER_SIZE);
    for (i = 0u; i < pkt->payload_len; i++) {
        pkt->payload[i] = raw[6u + i];
    }

    /* TODO: configure CC1120 append status and parse RSSI/LQI bytes here. */
    pkt->rssi = 0;
    pkt->lqi  = 0;

    if (pkt->net_id != net_id_) {
        return false;
    }
    if ((pkt->dst_id != local_id_) && (pkt->dst_id != RF_BROADCAST_ID)) {
        return false;
    }

    if (rssi_avg_ == 0u) {
        rssi_avg_ = (uint8_t)pkt->rssi;
    } else {
        rssi_avg_ = (uint8_t)(((uint16_t)rssi_avg_ * 3u + (uint8_t)pkt->rssi) / 4u);
    }

    if (pkt->type == RF_TYPE_DATA) {
        if (!expected_seq_valid_) {
            expected_data_seq_ = (uint8_t)(pkt->seq + 1u);
            expected_seq_valid_ = true;
        } else {
            seq_gap = (uint8_t)(pkt->seq - expected_data_seq_);
            if (seq_gap > 0u) {
                data_rx_missing_ += seq_gap;
            }
            expected_data_seq_ = (uint8_t)(pkt->seq + 1u);
        }
        data_rx_ok_++;
    }

    return true;
}

/* ===== DATA coalescing ====================================================*/
void radio_link_data_buf_reset(void)
{
    data_buf_len_ = 0;
    data_buf_first_ts_ = 0;
}

void radio_link_data_buf_push(const uint8_t *data, uint8_t len, uint32_t now_ms)
{
    uint8_t i;
    for (i = 0u; i < len; i++) {
        if (data_buf_len_ == 0u) {
            data_buf_first_ts_ = now_ms;
        }
        if (data_buf_len_ >= MAX_RF_PAYLOAD) {
            break;
        }
        data_buf_[data_buf_len_++] = data[i];
    }
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
    data_buf_len_ = 0;
    data_buf_first_ts_ = 0;
    return len;
}

/* ===== Metrics ============================================================*/
void radio_link_metrics_set_rtt(uint16_t rtt_ms)
{
    rtt_ms_ = rtt_ms;
}

void radio_link_metrics_get(link_metrics_t *out)
{
    uint32_t total = data_rx_ok_ + data_rx_missing_;
    out->rssi_avg = rssi_avg_;
    if (total == 0u) {
        out->per_pct = 0u;
    } else {
        out->per_pct = (uint8_t)((data_rx_missing_ * 100u) / total);
    }
    out->rtt_ms = rtt_ms_;
}
