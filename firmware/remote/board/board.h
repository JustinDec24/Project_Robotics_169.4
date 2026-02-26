/*============================================================================
 * board/board.h — Hardware Abstraction Layer
 *
 * ALL pin-mapping and PIC-specific definitions live here.
 * When porting to a specific PIC, edit ONLY this file and board.c.
 *===========================================================================*/
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * PIN MAPPING — TODO: Fill in for your chosen PIC
 *
 * Example (PIC18F26K22):
 *   CS    -> RB0   (LATBbits.LATB0)
 *   RESET -> RB1   (LATBbits.LATB1)
 *   GDO0  -> RB2   (PORTBbits.RB2, external INT on this pin)
 *   LED   -> RC0   (LATCbits.LATC0)
 *   SPI SCK  -> RC3
 *   SPI MISO -> RC4
 *   SPI MOSI -> RC5
 *   UART TX  -> RC6
 *   UART RX  -> RC7
 *
 * TODO (project specific):
 *   - Confirm CC1120 CS/RESET/GDO pin mapping for your chosen PIC.
 *   - Confirm UART pins used by:
 *       REMOTE role -> USB-UART dongle to control PC.
 *       ROBOT role  -> SBC serial tty.
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

/* ---- Low-level UART hardware ---------------------------------------------*/
void board_uart_hw_init(uint32_t baud);
void board_uart_tx_byte(uint8_t b);       /* blocking: waits for TX ready */

/* ---- Low-level SPI hardware ----------------------------------------------*/
void board_spi_hw_init(void);
uint8_t board_spi_exchange(uint8_t b);    /* full-duplex single byte       */

/* ---- Low-level Timer hardware --------------------------------------------*/
void board_timer_hw_init(void);           /* configure timer for 1 ms IRQ  */

/* ---- Board-level init (oscillator, pin dirs, peripherals) ----------------*/
void board_init(void);

#endif /* BOARD_H */
