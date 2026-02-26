/*============================================================================
 * protocol/protocol.h — Framed UART protocol (PC <-> Remote modem)
 *
 * Frame format:
 *   [0xAA][0x55][LEN][TYPE][PAYLOAD...][CRC8]
 * LEN = number of bytes from TYPE through PAYLOAD.
 * CRC8 is computed over [LEN, TYPE, PAYLOAD...].
 *===========================================================================*/
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

/* ---- PC -> Remote types ---------------------------------------------------*/
#define UART_MSG_SCAN_START      0x01u
#define UART_MSG_SCAN_STOP       0x02u
#define UART_MSG_CONNECT         0x03u
#define UART_MSG_DISCONNECT      0x04u
#define UART_MSG_GET_STATS       0x05u
#define UART_MSG_DATA_TX         0x10u

/* ---- Remote -> PC types ---------------------------------------------------*/
#define UART_MSG_SCAN_RESULT     0x81u
#define UART_MSG_CONNECTED       0x82u
#define UART_MSG_DISCONNECTED    0x83u
#define UART_MSG_STATS           0x84u
#define UART_MSG_DATA_RX         0x90u
#define UART_MSG_LOG             0x9Fu

/* ---- Parsed UART frame ---------------------------------------------------*/
typedef struct {
    uint8_t msg_type;
    uint8_t payload[MAX_UART_PAYLOAD];
    uint8_t payload_len;
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

/* ---- Generic encoder ------------------------------------------------------*/
void protocol_send_frame(uint8_t msg_type,
                         const uint8_t *payload, uint8_t payload_len);

/* ---- Convenience encoders (Remote -> PC) ---------------------------------*/
void protocol_send_scan_result(uint8_t robot_id, uint8_t rssi, uint8_t age_100ms);
void protocol_send_connected(uint8_t robot_id);
void protocol_send_disconnected(uint8_t reason);
void protocol_send_stats(uint8_t rssi_avg, uint8_t per_pct, uint16_t rtt_ms);
void protocol_send_data_rx(const uint8_t *data, uint8_t len);
void protocol_send_log(const char *text);

#endif /* PROTOCOL_H */
