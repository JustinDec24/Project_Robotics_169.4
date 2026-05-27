/*============================================================================
 * radio/cc1120.h — CC1120 low-level SPI command interface
 *===========================================================================*/
#ifndef CC1120_H
#define CC1120_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Initialization / reset ----------------------------------------------*/
void    cc1120_reset(void);
bool    cc1120_init_minimal(void);
/* Same as cc1120_init_minimal, but skips the SCAL strobe at the end. Used
 * during bring-up debug — SCAL can heat the chip if registers are wrong. */
bool    cc1120_init_no_scal(void);

/* ---- Generic command strobe ----------------------------------------------*/
uint8_t cc1120_strobe(uint8_t strobe_addr);

/* ---- Standard register access (addr 0x00 – 0x2E) ------------------------*/
uint8_t cc1120_reg_read(uint8_t addr);
void    cc1120_reg_write(uint8_t addr, uint8_t val);
void    cc1120_reg_burst_read(uint8_t addr, uint8_t *buf, size_t len);
void    cc1120_reg_burst_write(uint8_t addr, const uint8_t *buf, size_t len);

/* ---- Extended register access (addr via 0x2F prefix) --------------------*/
uint8_t cc1120_ext_reg_read(uint8_t ext_addr);
void    cc1120_ext_reg_write(uint8_t ext_addr, uint8_t val);

/* ---- FIFO access ---------------------------------------------------------*/
void    cc1120_write_txfifo(const uint8_t *buf, size_t len);
void    cc1120_read_rxfifo(uint8_t *buf, size_t len);

/* ---- Status helpers ------------------------------------------------------*/
uint8_t cc1120_get_status(void);
uint8_t cc1120_get_marcstate(void);
uint8_t cc1120_get_num_rxbytes(void);
uint8_t cc1120_get_num_txbytes(void);

/* ---- Mode helpers --------------------------------------------------------*/
void    cc1120_set_rx(void);
void    cc1120_set_idle(void);
void    cc1120_flush_rx(void);
void    cc1120_flush_tx(void);

/* ---- Convenience single-strobe wrappers ----------------------------------*/
uint8_t cc1120_strobe_stx(void);
uint8_t cc1120_strobe_srx(void);
uint8_t cc1120_strobe_sidle(void);
uint8_t cc1120_strobe_sfrx(void);
uint8_t cc1120_strobe_sftx(void);
uint8_t cc1120_strobe_sres(void);
uint8_t cc1120_strobe_snop(void);

/* ---- Radio event flags (bitmask) -----------------------------------------*/
#define RADIO_EVT_RX_DONE          (1u << 0)
#define RADIO_EVT_TX_DONE          (1u << 1)
#define RADIO_EVT_RX_OVERFLOW      (1u << 2)
#define RADIO_EVT_TX_UNDERFLOW     (1u << 3)
#define RADIO_EVT_GDO_EDGE         (1u << 4)

/* GDO ISR entry point — called from PIC interrupt context.
 * Latches a raw IRQ flag; call cc1120_process_events() from the main loop
 * to classify it into the semantic flags above. */
void    isr_cc1120_gdo(void);

/* Classify a pending GDO IRQ into RADIO_EVT_* flags.
 * Safe to call from main-loop context (performs SPI reads). */
void    cc1120_process_events(void);

/* Return the flags that match 'mask' and atomically clear them. */
uint8_t cc1120_events_get_and_clear(uint8_t mask);

/* Clear every pending event flag. */
void    cc1120_events_clear_all(void);

#endif /* CC1120_H */
