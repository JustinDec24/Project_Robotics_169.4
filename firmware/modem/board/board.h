/*============================================================================
 * board/board.h — Hardware Abstraction Layer
 *
 * Target : PIC18F26Q10-I/SS, internal HFINTOSC @ 32 MHz (Fcy = 8 MIPS).
 *          The board's only crystal is the 32 MHz one wired to the CC1120.
 * All PIC-specific code lives in board.c. The rest of the firmware is portable.
 *===========================================================================*/
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * PIN MAPPING — PIC18F26Q10-I/SS (28-pin SSOP)
 *
 * RA0  : RTS#  (out)  -> FT231 RTS  (driven LOW = ready to RX)
 * RA1  : CTS#  (in, currently ignored)  <- FT231 CTS
 * RA2..RA7 : free for GPIO (no PIC crystal — HFINTOSC used internally).
 *
 * RB0  : CC1120 CSn    (out, active low, idle high)
 * RB1  : CC1120 GDO0   (in, INT0 via PPS)
 * RB2..RB5 : unused
 * RB6  : ICSP CLK
 * RB7  : ICSP DAT
 *
 * RC2  : LED debug    (out, active high)
 * RC3  : SCK1   (PPS) -> CC1120 SCLK
 * RC4  : SDI1   (PPS) <- CC1120 SO  (MISO)
 * RC5  : SDO1   (PPS) -> CC1120 SI  (MOSI)
 * RC6  : RX1    (PPS) <- FT231 TXD
 * RC7  : TX1    (PPS) -> FT231 RXD
 *
 * NOTE: CC1120 RESET_N is NOT wired to the PIC on this board. Soft reset
 *       only via SRES SPI strobe. cc1120_reset_assert/deassert are no-ops.
 *
 * If your board differs, change ONLY the PIN_* macros at the top of board.c.
 * ====================================================================*/

/* ---- CC1120 chip-select --------------------------------------------------*/
void cc1120_cs_assert(void);
void cc1120_cs_deassert(void);

/* ---- CC1120 hardware reset -----------------------------------------------*/
void cc1120_reset_assert(void);
void cc1120_reset_deassert(void);

/* ---- CC1120 GDO0 input ---------------------------------------------------*/
uint8_t cc1120_gdo0_read(void);

/* ---- Status LED (optional) -----------------------------------------------*/
void led_on(void);
void led_off(void);
void led_toggle(void);

/* ---- Delay helpers -------------------------------------------------------*/
void Delay_ms(uint16_t ms);
void Delay_us(uint16_t us);

/* ---- Interrupt control ---------------------------------------------------*/
void global_int_enable(void);
void global_int_disable(void);

/* Save current GIE state, mask interrupts, return a token usable with
 * board_int_restore(). Use this around non-atomic reads of variables
 * shared with ISRs. Safe to nest. */
uint8_t board_int_save_disable(void);
void    board_int_restore(uint8_t saved_token);

/* ---- Low-level UART hardware ---------------------------------------------*/
void board_uart_hw_init(uint32_t baud);
/* Trigger the TXIE-driven send path. The driver pumps board_uart_tx_ready()
 * from its ISR helper. */
void board_uart_tx_irq_enable(void);
void board_uart_tx_irq_disable(void);
bool board_uart_tx_ready(void);            /* true when TX register is empty */
void board_uart_tx_byte_raw(uint8_t b);    /* write to TX register, no wait  */

/* ---- Low-level SPI hardware ----------------------------------------------*/
void    board_spi_hw_init(void);
uint8_t board_spi_exchange(uint8_t b);     /* full-duplex single byte        */

/* ---- Low-level Timer hardware --------------------------------------------*/
void board_timer_hw_init(void);            /* configure timer for 1 ms IRQ   */

/* ---- GDO0 external interrupt control -------------------------------------*/
void board_cc1120_int_enable(void);
void board_cc1120_int_disable(void);

/* ---- Board-level init (oscillator, pin dirs, peripherals) ----------------*/
void board_init(void);

#endif /* BOARD_H */
