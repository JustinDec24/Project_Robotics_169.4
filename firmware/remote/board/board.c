/*============================================================================
 * board/board.c — Hardware Abstraction Layer  (PIC-specific stubs)
 *
 * TODO: Replace every stub body with actual register accesses for your PIC.
 *       #include <xc.h> once you have selected the PIC in MPLAB X.
 *===========================================================================*/

/* TODO: Uncomment once a PIC device is selected in the MPLAB X project.
 * #include <xc.h>
 */
#include "board.h"

/* Pull in ISR entry-points defined in driver modules */
#include "../drivers/uart.h"
#include "../drivers/timer.h"
#include "../radio/cc1120.h"

/*--------------------------------------------------------------------------
 * PIC Configuration bits — TODO
 *
 * #pragma config FOSC  = ...
 * #pragma config WDTEN = OFF
 * #pragma config LVP   = OFF
 * ...
 *-------------------------------------------------------------------------*/

/* ===== CC1120 Chip Select ================================================*/
void cc1120_cs_assert(void)
{
    /* TODO: e.g. LATBbits.LATB0 = 0; */
}

void cc1120_cs_deassert(void)
{
    /* TODO: e.g. LATBbits.LATB0 = 1; */
}

/* ===== CC1120 Reset ======================================================*/
void cc1120_reset_assert(void)
{
    /* TODO: e.g. LATBbits.LATB1 = 0; */
}

void cc1120_reset_deassert(void)
{
    /* TODO: e.g. LATBbits.LATB1 = 1; */
}

/* ===== CC1120 GDO0 =======================================================*/
uint8_t cc1120_gdo0_read(void)
{
    /* TODO: e.g. return PORTBbits.RB2; */
    return 0;
}

/* ===== LED ================================================================*/
void led_on(void)
{
    /* TODO: e.g. LATCbits.LATC0 = 1; */
}

void led_off(void)
{
    /* TODO: e.g. LATCbits.LATC0 = 0; */
}

void led_toggle(void)
{
    /* TODO: e.g. LATCbits.LATC0 ^= 1; */
}

/* ===== Delay ==============================================================*/
void Delay_ms(uint16_t ms)
{
    /*
     * TODO: Use __delay_ms() from <xc.h> (requires _XTAL_FREQ defined),
     * or implement a loop calibrated to your clock frequency.
     * Naive busy-wait placeholder:
     */
    while (ms--) {
        Delay_us(1000);
    }
}

void Delay_us(uint16_t us)
{
    /*
     * TODO: Calibrate for your Fosc.
     * With _XTAL_FREQ defined you can use __delay_us() for short delays.
     */
    (void)us;
}

/* ===== Interrupt control ==================================================*/
void global_int_enable(void)
{
    /* TODO: e.g.  INTCONbits.GIE = 1; INTCONbits.PEIE = 1; */
}

void global_int_disable(void)
{
    /* TODO: e.g.  INTCONbits.GIE = 0; */
}

/* ===== UART hardware =====================================================*/
void board_uart_hw_init(uint32_t baud)
{
    (void)baud;
    /*
     * TODO: Configure UART/EUSART peripheral.
     *  - Set TX pin as output, RX pin as input
     *  - Compute SPBRG from Fosc and desired baud
     *  - Enable transmitter and receiver
     *  - Enable receive interrupt (RCIE=1, PEIE=1)
     *
     * Example (PIC18F, 8 MHz, 9600 baud, BRGH=1, BRG16=1):
     *   SPBRG  = 207;
     *   TXSTAbits.BRGH = 1;
     *   BAUDCONbits.BRG16 = 1;
     *   TXSTAbits.TXEN = 1;
     *   RCSTAbits.SPEN = 1;
     *   RCSTAbits.CREN = 1;
     *   PIE1bits.RCIE  = 1;
     */
}

void board_uart_tx_byte(uint8_t b)
{
    /*
     * TODO: Wait for TX buffer empty, then write.
     * Example:
     *   while (!PIR1bits.TXIF);
     *   TXREG = b;
     */
    (void)b;
}

/* ===== SPI hardware ======================================================*/
void board_spi_hw_init(void)
{
    /*
     * TODO: Configure MSSP/SPI peripheral.
     *  - SPI master mode
     *  - Clock polarity/phase per CC1120 spec (CPOL=0, CPHA=0)
     *  - Clock rate ≤ 10 MHz (CC1120 max SCLK)
     *  - SCK, MOSI as outputs; MISO as input
     *  - CS handled manually via cc1120_cs_*()
     *
     * Example (PIC18F MSSP):
     *   SSPSTATbits.SMP = 1;
     *   SSPSTATbits.CKE = 1;
     *   SSPCON1bits.CKP = 0;
     *   SSPCON1bits.SSPM = 0b0001;  // SPI master, Fosc/16
     *   SSPCON1bits.SSPEN = 1;
     */
}

uint8_t board_spi_exchange(uint8_t b)
{
    /*
     * TODO: Write byte to SPI buffer, wait for completion, return received byte.
     * Example:
     *   SSPBUF = b;
     *   while (!SSPSTATbits.BF);
     *   return SSPBUF;
     */
    (void)b;
    return 0x00;
}

/* ===== Timer hardware ====================================================*/
void board_timer_hw_init(void)
{
    /*
     * TODO: Configure a timer to generate an interrupt every 1 ms.
     *
     * Example (PIC18F, Timer0, 8 MHz Fosc):
     *   T0CON = 0b11000010;  // 8-bit, prescaler 1:8
     *   TMR0  = 256 - 250;   // 250 × 4us = 1 ms
     *   INTCONbits.TMR0IE = 1;
     */
}

/* ===== Board init ========================================================*/
void board_init(void)
{
    /*
     * TODO: Oscillator selection, ANSEL/ANSELA clearing, pin directions.
     *
     * Typical sequence:
     *  1. Set oscillator (internal / external crystal)
     *  2. Disable analog on all used digital pins (ANSELx = 0)
     *  3. Set TRIS for CS, RESET, LED as outputs
     *  4. Set TRIS for GDO0 as input
     *  5. Default pin states (CS high, RESET high, LED off)
     */

    cc1120_cs_deassert();
    cc1120_reset_deassert();
    led_off();
}

/* ===== Interrupt Service Routine template =================================
 *
 * TODO: Wire this into the PIC's ISR vector.  For XC8:
 *
 * void __interrupt() ISR(void)
 * {
 *     // --- UART RX ---
 *     if (PIR1bits.RCIF) {
 *         uint8_t b = RCREG;       // reading RCREG clears RCIF
 *         isr_uart_rx_byte(b);
 *     }
 *
 *     // --- Timer 1 ms tick ---
 *     if (INTCONbits.TMR0IF) {
 *         INTCONbits.TMR0IF = 0;
 *         TMR0 = 256 - 250;       // reload for next 1 ms
 *         isr_timer_tick_1ms();
 *     }
 *
 *     // --- CC1120 GDO0 external interrupt ---
 *     if (INTCONbits.INT0IF) {     // or INTCON3bits.INT1IF etc.
 *         INTCONbits.INT0IF = 0;
 *         isr_cc1120_gdo();
 *     }
 * }
 *
 *=========================================================================*/
