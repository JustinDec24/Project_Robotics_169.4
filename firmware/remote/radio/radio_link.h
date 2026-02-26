/*============================================================================
 * radio/radio_link.h — RF packet format + coalescing + metrics
 *
 * RF payload (CC1120 variable packet length mode):
 *   [LEN][NET_ID][SRC_ID][DST_ID][TYPE][SEQ][PAYLOAD...]
 *
 * LEN = bytes from NET_ID through end of payload.
 * RF CRC is handled by CC1120 hardware.
 *===========================================================================*/
#ifndef RADIO_LINK_H
#define RADIO_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

/* ---- RF packet types -----------------------------------------------------*/
#define RF_TYPE_BEACON              0x20u
#define RF_TYPE_CONNECT_REQ         0x30u
#define RF_TYPE_CONNECT_OK          0x31u
#define RF_TYPE_DISCONNECT          0x32u
#define RF_TYPE_DATA                0x40u
#define RF_TYPE_STATS               0x50u

/* RF_TYPE_STATS payload byte 0 subtypes (RTT placeholder). */
#define RF_STATS_PING_REQ           0x01u
#define RF_STATS_PING_RESP          0x02u

/* ---- Parsed RF packet ----------------------------------------------------*/
typedef struct {
    uint8_t net_id;
    uint8_t src_id;
    uint8_t dst_id;
    uint8_t type;
    uint8_t seq;
    uint8_t payload[MAX_RF_PAYLOAD];
    uint8_t payload_len;
    int8_t  rssi;       /* TODO: fill from append status bytes. */
    uint8_t lqi;        /* TODO: fill from append status bytes. */
} rf_packet_t;

typedef struct {
    uint8_t  rssi_avg;
    uint8_t  per_pct;
    uint16_t rtt_ms;
} link_metrics_t;

/* ---- API -----------------------------------------------------------------*/
void    radio_link_init(uint8_t local_id, uint8_t net_id);
bool    radio_link_send(uint8_t dst_id, uint8_t type, const uint8_t *payload, uint8_t payload_len);
bool    radio_link_receive(rf_packet_t *pkt);
uint8_t radio_link_next_seq(void);

/* Outgoing DATA packet coalescing helper (fixed-size, no malloc). */
void    radio_link_data_buf_reset(void);
void    radio_link_data_buf_push(const uint8_t *data, uint8_t len, uint32_t now_ms);
bool    radio_link_data_buf_should_flush(uint32_t now_ms);
uint8_t radio_link_data_buf_pop(uint8_t *out_payload);

/* Link metrics helper. */
void    radio_link_metrics_set_rtt(uint16_t rtt_ms);
void    radio_link_metrics_get(link_metrics_t *out);

#endif /* RADIO_LINK_H */
