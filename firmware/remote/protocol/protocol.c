/*============================================================================
 * protocol/protocol.c — UART binary framing implementation
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
        if (byte_in == 0 || byte_in > (MAX_UART_PAYLOAD + 1)) {
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
            out->cmd_type    = dec->buf[0];
            out->payload_len = dec->len_field - 1;
            if (out->payload_len > 0)
                memcpy(out->payload, &dec->buf[1], out->payload_len);
            return true;
        }
        /* CRC mismatch — frame dropped silently */
        break;
    }

    default:
        dec->state = PARSE_SYNC1;
        break;
    }

    return false;
}

/* ===== Encoder ============================================================*/

void protocol_send_frame(uint8_t cmd_type,
                         const uint8_t *payload, uint8_t payload_len)
{
    uint8_t len_field = 1 + payload_len;    /* CMD_TYPE + payload */

    uart_write(UART_SYNC1);
    uart_write(UART_SYNC2);
    uart_write(len_field);

    /* Compute CRC over LEN + CMD_TYPE + PAYLOAD */
    uint8_t crc = crc8_update(0x00, len_field);
    crc = crc8_update(crc, cmd_type);

    uart_write(cmd_type);

    for (uint8_t i = 0; i < payload_len; i++) {
        uart_write(payload[i]);
        crc = crc8_update(crc, payload[i]);
    }

    uart_write(crc);
}

/* ===== Convenience senders ================================================*/

void protocol_send_log(const char *text)
{
    uint8_t len = 0;
    while (text[len] != '\0' && len < MAX_UART_PAYLOAD)
        len++;
    protocol_send_frame(EVT_LOG, (const uint8_t *)text, len);
}

void protocol_send_ack_status(uint8_t seq, uint8_t result)
{
    uint8_t buf[2];
    buf[0] = seq;
    buf[1] = result;
    protocol_send_frame(EVT_ACK_STATUS, buf, 2);
}

void protocol_send_telemetry(const uint8_t *data, uint8_t len)
{
    protocol_send_frame(EVT_TELEMETRY, data, len);
}
