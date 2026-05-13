/*============================================================================
 * cc1120_test/main.c — Datasheet-strict CC1120 connection test
 *
 * Built from a deep read of:
 *   - TI CC1120 datasheet  (SWRS112H)
 *   - TI CC112x user guide (SWRU295E)
 *
 * Goal: read PARTNUMBER (= 0x48) from a freshly-reset CC1120 over SPI,
 * obeying EVERY timing constraint in the datasheet.
 *
 * Key datasheet rules implemented here:
 *
 *   [SWRU295E §3.1.2 Table 1]
 *     fSCLK reg R/W       : max 8 MHz   (@fXOSC=32 MHz)
 *     fSCLK ext-mem read  : max 6 MHz   (@fXOSC=32 MHz)
 *     fSCLK during XOSC startup: must be slow until SO drops
 *     tsp (CSn↓ to SCLK↑) : >= 50 ns
 *     tns (SCLK↓ to CSn↑) : >= 200 ns
 *     CSn high between transactions : >= 50 ns
 *   We use SCLK = Fosc/16 = 2 MHz, well under every cap.
 *
 *   [SWRU295E §9.1.1, Figure 23]
 *     After power-on or SRES, pull CSn low and **wait for SO to go LOW**
 *     before issuing any other SPI byte. SO=1 = CHIP_RDYn=1 = not ready.
 *     SO=0 = CHIP_RDYn=0 = ready.
 *
 *   [SWRU295E §3.2.2, Figure 5]
 *     SRES strobe: after sending 0x30, KEEP CSn LOW and wait for SO to go
 *     LOW again before next header byte.
 *
 *   [SWRU295E §3.2 Table 3]
 *     Standard register read = 2 bytes: { READ|addr, dummy }, data on MISO[1].
 *     Extended register read = 3 bytes: { 0xAF, ext_addr, dummy }, data on
 *     MISO[2]. MISO[1] is forced to 0x00 by the chip during ext addr byte.
 *
 *   [SWRU295E §3.1.2 Table 2]
 *     Status byte format: { CHIP_RDYn, STATE[2:0], reserved[3:0] }
 *     STATE: 000=IDLE, 001=RX, 010=TX, 011=FSTXON, 100=CALIBRATE,
 *            101=SETTLING, 110=RX_FIFO_ERROR, 111=TX_FIFO_ERROR.
 *
 *   Reset values used to verify the chip is alive:
 *     IOCFG3 (std 0x00) = 0x06
 *     SYNC0  (std 0x07) = 0xDE
 *     PARTNUMBER (ext 0x8F, ROM) = 0x48 for CC1120
 *     MARCSTATE  (ext 0x73) = 0x41 (settled IDLE)
 *
 * Pin mapping (PIC18F26Q10-I/SS SSOP-28):
 *     RC3 (pin 14) -> SCLK (CC1120 pin 8)
 *     RC4 (pin 15) -> SO   (CC1120 pin 9)        [MISO into PIC]
 *     RC5 (pin 16) -> SI   (CC1120 pin 7)        [MOSI out of PIC]
 *     RB0 (pin 21) -> CSn  (CC1120 pin 11)
 *     RC7 (pin 18) -> UART TX to FT231 (debug)
 *===========================================================================*/
#include <xc.h>
#include <stdint.h>

/* === Config bits ==========================================================*/
#pragma config FEXTOSC  = OFF
#pragma config RSTOSC   = HFINTOSC_1MHZ
#pragma config CLKOUTEN = OFF
#pragma config CSWEN    = ON
#pragma config FCMEN    = OFF
#pragma config MCLRE    = EXTMCLR
#pragma config PWRTE    = ON
#pragma config LPBOREN  = OFF
#pragma config BOREN    = SBORDIS
#pragma config BORV     = VBOR_245
#pragma config ZCD      = OFF
#pragma config PPS1WAY  = OFF
#pragma config STVREN   = ON
#pragma config XINST    = OFF
#pragma config WDTCPS   = WDTCPS_31
#pragma config WDTE     = OFF
#pragma config WDTCWS   = WDTCWS_7
#pragma config WDTCCS   = SC
#pragma config WRT0     = OFF
#pragma config WRT1     = OFF
#pragma config WRT2     = OFF
#pragma config WRT3     = OFF
#pragma config WRTC     = OFF
#pragma config WRTB     = OFF
#pragma config WRTD     = OFF
#pragma config SCANE    = ON
#pragma config LVP      = ON
#pragma config CP       = OFF
#pragma config CPD      = OFF
#pragma config EBTR0    = OFF
#pragma config EBTR1    = OFF
#pragma config EBTR2    = OFF
#pragma config EBTR3    = OFF
#pragma config EBTRB    = OFF

#define _XTAL_FREQ 32000000UL

/* === CC1120 opcodes (SWRU295E §3.2) =======================================*/
#define R_FLAG          0x80u   /* OR with header for READ */
#define B_FLAG          0x40u   /* OR with header for BURST */
#define EXT_HDR         0x2Fu   /* extension command, send as header */

#define SRES            0x30u
#define SNOP            0x3Du
#define SIDLE           0x36u

/* === GPIO ================================================================*/
#define CS_LOW()    do { LATBbits.LATB0 = 0; } while (0)
#define CS_HIGH()   do { LATBbits.LATB0 = 1; } while (0)
#define MISO_PIN()  (PORTCbits.RC4)

/* === Clock ================================================================*/
static void clock_init(void)
{
    OSCFRQ  = 0x06u;        /* HFFRQ = 32 MHz */
    OSCCON1 = 0x60u;        /* NOSC = HFINTOSC, NDIV = 1 */
    while (!OSCCON3bits.ORDY) { /* spin */ }
}

/* === UART (115200 8N1 on RC7) ============================================*/
static void uart_init(void)
{
    ANSELCbits.ANSELC6 = 0;
    ANSELCbits.ANSELC7 = 0;
    TRISCbits.TRISC6   = 1;
    TRISCbits.TRISC7   = 0;
    RC7PPS = 0x09u;         /* TX1 out */
    RX1PPS = 0x16u;         /* RX1 in = RC6 */

    BAUD1CONbits.BRG16 = 1;
    TX1STAbits.BRGH    = 1;
    TX1STAbits.SYNC    = 0;
    SP1BRGH = 0x00u;
    SP1BRGL = 68u;          /* 32 MHz / (4*69) = ~115942 baud */
    RC1STAbits.SPEN = 1;
    TX1STAbits.TXEN = 1;
}

static void uart_putc(uint8_t c)
{
    while (!PIR3bits.TX1IF) { /* spin */ }
    TX1REG = c;
}

static void uart_puts(const char *s) { while (*s) uart_putc((uint8_t)*s++); }

static void uart_print_hex(uint8_t b)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_putc((uint8_t)hex[(b >> 4) & 0x0Fu]);
    uart_putc((uint8_t)hex[b & 0x0Fu]);
}

/* === SPI master (MSSP1, Fosc/16 = 2 MHz, mode 0) ==========================
 *
 * Mode 0 on PIC18 MSSP:
 *   CKP = 0   (idle low)
 *   CKE = 1   (data shifted on falling edge of SCK — correct mode 0 master)
 *   SMP = 0   (sample MISO at MIDDLE of bit time = rising edge = correct
 *              mode 0 sample point. SMP=1 is WRONG here, races with slave.)
 *   SSPM= 0001 (Fosc/16 = 2 MHz)
 *==========================================================================*/
static void spi_init(void)
{
    /* CS GPIO (RB0) */
    ANSELBbits.ANSELB0 = 0;
    TRISBbits.TRISB0   = 0;
    LATBbits.LATB0     = 1;     /* CS idle HIGH */

    /* SPI pins */
    ANSELCbits.ANSELC3 = 0;     /* SCK   */
    ANSELCbits.ANSELC4 = 0;     /* SDI   - digital input buffer must be enabled */
    ANSELCbits.ANSELC5 = 0;     /* SDO   */
    TRISCbits.TRISC3   = 0;     /* SCK output */
    TRISCbits.TRISC4   = 1;     /* SDI input  */
    TRISCbits.TRISC5   = 0;     /* SDO output */

    /* PPS — verified against PIC18F26Q10 datasheet DS40001996A Table 17-2.
     * Output codes (Q10-specific, DO NOT copy from other PIC18 families):
     *   0x09 = TX1     0x0F = SCK1     0x10 = SDO1     0x11 = SCK2
     * Input encoding: bits[6:4]=port (A=0,B=1,C=2), bits[3:0]=pin
     *   RC3 -> 0x13    RC4 -> 0x14    RC6 -> 0x16
     *
     * !!! Master mode REQUIRES SSP1CLKPPS to point at the SCK pin too !!!
     * Quote from datasheet §27.2.1: "In Master mode the clock signal output
     * to the SCK pin is also the clock signal input to the peripheral. The
     * pin selected for output with the RxyPPS register must also be selected
     * as the peripheral input with the SSPxCLKPPS register."
     * Without this, the shift register internal latch never fires properly. */
    RC3PPS     = 0x0Fu;         /* RC3 outputs SCK1 */
    RC5PPS     = 0x10u;         /* RC5 outputs SDO1 (MOSI) */
    SSP1CLKPPS = 0x13u;         /* MSSP1 SCK input = RC3 (master feedback) */
    SSP1DATPPS = 0x14u;         /* MSSP1 SDI input = RC4 (MISO) */

    /* MSSP1 mode 0 master @ Fosc/16 */
    SSP1CON1bits.SSPEN = 0;
    SSP1STAT = 0x40u;           /* SMP=0, CKE=1 */
    SSP1CON1 = 0x21u;           /* SSPEN=1, CKP=0, SSPM=0001 */
}

static uint8_t spi_xfer(uint8_t tx)
{
    volatile uint8_t dummy;
    /* Make sure BF is clear (read absorbs any stale received byte). */
    dummy = SSP1BUF;
    (void)dummy;
    PIR3bits.SSP1IF = 0;

    /* Detect WCOL from a previous failed write. */
    if (SSP1CON1bits.WCOL) {
        SSP1CON1bits.WCOL = 0;
    }

    SSP1BUF = tx;
    while (!SSP1STATbits.BF) { /* wait for byte exchange to complete */ }
    return SSP1BUF;
}

/* === MISO direct-read helper ===
 * RC4 is muxed to SSP1 via PPS for SPI sampling AND remains readable as a
 * regular GPIO via PORTCbits.RC4 (input buffer not disabled by PPS).
 * Used to wait for CHIP_RDYn to drop during XOSC start-up and after SRES.
 */
static uint8_t wait_so_low(uint16_t max_us)
{
    while (max_us != 0u) {
        if (MISO_PIN() == 0u) return 1u;
        __delay_us(10);
        max_us = (max_us > 10u) ? (uint16_t)(max_us - 10u) : 0u;
    }
    return 0u;
}

/* === CC1120 SPI primitives (datasheet-strict) =============================*/

/* Strobe: 1 byte command, returns status. */
static uint8_t cc_strobe(uint8_t cmd)
{
    uint8_t status;
    CS_LOW();
    /* Wait for CHIP_RDYn=0 before clocking SCLK (datasheet §9.1.1). */
    (void)wait_so_low(2000u);   /* 2 ms is plenty after init */
    status = spi_xfer(cmd);
    /* tns (SCLK↓ to CSn↑) >= 200 ns; we use 1 us. */
    __delay_us(1);
    CS_HIGH();
    /* CSn high between transactions >= 50 ns; we use 1 us. */
    __delay_us(1);
    return status;
}

/* Standard register read (addr in 0x00..0x2E). */
static uint8_t cc_read_reg(uint8_t addr)
{
    uint8_t data;
    CS_LOW();
    (void)wait_so_low(2000u);
    (void)spi_xfer((uint8_t)(R_FLAG | addr));   /* byte 1: cmd, returns status */
    data = spi_xfer(0x00u);                      /* byte 2: dummy, returns data */
    __delay_us(1);
    CS_HIGH();
    __delay_us(1);
    return data;
}

/* Standard register write (addr in 0x00..0x2E). */
static void cc_write_reg(uint8_t addr, uint8_t value)
{
    CS_LOW();
    (void)wait_so_low(2000u);
    (void)spi_xfer(addr);
    (void)spi_xfer(value);
    __delay_us(1);
    CS_HIGH();
    __delay_us(1);
}

/* Extended register read (ext addr in 0x00..0xFF). */
static uint8_t cc_read_ext(uint8_t ext_addr)
{
    uint8_t data;
    CS_LOW();
    (void)wait_so_low(2000u);
    (void)spi_xfer((uint8_t)(R_FLAG | EXT_HDR)); /* byte 1: 0xAF, returns status */
    (void)spi_xfer(ext_addr);                    /* byte 2: addr, returns 0x00 */
    data = spi_xfer(0x00u);                      /* byte 3: dummy, returns data */
    __delay_us(1);
    CS_HIGH();
    __delay_us(1);
    return data;
}

/* === CC1120 manual reset (SWRU295E §3.2.2 Figure 5) =======================*/
static uint8_t cc_manual_reset(void)
{
    uint8_t so_ok_before, so_ok_after;

    /* Toggle CS to sync the SPI state machine in the chip. */
    CS_HIGH(); __delay_us(50);
    CS_LOW();  __delay_us(50);
    CS_HIGH(); __delay_us(50);

    /* CSn low and wait for SO low (chip ready). */
    CS_LOW();
    __delay_us(50);                  /* tsp setup */
    so_ok_before = wait_so_low(5000u);

    /* Send SRES strobe. */
    (void)spi_xfer(SRES);

    /* CRITICAL: KEEP CS LOW after SRES until SO goes low again. */
    so_ok_after = wait_so_low(10000u);

    __delay_us(1);
    CS_HIGH();
    __delay_us(5);

    return (uint8_t)((so_ok_before & 1u) | ((so_ok_after & 1u) << 1u));
}

/* === Pretty print helpers =================================================*/
static void print_named(const char *label, uint8_t v, const char *note)
{
    uart_puts(label);
    uart_puts(": 0x");
    uart_print_hex(v);
    if (note) { uart_puts("  ("); uart_puts(note); uart_puts(")"); }
    uart_puts("\r\n");
}

/* ===========================================================================
 *                              main
 *===========================================================================*/
void main(void)
{
    uint8_t rst, status, i;
    uint8_t partnum, partver, marc, iocfg3, sync0;

    clock_init();
    uart_init();
    spi_init();
    __delay_ms(100);    /* let everything settle */

    uart_puts("\r\n\r\n===== CC1120 DATASHEET-STRICT TEST =====\r\n");
    uart_puts("SPI: mode 0, SMP=0, CKE=1, CKP=0, Fosc/16=2 MHz\r\n");
    uart_puts("Pins: SCK=RC3(14)  SO=RC4(15)  SI=RC5(16)  CSn=RB0(21)\r\n");
    uart_puts("==========================================\r\n\r\n");

    /* === [A] Sanity check the MISO pin physical wiring =====================
     * If MISO is properly wired, then:
     *   CSn HIGH -> SO is high-Z, reading depends on pull/parasitics
     *   CSn LOW & chip not ready -> SO actively HIGH
     *   CSn LOW & chip ready     -> SO actively LOW
     * If MISO is OPEN (not wired), all of the above readings are random and
     * driven by capacitive coupling from nearby traces. */
    uart_puts("[A] MISO physical wiring check\r\n");
    CS_HIGH();
    __delay_ms(2);
    uart_puts("    MISO with CSn HIGH (idle, high-Z expected): ");
    uart_putc((uint8_t)('0' + MISO_PIN()));
    uart_puts("\r\n");
    CS_LOW();
    __delay_us(50);
    uart_puts("    MISO immediately after CSn LOW: ");
    uart_putc((uint8_t)('0' + MISO_PIN()));
    uart_puts("\r\n");
    if (wait_so_low(10000u)) {
        uart_puts("    MISO went LOW within 10 ms -> chip signalled READY\r\n");
    } else {
        uart_puts("    MISO stayed HIGH for 10 ms -> chip NOT ready OR not wired\r\n");
    }
    CS_HIGH();

    /* === [B] Manual reset per datasheet ====================================*/
    uart_puts("\r\n[B] Manual reset (datasheet-strict)\r\n");
    rst = cc_manual_reset();
    uart_puts("    SO low BEFORE SRES: ");
    uart_puts((rst & 1u) ? "YES" : "NO");
    uart_puts("\r\n    SO low AFTER  SRES: ");
    uart_puts((rst & 2u) ? "YES" : "NO");
    uart_puts("\r\n");

    /* === [C] SNOP strobes — status byte readout ============================
     * Status byte = CHIP_RDYn|STATE[2:0]|reserved[3:0]
     * After SRES, expect 0x00 (chip ready + IDLE state). */
    uart_puts("\r\n[C] SNOP strobes (expect 0x00 = ready+IDLE)\r\n");
    for (i = 0; i < 3u; i++) {
        status = cc_strobe(SNOP);
        uart_puts("    SNOP[");
        uart_putc((uint8_t)('0' + i));
        uart_puts("]: 0x");
        uart_print_hex(status);
        uart_puts("\r\n");
        __delay_ms(2);
    }

    /* === [D] PARTNUMBER read (ext 0x8F) ====================================*/
    uart_puts("\r\n[D] PARTNUMBER (ext 0x8F, expect 0x48 = CC1120)\r\n");
    for (i = 0; i < 3u; i++) {
        partnum = cc_read_ext(0x8Fu);
        uart_puts("    PARTNUMBER[");
        uart_putc((uint8_t)('0' + i));
        uart_puts("]: 0x");
        uart_print_hex(partnum);
        if (partnum == 0x48u)      uart_puts("  <-- CC1120 OK !");
        else if (partnum == 0x58u) uart_puts("  <-- CC1125 OK !");
        else if (partnum == 0x5Au) uart_puts("  <-- CC1175 OK !");
        else if (partnum == 0x40u) uart_puts("  <-- CC1121 OK !");
        else if (partnum == 0xFFu) uart_puts("  (MISO floating / open ?)");
        else if (partnum == 0x00u) uart_puts("  (MISO pulled low ?)");
        else                       uart_puts("  (unexpected)");
        uart_puts("\r\n");
        __delay_ms(2);
    }

    /* === [E] PARTVERSION + MARCSTATE =======================================*/
    partver = cc_read_ext(0x90u);
    print_named("\r\n[E] PARTVERSION (ext 0x90)\r\n    PARTVERSION", partver,
                "factory revision (non-zero on real chip)");
    marc = cc_read_ext(0x73u);
    print_named("[F] MARCSTATE (ext 0x73)\r\n    MARCSTATE", marc,
                "expect 0x41 (settled IDLE)");

    /* === [G] Standard register reads with non-zero reset values ============*/
    iocfg3 = cc_read_reg(0x00u);
    print_named("\r\n[G] IOCFG3 (std 0x00, reset 0x06)\r\n    IOCFG3", iocfg3,
                "expect 0x06");
    sync0 = cc_read_reg(0x07u);
    print_named("    SYNC0  (std 0x07, reset 0xDE)\r\n    SYNC0", sync0,
                "expect 0xDE");

    /* === [H] Write/read test on IOCFG3 =====================================*/
    uart_puts("\r\n[H] Write 0xAA to IOCFG3, read back\r\n");
    cc_write_reg(0x00u, 0xAAu);
    iocfg3 = cc_read_reg(0x00u);
    print_named("    IOCFG3 readback", iocfg3, "expect 0xAA");
    cc_write_reg(0x00u, 0x06u);                  /* restore reset value */

    /* === [I] State transition test: SIDLE -> SNOP =========================*/
    uart_puts("\r\n[I] SIDLE strobe then SNOP (chip should stay in IDLE)\r\n");
    (void)cc_strobe(SIDLE);
    __delay_ms(2);
    status = cc_strobe(SNOP);
    print_named("    SNOP after SIDLE", status, "expect 0x00");

    uart_puts("\r\n===== TEST COMPLETE =====\r\n");

    for (;;) { /* idle */ }
}
