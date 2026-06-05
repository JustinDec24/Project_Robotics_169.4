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

/* ===== ARQ ================================================================
 * Stop-and-wait reliability layer for RF_TYPE_DATA packets.
 *
 *   Sender side                          Receiver side
 *   -----------                          -------------
 *   arq_send() -> pending=true           handle_rx(DATA, seq=N)
 *      |                                    |  ACK is ALWAYS sent (even if
 *      |  tx_raw(DATA, seq=N)               |  duplicate) so a lost ACK
 *      |  sent_ms = now                     |  doesn't strand the sender.
 *      v                                    v
 *   tick() polls every main-loop iter    last_rx_data_seq_ tracks the
 *      |                                  last delivered seq → dedup on
 *      | now >= sent_ms+ACK_TIMEOUT?      retransmits.
 *      |   no -> still waiting
 *      |   yes -> retries++, retransmit
 *      |          unless retries reached MAX -> give up (link lost)
 *      v
 *   handle_rx(DATA_ACK, seq=N) -> pending=false, fire arq_tx_ack_event_
 *
 * State invariants:
 *   - At most ONE DATA frame in flight at a time. arq_link_arq_send() refuses
 *     while pending.
 *   - arq_tx_.seq, .dst_id, .payload[] are valid IFF arq_tx_.pending == true.
 *   - last_rx_data_seq_valid_ stays false until the first DATA is delivered;
 *     after that the dedup test always runs.
 *   - arq_tx_ack_event_ is a one-shot flag — take_ack_event() clears it on
 *     read so the app surfaces exactly one TX_ACK per successful round-trip.
 *===========================================================================*/

/* Emit (or re-emit) the currently-pending ARQ DATA frame. Updates sent_ms to
 * the current tick and rolls a fresh random backoff so back-to-back retries
 * from two colliding nodes don't keep racing for the same air slot. */
static bool arq_send_now(uint32_t now_ms)
{
    bool ok = tx_raw(arq_tx_.dst_id, RF_TYPE_DATA, arq_tx_.seq,
                     arq_tx_.payload, arq_tx_.payload_len);
    arq_tx_.sent_ms = now_ms;
    arq_tx_.backoff_ms = (uint32_t)(rand_byte_() % (ARQ_BACKOFF_MAX_MS + 1u));
    return ok;
}

/* Submit a fresh DATA payload for ARQ-protected transmission. Returns false
 * if the caller must retry later: another DATA is still pending its ACK, or
 * the payload is empty / too long. On success, the frame is transmitted
 * immediately and the pending flag stays set until either an ACK lands
 * (handle_rx) or the retry budget is exhausted (tick). */
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

/* True while a DATA frame is awaiting ACK. Used by the coalescing layer to
 * gate when a new RF flush is allowed. */
bool radio_link_arq_pending(void)
{
    return arq_tx_.pending;
}

/* Consume the one-shot "last DATA was ACKed" event. Returns true exactly once
 * per ACK arrival, then resets. The app polls this in remote_periodic() and
 * surfaces it to the host as a UART_MSG_TX_ACK frame. */
bool radio_link_arq_take_ack_event(void)
{
    if (arq_tx_ack_event_) {
        arq_tx_ack_event_ = false;
        return true;
    }
    return false;
}

/* Feed an incoming RF packet into the ARQ layer. Three outcomes:
 *
 *   - DATA_ACK matching our pending seq -> clear pending, fire ack event,
 *     bump data_tx_ok_ counter. Caller does NOT deliver this packet to the
 *     app (returns NOT_DATA).
 *   - Fresh DATA -> immediately ACK back to the sender, remember seq for
 *     dedup, return DELIVER so the caller hands payload to the app.
 *   - Duplicate DATA (same seq as last delivered) -> re-ACK so the peer can
 *     clear its pending state, return DUPLICATE so the caller does NOT
 *     deliver again. This is what saves us from double-writes when the
 *     original ACK was lost in air.
 *   - Anything else (BEACON, CONNECT_*, STATS, etc.) -> NOT_DATA, caller
 *     dispatches normally.
 *
 * We ACK every valid DATA, dup or not — the peer needs the ACK to make
 * forward progress regardless of our local dedup decision. */
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

    /* ACK first, dedup second. The ack-seq is carried in the RF header so
     * the body stays empty. */
    (void)tx_raw(pkt->src_id, RF_TYPE_DATA_ACK, pkt->seq, NULL, 0u);

    if (last_rx_data_seq_valid_ && pkt->seq == last_rx_data_seq_) {
        return RX_DATA_DUPLICATE;
    }
    last_rx_data_seq_       = pkt->seq;
    last_rx_data_seq_valid_ = true;
    return RX_DATA_DELIVER;
}

/* Periodic ARQ tick — called from the app super-loop every iteration. Three
 * possible actions:
 *
 *   1. No DATA pending             -> return false (nothing to do).
 *   2. Pending, deadline not hit   -> return false (still waiting for ACK).
 *   3. Pending, deadline hit:
 *        - retries exhausted       -> clear pending, bump failed counter,
 *                                     return TRUE to signal "link is dead"
 *                                     (caller treats as ARQ_FAILED and tears
 *                                     down the session).
 *        - retries remain          -> retransmit, return false.
 *
 * The deadline is (sent_ms + ACK_TIMEOUT + backoff). The backoff is rolled
 * fresh on each (re)transmit so two retransmitting peers desync naturally.
 * The signed cast on (now_ms - deadline) handles the millis() wraparound
 * correctly (32-bit, ~49 days). */
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

/* ===== DATA coalescing ====================================================
 * Tiny producer/consumer buffer that batches small UART writes into a single
 * RF DATA packet. Without this, every byte the user types on the transparent
 * UART side of the robot would trigger its own ARQ round-trip (~120 ms air
 * time + ACK) — typing "hello" would take half a second.
 *
 * Flush policy (driven by data_buf_should_flush, evaluated each loop tick):
 *   - the buffer holds >= DATA_FLUSH_BYTES (24)         -> flush now
 *   - the oldest byte is older than DATA_FLUSH_TIMEOUT  -> flush now
 *                                       (15 ms)            (so a single
 *                                                          keystroke still
 *                                                          gets out fast)
 *
 * The buffer is single-threaded: app code pushes from the main loop only.
 * No locking needed.
 *===========================================================================*/

/* Drop everything in the coalescing buffer. Called on session teardown and
 * connect/disconnect transitions so leftover bytes from a previous session
 * don't leak into the new one. */
void radio_link_data_buf_reset(void)
{
    data_buf_len_      = 0;
    data_buf_first_ts_ = 0;
}

/* Push up to len bytes into the buffer. Returns the count actually accepted
 * (less than len if the buffer hit MAX_RF_PAYLOAD). The caller is responsible
 * for re-queuing the unaccepted tail. The first byte of an empty buffer also
 * latches the timestamp used by the flush-after-timeout rule. */
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

/* Returns true if the buffer should be flushed now. Two triggers:
 *   - enough bytes have accumulated (DATA_FLUSH_BYTES)
 *   - oldest byte has aged past DATA_FLUSH_TIMEOUT_MS
 * The caller (app layer) then pops the buffer and submits it via ARQ. */
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

/* Atomic drain: copies the buffer contents into out_payload and resets the
 * buffer. Returns the number of bytes written. Caller must size out_payload
 * to at least MAX_RF_PAYLOAD bytes. */
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

/* Current fill level — useful for diagnostics / soft back-pressure on the
 * UART RX side (e.g. stop reading the ring buffer if we can't flush yet). */
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
