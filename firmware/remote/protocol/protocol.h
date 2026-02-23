/*============================================================================
 * protocol/protocol.h — UART binary framing  (PC <-> Remote)
 *
 * Frame format:
 *   [SYNC1=0xAA][SYNC2=0x55][LEN][CMD_TYPE][PAYLOAD...][CRC8]
 *
 * LEN = number of bytes from CMD_TYPE through end of PAYLOAD (excludes CRC8).
 * CRC8 is computed over bytes [LEN .. last payload byte].
 *===========================================================================*/
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

/* ---- Command types (PC -> Remote) ----------------------------------------*/
#define CMD_MOVE        0x01    /* payload: 2 bytes (e.g. left_speed, right_speed) */
#define CMD_STOP        0x02    /* payload: 0 bytes                                */
#define CMD_PING        0x03    /* payload: 0 bytes                                */
#define CMD_SET_PARAM   0x04    /* payload: key(1) + value(N), placeholder         */

/* ---- Event types (Remote -> PC) ------------------------------------------*/
#define EVT_LOG         0x80    /* payload: ASCII text (no null terminator)  */
#define EVT_ACK_STATUS  0x81    /* payload: seq(1) + result(1)              */
#define EVT_TELEMETRY   0x82    /* payload: variable telemetry data         */

/* ---- ACK result codes ----------------------------------------------------*/
#define ACK_OK          0x00
#define ACK_FAIL        0x01
#define ACK_TIMEOUT     0x02

/* ---- Parsed UART frame ---------------------------------------------------*/
typedef struct {
    uint8_t cmd_type;
    uint8_t payload[MAX_UART_PAYLOAD];
    uint8_t payload_len;                /* bytes in payload (LEN - 1) */
} uart_frame_t;

/* ---- Decoder (processes one byte at a time, called from main loop) -------*/
typedef enum {
    PARSE_SYNC1,
    PARSE_SYNC2,
    PARSE_LEN,
    PARSE_DATA,
    PARSE_CRC
} parse_state_t;

typedef struct {
    parse_state_t state;
    uint8_t       buf[MAX_UART_PAYLOAD + 1];   /* CMD_TYPE + payload */
    uint8_t       len_field;                    /* LEN from frame    */
    uint8_t       idx;
} protocol_decoder_t;

void protocol_decoder_init(protocol_decoder_t *dec);
bool protocol_decoder_feed(protocol_decoder_t *dec, uint8_t byte_in,
                           uart_frame_t *out);

/* ---- Encoder (builds & sends a frame over UART) -------------------------*/
void protocol_send_frame(uint8_t cmd_type,
                         const uint8_t *payload, uint8_t payload_len);

/* ---- Convenience: send a text log line to PC ----------------------------*/
void protocol_send_log(const char *text);

/* ---- Convenience: send ACK status to PC ---------------------------------*/
void protocol_send_ack_status(uint8_t seq, uint8_t result);

/* ---- Convenience: send telemetry to PC ----------------------------------*/
void protocol_send_telemetry(const uint8_t *data, uint8_t len);

#endif /* PROTOCOL_H */
