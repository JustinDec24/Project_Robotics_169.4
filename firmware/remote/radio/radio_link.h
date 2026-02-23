/*============================================================================
 * radio/radio_link.h — Packet format, send/receive, ACK, duty-cycle limiter
 *
 * RF packet layout (written to CC1120 TXFIFO, variable-length mode):
 *   [LEN][NET_ID][DEV_ID][SEQ][TYPE][PAYLOAD_0 .. PAYLOAD_N-1]
 *
 * LEN = number of bytes after LEN = 4 + payload_length.
 * RF CRC is appended/checked by CC1120 hardware (not computed here).
 *===========================================================================*/
#ifndef RADIO_LINK_H
#define RADIO_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../config.h"

/* ---- RF packet types -----------------------------------------------------*/
#define RF_TYPE_CMD             0x01
#define RF_TYPE_ACK             0x02
#define RF_TYPE_TELEMETRY       0x03
#define RF_TYPE_PING            0x04
#define RF_TYPE_PING_REPLY      0x05

/* ---- Parsed RF packet ----------------------------------------------------*/
typedef struct {
    uint8_t net_id;
    uint8_t dev_id;
    uint8_t seq;
    uint8_t type;
    uint8_t payload[MAX_RF_PAYLOAD];
    uint8_t payload_len;
    int8_t  rssi;       /* filled if CC1120 append-status is enabled */
    uint8_t lqi;        /* filled if CC1120 append-status is enabled */
} rf_packet_t;

/* ---- TX result -----------------------------------------------------------*/
typedef enum {
    RF_TX_OK,
    RF_TX_BUSY,         /* duty-cycle limiter said no */
    RF_TX_FIFO_ERR,
    RF_TX_FAIL
} rf_tx_result_t;

/* ---- API -----------------------------------------------------------------*/

void            radio_link_init(void);

/* Build a packet and write it into the TX FIFO, then strobe TX. */
rf_tx_result_t  radio_link_send(uint8_t dev_id, uint8_t type,
                                const uint8_t *payload, uint8_t payload_len);

/* Check if an RF packet has been received; returns true and fills pkt. */
bool            radio_link_receive(rf_packet_t *pkt);

/* Sequence number management */
uint8_t         radio_link_next_seq(void);

/* Duty-cycle / rate-limit check and update */
bool            radio_link_tx_allowed(void);
void            radio_link_mark_tx_done(void);

#endif /* RADIO_LINK_H */
