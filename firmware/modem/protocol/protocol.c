/*============================================================================
 * protocol/protocol.c — Framed UART protocol implementation
 *===========================================================================*/
#include "protocol.h"
#include "../drivers/uart.h"
#include "../util/crc8.h"
#include <string.h>

/* ===== Decoder ============================================================*/

void protocol_decoder_init(protocol_decoder_t *dec)
{
    dec->state     = PARSE_SYNC1;
    dec->len_field = 0;
    dec->idx       = 0;
}

bool protocol_decoder_feed(protocol_decoder_t *dec, uint8_t byte_in,
                           uart_frame_t *out)
{
    switch (dec->state) {

    case PARSE_SYNC1:
        if (byte_in == UART_SYNC1)
            dec->state = PARSE_SYNC2;
        break;

    case PARSE_SYNC2:
        if (byte_in == UART_SYNC2)
            dec->state = PARSE_LEN;
        else
            dec->state = (byte_in == UART_SYNC1) ? PARSE_SYNC2 : PARSE_SYNC1;
        break;

    case PARSE_LEN:
        if (byte_in == 0u || byte_in > (MAX_UART_PAYLOAD + 1u)) {
            dec->state = PARSE_SYNC1;
        } else {
            dec->len_field = byte_in;
            dec->idx       = 0;
            dec->state     = PARSE_DATA;
        }
        break;

    case PARSE_DATA:
        dec->buf[dec->idx++] = byte_in;
        if (dec->idx >= dec->len_field)
            dec->state = PARSE_CRC;
        break;

    case PARSE_CRC: {
        /* CRC covers: LEN byte + data bytes (CMD_TYPE + payload) */
        uint8_t crc = crc8_update(0x00, dec->len_field);
        for (uint8_t i = 0; i < dec->len_field; i++)
            crc = crc8_update(crc, dec->buf[i]);

        dec->state = PARSE_SYNC1;

        if (crc == byte_in) {
            out->msg_type    = dec->buf[0];
            out->payload_len = (uint8_t)(dec->len_field - 1u);
            if (out->payload_len > 0u)
                memcpy(out->payload, &dec->buf[1], out->payload_len);
            return true;
        }
        /* CRC mismatch: drop frame and resync on next frame header. */
        break;
    }

    default:
        dec->state = PARSE_SYNC1;
        break;
    }

    return false;
}

/* ===== Encoder ============================================================*/

void protocol_send_frame(uint8_t msg_type,
                         const uint8_t *payload, uint8_t payload_len)
{
    if (payload_len > MAX_UART_PAYLOAD) {
        payload_len = MAX_UART_PAYLOAD;
    }

    /* LEN covers TYPE + payload bytes. */
    uint8_t len_field = (uint8_t)(1u + payload_len);

    uart_write(UART_SYNC1);
    uart_write(UART_SYNC2);
    uart_write(len_field);

    /* CRC covers LEN + TYPE + PAYLOAD. */
    uint8_t crc = crc8_update(0x00, len_field);
    crc = crc8_update(crc, msg_type);

    uart_write(msg_type);

    for (uint8_t i = 0; i < payload_len; i++) {
        uart_write(payload[i]);
        crc = crc8_update(crc, payload[i]);
    }

    uart_write(crc);
}

/* ===== Convenience senders (Remote -> PC) =================================*/

void protocol_send_scan_result(uint8_t robot_id, uint8_t rssi, uint8_t age_100ms)
{
    uint8_t buf[3];
    buf[0] = robot_id;
    buf[1] = rssi;
    buf[2] = age_100ms;
    protocol_send_frame(UART_MSG_SCAN_RESULT, buf, 3);
}

void protocol_send_connected(uint8_t robot_id)
{
    protocol_send_frame(UART_MSG_CONNECTED, &robot_id, 1);
}

void protocol_send_disconnected(uint8_t reason)
{
    protocol_send_frame(UART_MSG_DISCONNECTED, &reason, 1);
}

void protocol_send_stats(uint8_t rssi_avg, uint8_t per_pct, uint16_t rtt_ms,
                         uint8_t retries_x10)
{
    uint8_t buf[5];
    buf[0] = rssi_avg;
    buf[1] = per_pct;
    buf[2] = (uint8_t)(rtt_ms & 0xFFu);
    buf[3] = (uint8_t)((rtt_ms >> 8) & 0xFFu);
    buf[4] = retries_x10;
    protocol_send_frame(UART_MSG_STATS, buf, 5);
}

void protocol_send_data_rx(const uint8_t *data, uint8_t len)
{
    protocol_send_frame(UART_MSG_DATA_RX, data, len);
}

void protocol_send_log(const char *text)
{
    uint8_t len = 0;
    while (text[len] != '\0' && len < MAX_UART_PAYLOAD) {
        len++;
    }
    protocol_send_frame(UART_MSG_LOG, (const uint8_t *)text, len);
}

void protocol_send_tx_ack(void)
{
    protocol_send_frame(UART_MSG_TX_ACK, NULL, 0);
}
