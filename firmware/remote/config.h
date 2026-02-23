/*============================================================================
 * config.h — Build-time configuration for Remote node firmware
 *
 * Project : 169.4 MHz RF link  (CC1120 + PIC)
 * Node    : REMOTE  (PC-connected controller)
 *===========================================================================*/
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Network / addressing ------------------------------------------------*/
#define NET_ID_DEFAULT          0x42
#define REMOTE_DEV_ID           0x01
#define ROBOT_DEV_ID_DEFAULT    0x10

/* ---- Payload limits ------------------------------------------------------*/
#define MAX_RF_PAYLOAD          32      /* max user payload bytes per RF pkt */
#define MAX_UART_PAYLOAD        64      /* max payload in a UART frame       */

/* ---- Timing (ms) ---------------------------------------------------------*/
#define DEFAULT_CMD_RATE_HZ     10
#define MIN_TX_INTERVAL_MS      (1000u / DEFAULT_CMD_RATE_HZ)
#define ACK_TIMEOUT_MS          100u
#define MAX_RETRIES             3

/* ---- UART framing --------------------------------------------------------*/
#define UART_SYNC1              0xAA
#define UART_SYNC2              0x55

/* ---- Ring-buffer sizes (MUST be power of 2) ------------------------------*/
#define UART_RX_BUF_SIZE        128
#define UART_TX_BUF_SIZE        128

/* ---- RF packet overhead --------------------------------------------------*/
/* LEN(1) + NET_ID(1) + DEV_ID(1) + SEQ(1) + TYPE(1) = 5 bytes before payload */
#define RF_HEADER_SIZE          5
#define RF_MAX_PKT_SIZE         (RF_HEADER_SIZE + MAX_RF_PAYLOAD)

#endif /* CONFIG_H */
