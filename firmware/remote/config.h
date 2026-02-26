/*============================================================================
 * config.h — Build-time configuration for dual-role modem firmware
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

/* ---- UART framing --------------------------------------------------------*/
#define UART_SYNC1                  0xAAu
#define UART_SYNC2                  0x55u

/* ---- Ring-buffer sizes (MUST be power of 2) ------------------------------*/
#define UART_RX_BUF_SIZE            256u

/* ---- Timing (ms) ---------------------------------------------------------*/
#define BEACON_INTERVAL_MS          500u
#define CONNECT_TIMEOUT_MS          800u
#define LINK_LOST_TIMEOUT_MS        3000u
#define STATS_PUSH_INTERVAL_MS      500u
#define RTT_PING_INTERVAL_MS        1000u

/* ---- Data coalescing -----------------------------------------------------*/
#define DATA_FLUSH_BYTES            24u
#define DATA_FLUSH_TIMEOUT_MS       15u

/* ---- Scan cache ----------------------------------------------------------*/
#define MAX_DISCOVERED_ROBOTS       8u

#endif /* CONFIG_H */
