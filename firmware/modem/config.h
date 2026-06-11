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
/* RX is sized to absorb host-side bursts (e.g. PuTTY paste) that arrive at
 * 115200 baud (~11.5 KB/s) while the RF link only drains at ~350 B/s. With
 * 2048 B we can swallow paste-bursts of ~2 KB without losing bytes; beyond
 * that the firmware would need software flow control (XON/XOFF) to stall
 * the sender — not implemented today, listed as a future enhancement. */
#define UART_RX_BUF_SIZE            2048u
#define UART_TX_BUF_SIZE            512u

/* ---- Timing (ms) ---------------------------------------------------------*/
#define BEACON_INTERVAL_MS          500u
/* In CONNECTED we halve the beacon rate (1 Hz instead of 2 Hz). The RTT
 * ping from the remote already provides 1 Hz of TX-side keep-alive, so a
 * single 1 Hz beacon from the robot is enough RX-side keep-alive to cover
 * an occasional lost RTT pong. Halving the beacon rate halves the
 * collision window against the remote's DATA/STATS-PING transmits. */
#define BEACON_INTERVAL_SESSION_MS  1000u
/* CONNECT_REQ retry strategy — sized to handle marginal-link conditions
 * (PER 20-40 % at -85 to -95 dBm RSSI). 12 retries × 250 ms = 3 s budget
 * with TIMEOUT a touch above to absorb the last RTT. With these values
 * the probability of all retries failing at 30 % PER is 0.3^12 ≈ 0.05 %,
 * essentially guaranteeing connect success as long as the link is alive. */
#define CONNECT_TIMEOUT_MS          3500u
#define CONNECT_RETRY_INTERVAL_MS   250u
#define CONNECT_MAX_RETRIES         12u
#define LINK_LOST_TIMEOUT_MS        6000u  /* 12 beacon periods worth */
#define STATS_PUSH_INTERVAL_MS      500u
#define RTT_PING_INTERVAL_MS        1000u
/* App-level RX watchdog: in an active session we expect packets every
 * ~1 s (1 Hz beacon + 1 Hz RTT pong, possibly DATA). If absolutely
 * nothing is received for RX_SILENCE_REINIT_MS the chip is presumed
 * wedged in a state our defensive SRX can't unstick, and a full
 * cc1120_init_minimal() re-run is triggered. Longer than the
 * 6 s LINK_LOST_TIMEOUT so we don't fire on the same event the
 * sticky-reconnect already handles. */
#define RX_SILENCE_REINIT_MS       10000u

/* ---- Data coalescing -----------------------------------------------------*/
/* Flush threshold = MAX_RF_PAYLOAD to maximize the bytes-per-ARQ-cycle ratio
 * during bursts (paste, large outputs). Single keystrokes are unaffected —
 * they still get out after DATA_FLUSH_TIMEOUT_MS even if the buffer hasn't
 * filled. Bumped from 24 to 48 after measuring ~149 B/s instead of the
 * ~300 B/s the link can sustain. */
#define DATA_FLUSH_BYTES            48u
#define DATA_FLUSH_TIMEOUT_MS       15u

/* ---- ARQ (stop-and-wait per direction) -----------------------------------*/
/* T_ACK_MS must cover: TX air time + chip turnaround + RX air time + jitter.
 * At 4.8 kbps GFSK, a 50-byte DATA + 8-byte ACK round trip is ~110 ms.
 * At 38.4 kbps it drops to ~15 ms. We size for the slow case with margin. */
#define ARQ_ACK_TIMEOUT_MS          300u  /* a bit more headroom than 200 */
#define ARQ_MAX_RETRIES             6u    /* doubled — fewer ARQ_FAILED disconnects */
/* Random jitter must be ≥ a frame air time (50 byte DATA @ 4.8 kbps ≈
 * 100 ms) so two colliding nodes desync — a 16-ms jitter just shifts
 * the collision to the next retransmit. */
#define ARQ_BACKOFF_MAX_MS          120u
/* Per-retry growth: each retry adds 150 ms to the ACK deadline. So the
 * timeline becomes 300, 450, 600, 750, ... ms — same retry budget but
 * spread out, giving a noisy channel time to clear. */
#define ARQ_BACKOFF_GROWTH_MS       150u

/* ---- Scan cache ----------------------------------------------------------*/
#define MAX_DISCOVERED_ROBOTS       8u
/* A cache entry is considered stale (and will be removed from the host's
 * Discovered Robots panel) if no beacon has been heard from that robot
 * for SCAN_CACHE_EXPIRY_MS. With BEACON_INTERVAL_MS = 500 ms this
 * tolerates ~20 missed beacons (10 s) before the robot disappears from
 * the list — long enough to ride out a marginal-link burst, short enough
 * to drop a powered-off robot promptly. */
#define SCAN_CACHE_EXPIRY_MS       10000u

#endif /* CONFIG_H */
