/*============================================================================
 * radio/radio_link.c — Packet build/parse, TX/RX, ACK, duty-cycle limiter
 *===========================================================================*/
#include "radio_link.h"
#include "cc1120.h"
#include "cc1120_regs.h"
#include "../config.h"
#include "../drivers/timer.h"

static uint8_t  seq_counter  = 0;
static uint32_t last_tx_time = 0;
static uint8_t  net_id       = NET_ID_DEFAULT;

/* ===== Init ===============================================================*/

void radio_link_init(void)
{
    seq_counter  = 0;
    last_tx_time = 0;
    net_id       = NET_ID_DEFAULT;
}

/* ===== Sequence number ====================================================*/

uint8_t radio_link_next_seq(void)
{
    return seq_counter++;
}

/* ===== Duty-cycle / rate limiter ==========================================*/

bool radio_link_tx_allowed(void)
{
    uint32_t now = millis();
    uint32_t elapsed = now - last_tx_time;
    return (elapsed >= MIN_TX_INTERVAL_MS);
}

void radio_link_mark_tx_done(void)
{
    last_tx_time = millis();
}

/* ===== Send packet ========================================================*/

rf_tx_result_t radio_link_send(uint8_t dev_id, uint8_t type,
                               const uint8_t *payload, uint8_t payload_len)
{
    if (payload_len > MAX_RF_PAYLOAD)
        return RF_TX_FAIL;

    if (!radio_link_tx_allowed())
        return RF_TX_BUSY;

    /* Build the on-air frame:
     * [LEN][NET_ID][DEV_ID][SEQ][TYPE][PAYLOAD...]
     * LEN = 4 + payload_len  (bytes after LEN field)
     */
    uint8_t pkt_len = 4 + payload_len;
    uint8_t frame[1 + 4 + MAX_RF_PAYLOAD];
    uint8_t idx = 0;

    frame[idx++] = pkt_len;
    frame[idx++] = net_id;
    frame[idx++] = dev_id;
    frame[idx++] = radio_link_next_seq();
    frame[idx++] = type;

    for (uint8_t i = 0; i < payload_len; i++)
        frame[idx++] = payload[i];

    /* Ensure IDLE before loading FIFO */
    cc1120_enter_idle();

    /* Check that TX FIFO is empty */
    if (cc1120_get_txbytes() != 0) {
        cc1120_flush_tx();
    }

    /* Write frame to TX FIFO */
    cc1120_fifo_write(frame, idx);

    /* Verify FIFO loaded correctly */
    if (cc1120_get_txbytes() != idx) {
        cc1120_flush_tx();
        return RF_TX_FIFO_ERR;
    }

    /* Strobe TX */
    cc1120_strobe(CC1120_STX);
    last_tx_time = millis();

    return RF_TX_OK;
}

/* ===== Receive packet =====================================================*/

bool radio_link_receive(rf_packet_t *pkt)
{
    uint8_t num_rx = cc1120_get_rxbytes();
    if (num_rx == 0)
        return false;

    /* Read the length byte first */
    uint8_t raw[1 + RF_MAX_PKT_SIZE + 2]; /* +2 for appended RSSI+LQI */
    cc1120_fifo_read(raw, 1);
    uint8_t pkt_len = raw[0];

    if (pkt_len < 4 || pkt_len > RF_MAX_PKT_SIZE) {
        cc1120_flush_rx();
        return false;
    }

    /*
     * Read pkt_len data bytes.
     * TODO: If CC1120 "append status" is enabled in PKT_CFG1, two extra
     *       bytes (RSSI, CRC_OK|LQI) follow the payload.  Adjust the
     *       read length accordingly:
     *         cc1120_fifo_read(raw + 1, pkt_len + 2);
     *       For now, read only the data bytes:
     */
    uint8_t read_len = pkt_len;  /* + 2 if append-status enabled */
    cc1120_fifo_read(raw + 1, read_len);

    /* Parse header */
    pkt->net_id      = raw[1];
    pkt->dev_id      = raw[2];
    pkt->seq         = raw[3];
    pkt->type        = raw[4];
    pkt->payload_len = pkt_len - 4;

    for (uint8_t i = 0; i < pkt->payload_len; i++)
        pkt->payload[i] = raw[5 + i];

    /* TODO: Parse appended RSSI/LQI if enabled.
     * pkt->rssi = (int8_t)raw[1 + pkt_len];
     * pkt->lqi  = raw[1 + pkt_len + 1] & 0x7F;
     */
    pkt->rssi = 0;
    pkt->lqi  = 0;

    /* Filter by NET_ID */
    if (pkt->net_id != net_id) {
        return false;
    }

    return true;
}
