/*============================================================================
 * board/board.c — PIC18F26Q10-I/SS hardware abstraction
 *
 * Clock      : Internal HFINTOSC at 32 MHz (no external crystal on the PIC).
 *              Fosc = 32 MHz, Fcy = 8 MIPS.
 *              The 32 MHz crystal on the board is on the CC1120 (XOSC_Q1/Q2),
 *              not on the PIC. RA6/RA7 are therefore available as GPIO.
 * Toolchain  : Microchip XC8 (set _XTAL_FREQ=32000000 in MPLAB X project,
 *              or rely on the value provided by config.h).
 *
 * Pin map (change the PIN_* macros below if your wiring differs):
 *   RA0  : RTS#  (out)               -> FT231 RTS  (driven low = ready to RX)
 *   RA1  : CTS#  (in, currently ignored) <- FT231 CTS
 *   RA6  : OSC2                      (crystal — driven by oscillator)
 *   RA7  : OSC1                      (crystal — driven by oscillator)
 *   RB0  : CC1120 CSn                (out, active low, idle high)
 *   RB1  : CC1120 GDO0               (in, INT0 via PPS)
 *   RB6  : ICSPCLK
 *   RB7  : ICSPDAT
 *   RC2  : LED debug                 (out, active high)
 *   RC3  : SCK1   (PPS)              -> CC1120 SCLK
 *   RC4  : SDI1   (PPS in)           <- CC1120 SO  (MISO)
 *   RC5  : SDO1   (PPS out)          -> CC1120 SI  (MOSI)
 *   RC6  : ROBOT build  -> RX1 (PPS in)  <- miniuart3 TXD
 *          REMOTE build -> TX1 (PPS out) -> FT231 RXD (input on FT231)
 *   RC7  : ROBOT build  -> TX1 (PPS out) -> miniuart3 RXD
 *          REMOTE build -> RX1 (PPS in)  <- FT231 TXD (output from FT231)
 *
 * Note: CC1120 RESET_N is NOT wired to the PIC on this board. The chip is
 *       reset purely via the SRES SPI strobe (cc1120_reset() in radio/cc1120.c).
 *       cc1120_reset_assert/deassert are stubs.
 *
 * The ISR vector at the bottom dispatches: EUSART RX, EUSART TX, Timer0,
 * INT0 (CC1120 GDO0).
 *===========================================================================*/

#include <xc.h>
#include "board.h"
#include "../config.h"
#include "../drivers/uart.h"
#include "../drivers/timer.h"
#include "../radio/cc1120.h"

/* ===========================================================================
 *                         Configuration bits
 *=========================================================================*/
/* CONFIG1L */
#pragma config FEXTOSC = OFF            /* No external oscillator on the PIC */
#pragma config RSTOSC  = HFINTOSC_1MHZ  /* Boot on HFINTOSC@1MHz; we promote
                                           to 32 MHz in board_init(). */
/* CONFIG1H */
#pragma config CLKOUTEN = OFF
#pragma config CSWEN    = ON        /* Allow runtime clock switching */
#pragma config FCMEN    = OFF
/* CONFIG2L */
#pragma config MCLRE = EXTMCLR
#pragma config PWRTE = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS
/* CONFIG2H */
#pragma config BORV   = VBOR_245
#pragma config ZCD    = OFF
#pragma config PPS1WAY = OFF        /* Allow re-mapping PPS at runtime */
#pragma config STVREN = ON
#pragma config XINST  = OFF
/* CONFIG3L — Watchdog ON, period ~4.1 s (WDTCPS_13 = 1:131072 on
 * LFINTOSC@31 kHz). Any code path that blocks the main loop for more
 * than 4 s (failing SPI, wedged CC1120, accidental infinite loop)
 * triggers a reset — essential for the robot which has nobody around
 * to press a reset button. The main loop calls CLRWDT() each iteration. */
#pragma config WDTCPS = WDTCPS_13
#pragma config WDTE   = ON
/* CONFIG3H */
#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC
/* CONFIG4L/H, CONFIG5L/H — flash protection / config write — leave defaults */
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRTC = OFF
#pragma config WRTB = OFF
#pragma config WRTD = OFF
#pragma config SCANE = ON
#pragma config LVP   = ON           /* Low-voltage programming on (typical) */
#pragma config CP   = OFF
#pragma config CPD  = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF

/* ===========================================================================
 *                       Pin map (edit here if wiring differs)
 *=========================================================================*/

/* CC1120 chip-select on RB0 (active low, idle high) */
#define PIN_CC_CS_LAT       LATBbits.LATB0
#define PIN_CC_CS_TRIS      TRISBbits.TRISB0
#define PIN_CC_CS_ANSEL     ANSELBbits.ANSELB0

/* CC1120 GDO0 on RB1 (input, mapped to INT0 via PPS) */
#define PIN_CC_GDO_PORT     PORTBbits.RB1
#define PIN_CC_GDO_TRIS     TRISBbits.TRISB1
#define PIN_CC_GDO_ANSEL    ANSELBbits.ANSELB1

/* SPI on RC3/RC4/RC5 */
#define PIN_SCK_TRIS        TRISCbits.TRISC3
#define PIN_SCK_ANSEL       ANSELCbits.ANSELC3
#define PIN_SDI_TRIS        TRISCbits.TRISC4
#define PIN_SDI_ANSEL       ANSELCbits.ANSELC4
#define PIN_SDO_TRIS        TRISCbits.TRISC5
#define PIN_SDO_ANSEL       ANSELCbits.ANSELC5

/* EUSART pin assignment depends on the PCB wiring.
 *
 *   ROBOT board : MCU TX on RC7, MCU RX on RC6 (UART connector wired
 *                 directly to a miniuart3 / FTDI cable).
 *   REMOTE board: MCU TX on RC6, MCU RX on RC7 (FT231XQ-R on the PCB —
 *                 the FT231 RXD input lands on RC6 and TXD output on RC7,
 *                 i.e. the opposite of the robot board). */
#if MODEM_ROLE == MODEM_ROLE_ROBOT
#define PIN_TX_TRIS         TRISCbits.TRISC7
#define PIN_TX_ANSEL        ANSELCbits.ANSELC7
#define PIN_RX_TRIS         TRISCbits.TRISC6
#define PIN_RX_ANSEL        ANSELCbits.ANSELC6
#else /* MODEM_ROLE_REMOTE */
#define PIN_TX_TRIS         TRISCbits.TRISC6
#define PIN_TX_ANSEL        ANSELCbits.ANSELC6
#define PIN_RX_TRIS         TRISCbits.TRISC7
#define PIN_RX_ANSEL        ANSELCbits.ANSELC7
#endif

/* LED on RC2 */
#define PIN_LED_LAT         LATCbits.LATC2
#define PIN_LED_TRIS        TRISCbits.TRISC2
#define PIN_LED_ANSEL       ANSELCbits.ANSELC2

/* RTS# on RA0 (output to FT231) */
#define PIN_RTS_LAT         LATAbits.LATA0
#define PIN_RTS_TRIS        TRISAbits.TRISA0
#define PIN_RTS_ANSEL       ANSELAbits.ANSELA0

/* CTS# on RA1 (input from FT231 — currently ignored by the firmware) */
#define PIN_CTS_PORT        PORTAbits.RA1
#define PIN_CTS_TRIS        TRISAbits.TRISA1
#define PIN_CTS_ANSEL       ANSELAbits.ANSELA1

/* PPS encoding for inputs: bits 5:3 = port (0=A,1=B,2=C), bits 2:0 = pin. */
#define PPS_RB1             0x09u
#define PPS_RC4             0x14u
#define PPS_RC6             0x16u
#define PPS_RC7             0x17u

/* PPS output source codes — PIC18F26Q10-specific (datasheet DS40001996A
 * Table 17-2). Verify against the EXACT device datasheet — these codes
 * differ between PIC18 families (Q10 vs K42 vs etc).
 *   0x09 = TX1/CK1   (EUSART1)
 *   0x0F = SCK1/SCL1 (MSSP1 clock)
 *   0x10 = SDO1/SDA1 (MSSP1 data out)
 *   0x11 = SCK2/SCL2 (MSSP2 clock)  -- NOT MSSP1 ! */
#define PPS_OUT_SCK1        0x0Fu
#define PPS_OUT_SDO1        0x10u
#define PPS_OUT_TX1         0x09u

/* ===========================================================================
 *                                CC1120 GPIO
 *=========================================================================*/

void cc1120_cs_assert(void)        { PIN_CC_CS_LAT = 0; }
void cc1120_cs_deassert(void)      { PIN_CC_CS_LAT = 1; }
/* RESET_N is not wired to the PIC on this board — soft reset only via SRES. */
void cc1120_reset_assert(void)     { /* no-op */ }
void cc1120_reset_deassert(void)   { /* no-op */ }
uint8_t cc1120_gdo0_read(void)     { return (uint8_t)PIN_CC_GDO_PORT; }

/* ===========================================================================
 *                                  LED
 *=========================================================================*/

void led_on(void)     { PIN_LED_LAT = 1; }
void led_off(void)    { PIN_LED_LAT = 0; }
void led_toggle(void) { PIN_LED_LAT ^= 1; }

/* ===========================================================================
 *                                  Delay
 *=========================================================================*/

void Delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_ms(1);
    }
}

void Delay_us(uint16_t us)
{
    while (us--) {
        __delay_us(1);
    }
}

/* ===========================================================================
 *                              Interrupt control
 *=========================================================================*/

void global_int_enable(void)
{
    INTCONbits.PEIE = 1;        /* peripheral interrupts on */
    INTCONbits.GIE  = 1;        /* global enable (legacy IPEN=0 mode) */
}

void global_int_disable(void)
{
    INTCONbits.GIE  = 0;
}

uint8_t board_int_save_disable(void)
{
    uint8_t saved = (uint8_t)INTCONbits.GIE;
    INTCONbits.GIE = 0;
    return saved;
}

void board_int_restore(uint8_t saved_token)
{
    if (saved_token) {
        INTCONbits.GIE = 1;
    }
}

/* ===========================================================================
 *                                  EUSART
 *=========================================================================*/

void board_uart_hw_init(uint32_t baud)
{
    uint32_t spbrg;

    /* Pin direction + analog disable.
     * TX is set as input to allow EUSART module to take it via PPS. */
    PIN_TX_ANSEL = 0;
    PIN_RX_ANSEL = 0;
    PIN_TX_TRIS  = 0;       /* TX as output */
    PIN_RX_TRIS  = 1;       /* RX as input */

    /* PPS routing — depends on the PCB (see PIN_TX_* macros above).
     * NOTE: depending on MPLAB X / XC8 device-file revision the EUSART RX
     * input register may be named RXPPS, RX1PPS or RX1DTPPS — they all
     * point to the same hardware mux. Adjust if you get a "not declared"
     * error. */
#if MODEM_ROLE == MODEM_ROLE_ROBOT
    RC7PPS  = PPS_OUT_TX1;
    RX1PPS  = PPS_RC6;
#else /* MODEM_ROLE_REMOTE */
    RC6PPS  = PPS_OUT_TX1;
    RX1PPS  = PPS_RC7;
#endif

    /* 16-bit baud rate generator, high speed:
     *   SPBRG = (Fosc / (4 * baud)) - 1 */
    spbrg = (FOSC_HZ / (4UL * baud)) - 1UL;

    BAUD1CONbits.BRG16 = 1;
    TX1STAbits.BRGH    = 1;
    TX1STAbits.SYNC    = 0;
    SP1BRGH = (uint8_t)((spbrg >> 8) & 0xFFu);
    SP1BRGL = (uint8_t)(spbrg & 0xFFu);

    RC1STAbits.SPEN = 1;    /* serial port enable */
    RC1STAbits.CREN = 1;    /* enable continuous receive */
    TX1STAbits.TXEN = 1;    /* enable transmitter */

    /* RX interrupt: enable now. TX interrupt: enabled on demand by driver. */
    PIR3bits.RC1IF = 0;
    PIE3bits.RC1IE = 1;
    PIE3bits.TX1IE = 0;
    INTCONbits.PEIE = 1;
}

bool board_uart_tx_ready(void)
{
    return (PIR3bits.TX1IF != 0);
}

void board_uart_tx_byte_raw(uint8_t b)
{
    TX1REG = b;
}

void board_uart_tx_irq_enable(void)
{
    PIE3bits.TX1IE = 1;
}

void board_uart_tx_irq_disable(void)
{
    PIE3bits.TX1IE = 0;
}

/* ===========================================================================
 *                                  SPI (MSSP1)
 *=========================================================================*/

void board_spi_hw_init(void)
{
    /* Pin directions / analog off. */
    PIN_SCK_ANSEL = 0;
    PIN_SDI_ANSEL = 0;
    PIN_SDO_ANSEL = 0;
    PIN_SCK_TRIS  = 0;      /* SCK as output (master) */
    PIN_SDI_TRIS  = 1;      /* SDI as input */
    PIN_SDO_TRIS  = 0;      /* SDO as output */

    /* PPS: RC3 = SCK1 out, RC4 = SDI1 in, RC5 = SDO1 out.
     *
     * !!! Master mode REQUIRES SSP1CLKPPS to also point at the SCK pin !!!
     * Datasheet §27.2.1: "In Master mode the clock signal output to the SCK
     * pin is also the clock signal input to the peripheral. The pin selected
     * for output with the RxyPPS register must also be selected as the
     * peripheral input with the SSPxCLKPPS register."
     * Without this, the shift register internal latch doesn't fire and
     * MISO sampling is broken. */
    RC3PPS     = PPS_OUT_SCK1;
    RC5PPS     = PPS_OUT_SDO1;
    SSP1CLKPPS = 0x13u;             /* RC3 (port C, pin 3) as SCK input */
    SSP1DATPPS = PPS_RC4;

    /* Disable module to reconfigure. */
    SSP1CON1bits.SSPEN = 0;

    /* SPI mode 0 (CC1120 expects CPOL=0, CPHA=0):
     *   CKP = 0 (clock idle low)
     *   CKE = 1 (transmit on active-to-idle edge — mode 0 with CKP=0)
     *   SMP = 0 (sample MISO at middle of bit time = rising edge of SCK,
     *            the correct mode-0 sample point. SMP=1 races with slave.)
     * Master mode, Fosc/16 = 2 MHz.
     * !!! Why Fosc/16, not Fosc/4? CC1120 datasheet SWRU295E Table 1 says
     * SCK max = 4 MHz when XOSC has not yet stabilised. The first SPI
     * transactions after reset happen exactly during XOSC settling, so we
     * must stay <= 4 MHz on the slow side. Fosc/4 = 8 MHz exceeded this and
     * caused the first read after reset to return 0xFF reliably. */
    SSP1STAT = 0x40u;       /* SMP=0, CKE=1 */
    SSP1CON1 = 0x21u;       /* SSPEN=1, CKP=0, SSPM=0001 (Fosc/16 = 2 MHz) */
}

uint8_t board_spi_exchange(uint8_t b)
{
    volatile uint8_t dummy;
    /* Clear any stale BF and write. */
    dummy = SSP1BUF;
    (void)dummy;
    PIR3bits.SSP1IF = 0;
    SSP1BUF = b;
    while (!SSP1STATbits.BF) {
        /* spin */
    }
    return SSP1BUF;
}

/* ===========================================================================
 *                              Timer0 (1 ms tick)
 *=========================================================================*/

/* TMR0 reload for 1 ms with HFINTOSC source ~= 8 MHz, prescaler 1:1.
 * NOTE: empirically measured. The HFINTOSC tap that Timer0 sees is fixed
 * at ~8 MHz on PIC18F26Q10 regardless of the CPU OSCFRQ setting — when
 * we ran with prescaler 1:1024 we observed millis() advancing at ~250 Hz,
 * which corresponds to a ~7.86 MHz source. Using prescaler 1:1 gives 1 ms
 * resolution with reload = 65536 - 8000 = 57536 = 0xE0C0. */
#define TMR0_RELOAD_H   0xE0u
#define TMR0_RELOAD_L   0xC0u

void board_timer_hw_init(void)
{
    /* PMD acts as a master power-gate for peripherals; clear it for
     * good measure. */
    PMD1bits.TMR0MD = 0;
    PMD1bits.TMR1MD = 0;
    PMD1bits.TMR2MD = 0;

    /* === Calibrated Timer0 config for PIC18F26Q10 ===
     * Clock source: HFINTOSC (T0CS=010) — empirically ~8 MHz on this
     * chip's Timer0 tap. Other documented sources (Fosc/4 = T0CS=001)
     * were tried and gave a stuck counter, so we stick with what works.
     * Prescaler 1:1 -> timer ticks at ~8 MHz.
     * Reload 0xE0C0 (-8000) -> overflow every 1 ms (within ~2 %). */
    T0CON0 = 0x00u;                 /* disable while configuring */
    T0CON1 = 0x40u;                 /* T0CS=010 (HFINTOSC),
                                     * T0ASYNC=0 (sync),
                                     * T0CKPS=0000 (1:1) */
    TMR0H  = TMR0_RELOAD_H;
    TMR0L  = TMR0_RELOAD_L;
    PIR0bits.TMR0IF = 0;
    PIE0bits.TMR0IE = 1;
    T0CON0 = 0x90u;                 /* T0EN=1, T016BIT=1, OUTPS=0 */
}

/* ===========================================================================
 *                          INT0 (CC1120 GDO0 edge)
 *=========================================================================*/

void board_cc1120_int_enable(void)
{
    PIR0bits.INT0IF = 0;
    PIE0bits.INT0IE = 1;
}

void board_cc1120_int_disable(void)
{
    PIE0bits.INT0IE   = 0;
}

/* ===========================================================================
 *                                  board_init
 *=========================================================================*/

void board_init(void)
{
    /* --- Clock: switch to HFINTOSC at 32 MHz.
     *   OSCCON1: NOSC = 110 (HFINTOSC), NDIV = 0 (divide by 1)  -> 0x60
     *   OSCFRQ : HFFRQ = 0b0110 = 32 MHz                         -> 0x06
     * The HFINTOSC factory trim gives ~ ±1 % at room temperature, sufficient
     * for 115200 baud (~0.5 % bit-rate budget). If your prototype shows UART
     * framing errors over a wide temperature range, drop to 57600 in
     * config.h::UART_BAUD_DEFAULT. */
    OSCFRQ  = 0x06u;
    OSCCON1 = 0x60u;
    while (!OSCCON3bits.ORDY) {
        /* wait for the oscillator switch to complete */
    }

    /* --- All ANSEL off by default (digital I/O); per-peripheral inits will
     * re-clear ANSEL on the bits they own. */
    ANSELA = 0x00u;
    ANSELB = 0x00u;
    ANSELC = 0x00u;

    /* --- LED debug on RC2 */
    PIN_LED_ANSEL = 0;
    PIN_LED_TRIS  = 0;
    PIN_LED_LAT   = 0;

    /* --- RTS# / CTS# (FT231 hardware flow control on RA0/RA1).
     * RTS# = output, driven LOW = "PIC ready to receive". The 512-byte UART
     * RX ring is large enough that we never need to back-pressure for our
     * use case — keeping it asserted permanently is the simple, safe choice.
     * CTS# = input, currently ignored (FT231 USB host-side rarely throttles
     * us at 115200; if you observe TX stalls, gate board_uart_tx_byte_raw()
     * on PIN_CTS_PORT == 0). */
    PIN_RTS_ANSEL = 0;
    PIN_RTS_TRIS  = 0;
    PIN_RTS_LAT   = 0;          /* assert RTS# */
    PIN_CTS_ANSEL = 0;
    PIN_CTS_TRIS  = 1;

    /* --- CC1120 control GPIOs */
    PIN_CC_CS_ANSEL  = 0;
    PIN_CC_CS_TRIS   = 0;
    PIN_CC_CS_LAT    = 1;       /* idle high */
    PIN_CC_GDO_ANSEL = 0;
    PIN_CC_GDO_TRIS  = 1;       /* input */

    /* --- INT0 from RB1 (CC1120 GDO0).
     * IOCFG0 = PKT_SYNC_RXTX: GDO0 goes HIGH on sync detect (start of
     * packet) and LOW at end of packet. We want to fire ONCE per packet,
     * at the end, when the RX FIFO actually contains the bytes — that
     * way cc1120_process_events() reads num_rxbytes > 0 and correctly
     * classifies the event as RX_DONE. So configure INT0 for the FALLING
     * edge. The first version used rising edge, which fired at sync-
     * detect time when the FIFO was still empty -> packets were silently
     * mis-classified as TX_DONE and dropped. */
    INT0PPS  = PPS_RB1;
    INTCONbits.INT0EDG = 0;     /* 0 = falling edge */
    board_cc1120_int_enable();

    /* --- Other unused pins as inputs (TRIS=1) is the reset default. */
}

/* ===========================================================================
 *                                  ISR
 *
 * Single legacy ISR vector (IPEN=0). The PEIE bit is set globally; per-
 * peripheral enables are in PIEx and flags in PIRx.
 *=========================================================================*/

void __interrupt() ISR(void)
{
    /* --- Timer0 1 ms tick --- */
    if (PIE0bits.TMR0IE && PIR0bits.TMR0IF) {
        PIR0bits.TMR0IF = 0;
        TMR0H = TMR0_RELOAD_H;
        TMR0L = TMR0_RELOAD_L;          /* commits both bytes */
        isr_timer_tick_1ms();
    }

    /* --- INT0 / CC1120 GDO0 edge --- */
    if (PIE0bits.INT0IE && PIR0bits.INT0IF) {
        PIR0bits.INT0IF = 0;
        isr_cc1120_gdo();
    }

    /* --- EUSART RX --- */
    if (PIE3bits.RC1IE && PIR3bits.RC1IF) {
        uint8_t b;
        if (RC1STAbits.OERR) {
            /* Overrun: clear by toggling CREN. */
            RC1STAbits.CREN = 0;
            RC1STAbits.CREN = 1;
        } else if (RC1STAbits.FERR) {
            /* Framing error: discard the byte. */
            b = RC1REG;
            (void)b;
        } else {
            b = RC1REG;                 /* clears RC1IF */
            isr_uart_rx_byte(b);
        }
    }

    /* --- EUSART TX --- */
    if (PIE3bits.TX1IE && PIR3bits.TX1IF) {
        uint8_t b;
        if (isr_uart_tx_next(&b)) {
            TX1REG = b;
        } else {
            /* Nothing left to send: mask the interrupt until uart_tx_kick()
             * re-enables it. */
            PIE3bits.TX1IE = 0;
        }
    }
}
