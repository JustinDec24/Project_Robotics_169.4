/*============================================================================
 * radio/radio_link.h — RF packet format, ARQ for DATA, metrics
 *
 * RF payload (CC1120 variable packet length mode):
 *   [LEN][NET_ID][SRC_ID][DST_ID][TYPE][SEQ][PAYLOAD...]   + appended status
 *
 * LEN = bytes from NET_ID through end of payload (excludes appended status).
 * RF CRC is handled by CC1120 hardware.
 * APPEND_STATUS adds 2 bytes after the packet: [RSSI][CRC_OK<<7 | LQI].
 *
 * ARQ: every DATA packet uses stop-and-wait. The receiver sends a DATA_ACK
 * carrying the same SEQ. Duplicates (lost ACKs) are detected and re-ACKed
 * but not delivered to the application twice.
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
#define RF_TYPE_DATA_ACK            0x41u
#define RF_TYPE_STATS               0x50u

/* RF_TYPE_STATS payload byte 0 subtypes (RTT). */
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
    int8_t  rssi;       /* parsed from append-status byte                */
    uint8_t lqi;        /* parsed from append-status byte (low 7 bits)   */
    bool    crc_ok;     /* parsed from append-status byte (high bit)     */
} rf_packet_t;

typedef struct {
    int8_t   rssi_avg;     /* exponentially smoothed, dBm-ish (signed)   */
    uint8_t  per_pct;      /* percent of DATA frames that gave up        */
    uint16_t rtt_ms;       /* last measured RTT (ping)                   */
} link_metrics_t;

/* Outcome of feeding an RX DATA packet to the ARQ layer. */
typedef enum {
    RX_DATA_DELIVER,        /* fresh DATA — caller must deliver to app  */
    RX_DATA_DUPLICATE,      /* duplicate — caller must NOT deliver      */
    RX_DATA_NOT_DATA        /* packet was not a DATA frame              */
} rx_data_action_t;

/* ---- Lifecycle -----------------------------------------------------------*/
void    radio_link_init(uint8_t local_id, uint8_t net_id);
/* Reset per-session counters (call on connect/disconnect transitions). */
void    radio_link_session_reset(void);

/* ---- Generic send (no ARQ — used for BEACON / CONNECT_* / STATS) ---------*/
bool    radio_link_send(uint8_t dst_id, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len);

/* ---- Receive one parsed packet from the FIFO -----------------------------*/
bool    radio_link_receive(rf_packet_t *pkt);

/* ---- ARQ (DATA only) -----------------------------------------------------*/
/* Submit a DATA payload for transmission with ARQ. Returns false if a
 * previous DATA is still in flight (caller must wait or drop). */
bool    radio_link_arq_send(uint8_t dst_id, const uint8_t *payload,
                            uint8_t payload_len, uint32_t now_ms);

/* True while a DATA frame is awaiting ACK. */
bool    radio_link_arq_pending(void);

/* Feed an incoming RX packet into the ARQ layer.
 *   - If pkt is a DATA_ACK matching our pending seq -> clears pending.
 *   - If pkt is a fresh DATA -> caller must deliver, ARQ auto-sends ACK.
 *   - If pkt is a duplicate DATA -> ARQ re-ACKs, caller must NOT deliver. */
rx_data_action_t radio_link_arq_handle_rx(const rf_packet_t *pkt,
                                          uint32_t now_ms);

/* Periodic tick: handles ACK timeout and retransmission. Returns true if
 * the ARQ gave up (caller should treat link as lost). */
bool    radio_link_arq_tick(uint32_t now_ms);

/* ---- DATA coalescing buffer (UART-side -> RF) ----------------------------*/
void    radio_link_data_buf_reset(void);
/* Returns the number of bytes actually accepted (rest must be retried). */
uint8_t radio_link_data_buf_push(const uint8_t *data, uint8_t len, uint32_t now_ms);
bool    radio_link_data_buf_should_flush(uint32_t now_ms);
uint8_t radio_link_data_buf_pop(uint8_t *out_payload);
uint8_t radio_link_data_buf_count(void);

/* ---- Link metrics --------------------------------------------------------*/
void    radio_link_metrics_set_rtt(uint16_t rtt_ms);
void    radio_link_metrics_get(link_metrics_t *out);
void    radio_link_metrics_note_rssi(int8_t rssi);

/* DBG: per-failure-mode counters for radio_link_receive(). */
void    radio_link_dbg_get_counters(uint16_t *att, uint16_t *bad_len,
                                    uint16_t *trunc, uint16_t *bad_crc,
                                    uint16_t *bad_net, uint16_t *bad_dst,
                                    uint16_t *ok, uint8_t *last_len);

#endif /* RADIO_LINK_H */
