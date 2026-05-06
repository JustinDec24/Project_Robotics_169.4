/*============================================================================
 * config.h — Build-time configuration for dual-role modem firmware
 *
 * Target MCU : Microchip PIC18F26Q10-I/SS (28-pin SSOP)
 * Clock      : Internal HFINTOSC @ 32 MHz -> Fosc=32 MHz, Fcy=8 MIPS.
 *              (The 32 MHz crystal on the board is wired to the CC1120's
 *               XOSC_Q1/Q2; the PIC has no external crystal.)
 * Radio      : TI CC1120 @ 169.4 MHz, GFSK, 32 MHz xosc reference
 * UART link  : 115200 8N1 both sides (PC<->Remote and Robot<->SBC)
 *===========================================================================*/
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Build role ----------------------------------------------------------*/
#define MODEM_ROLE_REMOTE       1u
#define MODEM_ROLE_ROBOT        2u

/* Default build role if project flags do not define MODEM_ROLE. */
#ifndef MODEM_ROLE
#define MODEM_ROLE MODEM_ROLE_REMOTE
#endif

#if (MODEM_ROLE != MODEM_ROLE_REMOTE) && (MODEM_ROLE != MODEM_ROLE_ROBOT)
#error "MODEM_ROLE must be MODEM_ROLE_REMOTE or MODEM_ROLE_ROBOT"
#endif

/* ---- Clock ---------------------------------------------------------------*/
/* Internal HFINTOSC at 32 MHz (no external crystal on the PIC).
 * Fosc = 32 MHz, instruction cycle Fcy = Fosc/4 = 8 MHz. */
#define _XTAL_FREQ              32000000UL    /* used by __delay_ms / __delay_us */
#define FOSC_HZ                 32000000UL
#define FCY_HZ                  8000000UL

/* ---- Network / node IDs --------------------------------------------------*/
#define NET_ID_DEFAULT              0x42u
#define REMOTE_MODEM_ID             0x01u
#define ROBOT_MODEM_ID_DEFAULT      0x10u
#define RF_BROADCAST_ID             0xFFu

/* ---- Payload limits ------------------------------------------------------*/
#define MAX_RF_PAYLOAD              48u   /* max bytes after RF TYPE/SEQ      */
#define MAX_UART_PAYLOAD            96u   /* max bytes in framed UART payload */

/* ---- RF packet overhead --------------------------------------------------*/
/* RF body bytes after LEN: NET_ID + SRC + DST + TYPE + SEQ + PAYLOAD */
#define RF_HEADER_SIZE              5u
#define RF_MAX_BODY_SIZE            (RF_HEADER_SIZE + MAX_RF_PAYLOAD)
#define RF_MAX_PKT_SIZE             (1u + RF_MAX_BODY_SIZE) /* includes LEN */
/* Two extra bytes appended by CC1120 when APPEND_STATUS is enabled
 * (RSSI byte + LQI/CRC_OK byte). */
#define RF_APPEND_STATUS_BYTES      2u

/* ---- UART framing --------------------------------------------------------*/
#define UART_SYNC1                  0xAAu
#define UART_SYNC2                  0x55u
#define UART_BAUD_DEFAULT           115200u

/* ---- Ring-buffer sizes (MUST be power of 2) ------------------------------*/
#define UART_RX_BUF_SIZE            512u
#define UART_TX_BUF_SIZE            512u

/* ---- Timing (ms) ---------------------------------------------------------*/
#define BEACON_INTERVAL_MS          500u
#define CONNECT_TIMEOUT_MS          800u
#define LINK_LOST_TIMEOUT_MS        3000u
#define STATS_PUSH_INTERVAL_MS      500u
#define RTT_PING_INTERVAL_MS        1000u

/* ---- Data coalescing -----------------------------------------------------*/
#define DATA_FLUSH_BYTES            24u
#define DATA_FLUSH_TIMEOUT_MS       15u

/* ---- ARQ (stop-and-wait per direction) -----------------------------------*/
/* T_ACK_MS must cover: TX air time + chip turnaround + RX air time + jitter.
 * At 4.8 kbps GFSK, a 50-byte DATA + 8-byte ACK round trip is ~110 ms.
 * At 38.4 kbps it drops to ~15 ms. We size for the slow case with margin. */
#define ARQ_ACK_TIMEOUT_MS          200u
#define ARQ_MAX_RETRIES             3u
#define ARQ_BACKOFF_MAX_MS          16u   /* random jitter before retransmit */

/* ---- Scan cache ----------------------------------------------------------*/
#define MAX_DISCOVERED_ROBOTS       8u

#endif /* CONFIG_H */
