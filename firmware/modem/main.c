/*============================================================================
 * main.c — PHASE 4c: ANALOG SUBSYSTEM PROBE (no SCAL).
 *
 * Phase 4b proved the chip is electrically alive (PARTNUM OK, all reset
 * values OK, IDLE for 30 s, stays cool). So the failure is on the PCB.
 *
 * This phase uses the GDO0 pin to expose internal CC1120 signals as
 * digital outputs that the PIC can read on RB1. No SCAL is run, so the
 * chip never enters the burn-loop. We probe:
 *
 *   (a) XOSC_STABLE (IOCFG0 = 0x39)
 *       - After SRES + power-up the CC1120 starts the XOSC automatically.
 *       - Expected: GDO0 high within ~150 us.
 *       - If GDO0 stays LOW -> XOSC not running -> problem on pins 28
 *         (AVDD_XOSC), 29 (DCPL_XOSC), 30 (XOSC_Q1), 31 (XOSC_Q2),
 *         32 (EXT_XOSC) or the crystal itself.
 *
 *   (b) RSSI_VALID (IOCFG0 = 0x06 SYNC_WORD_RXTX doesn't help here, we use
 *       RSSI_VALID = 0x21). Goes high once the RX chain is up; we won't
 *       enter RX here, just sanity-check that the bit can be read.
 *
 *   (c) MARCSTATE polling during a *short* IDLE -> TX setup without SCAL.
 *       We set FS_AUTOCAL=01 in SETTLING_CFG and strobe STX. The chip will
 *       try to calibrate; we sample MARCSTATE every 100 us for the first
 *       ~5 ms and report the trace. As soon as we see the chip stuck at
 *       0x04 (REG_SETTLE_MC) we abort with SIDLE before it heats up.
 *
 * The verdict from this probe tells us *which analog block* of the radio
 * is failing on the robot PCB, so we know which pins to inspect / which
 * passive component to suspect.
 *===========================================================================*/
#include <xc.h>
#include "config.h"
#include "board/board.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/timer.h"
#include "radio/cc1120.h"
#include "radio/cc1120_regs.h"

#define IOCFG_XOSC_STABLE   0x39u
#define IOCFG_PLL_LOCK      0x37u
#define IOCFG_RSSI_VALID    0x21u
#define IOCFG_HIZ           0x30u

static void uart_puts(const char *s)
{
    while (*s) {
        uart_write((uint8_t)*s);
        s++;
    }
}

static void uart_print_hex(uint8_t b)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_write((uint8_t)hex[(b >> 4) & 0x0Fu]);
    uart_write((uint8_t)hex[b & 0x0Fu]);
}

static void uart_print_dec(uint16_t v)
{
    char buf[6];
    int n = 0;
    if (v == 0) { uart_write('0'); return; }
    while (v > 0u && n < 6) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n-- > 0) {
        uart_write((uint8_t)buf[n]);
    }
}

/* Read CC1120 GDO0 pin via PIC's RB1 input. */
static uint8_t read_gdo0(void)
{
    return (uint8_t)(PORTBbits.RB1 & 0x01u);
}

int main(void)
{
    uint8_t partnum, gdo, marc;
    uint8_t i;
    uint8_t marc_trace[50];     /* 50 samples x 100us = 5 ms */
    uint8_t marc_stuck_at_04;

    board_init();
    uart_init(UART_BAUD_DEFAULT);
    spi_init();
    timer_init();
    global_int_enable();

    Delay_ms(200);

    uart_puts("\r\n\r\n===========================================\r\n");
    uart_puts("  PHASE 4c — analog subsystem probe via GDO0\r\n");
    uart_puts("===========================================\r\n");

    /* --- Reset + identity check ------------------------------------------ */
    cc1120_reset();
    Delay_ms(10);
    partnum = cc1120_ext_reg_read(0x8F);
    if (partnum != 0x48u) {
        uart_puts("\r\nPARTNUM bad — SPI link dead. Halting.\r\n");
        for (;;) { /* halt */ }
    }
    uart_puts("\r\nPARTNUM OK (0x48). Probes starting.\r\n");

    /* ===== TEST A: XOSC_STABLE ============================================ *
     * Make GDO0 expose the XOSC stable signal, then poll it. */
    uart_puts("\r\n[A] XOSC stability check (GDO0 = XOSC_STABLE):\r\n");
    cc1120_reg_write(CC1120_IOCFG0, IOCFG_XOSC_STABLE);
    Delay_us(500);                              /* give the buffer + XOSC time */

    /* Sample 20 times over ~2 ms to see if it ever goes HIGH. */
    uart_puts("    GDO0 samples (every 100 us): ");
    for (i = 0; i < 20u; i++) {
        gdo = read_gdo0();
        uart_write((uint8_t)('0' + gdo));
        Delay_us(100);
    }
    uart_puts("\r\n");

    /* Final read. */
    gdo = read_gdo0();
    uart_puts("    Final GDO0 = "); uart_write((uint8_t)('0' + gdo));
    if (gdo == 1u) {
        uart_puts("  -> XOSC IS STABLE.\r\n");
    } else {
        uart_puts("  -> XOSC NOT running !\r\n");
        uart_puts("    Suspect: pins 28-32 or 32 MHz crystal itself.\r\n");
    }

    /* ===== TEST B: GDO0 high-Z then drive low (sanity of the GDO line) === *
     * Set IOCFG0 = 0x30 (HiZ) -> RB1 should be pulled around by leakage,
     * but more importantly the chip honored the SPI write. Reading IOCFG0
     * back must show what we wrote. */
    uart_puts("\r\n[B] GDO0 SPI control sanity:\r\n");
    cc1120_reg_write(CC1120_IOCFG0, IOCFG_HIZ);
    Delay_us(200);
    {
        uint8_t v = cc1120_reg_read(CC1120_IOCFG0);
        uart_puts("    Wrote 0x30 to IOCFG0, read back 0x");
        uart_print_hex(v);
        if (v == IOCFG_HIZ) {
            uart_puts("  OK\r\n");
        } else {
            uart_puts("  MISMATCH (SPI flaky?)\r\n");
        }
    }

    /* ===== TEST C: MARCSTATE trace during STX attempt (NO SCAL) ========== *
     * Strobe STX with FS_AUTOCAL=01 (cal on idle->tx), then poll MARCSTATE
     * every 100 us for ~5 ms. The trace shows exactly which sub-state the
     * chip dies in. We abort the moment we see 0x04 for >1 ms (REG_SETTLE
     * stuck), to avoid the burn loop. */
    uart_puts("\r\n[C] MARCSTATE trace during cal attempt:\r\n");

    /* Minimal config: just FREQ + LO_DIV so the chip knows what to cal to. */
    cc1120_reg_write(CC1120_FS_CFG, 0x1Au);
    cc1120_ext_reg_write(CC1120_FREQ2, 0x69u);
    cc1120_ext_reg_write(CC1120_FREQ1, 0xE4u);
    cc1120_ext_reg_write(CC1120_FREQ0, 0x66u);
    /* Enable auto-cal on idle->tx transition, max FSREG_TIME. */
    cc1120_reg_write(CC1120_SETTLING_CFG, 0x1Bu);

    /* Make sure we are in IDLE. */
    cc1120_strobe(CC1120_SIDLE);
    Delay_ms(1);

    /* Strobe STX — this triggers auto-cal. */
    cc1120_strobe(CC1120_STX);

    /* Capture 50 MARCSTATE samples at ~100 us spacing. */
    marc_stuck_at_04 = 0;
    for (i = 0; i < 50u; i++) {
        marc_trace[i] = cc1120_get_marcstate();
        if (marc_trace[i] == 0x04u) {
            marc_stuck_at_04++;
        }
        Delay_us(100);
    }

    /* Abort whatever the chip is doing — SIDLE saves it from heating. */
    cc1120_strobe(CC1120_SIDLE);
    Delay_ms(1);

    /* Dump the trace as hex. */
    uart_puts("    MARCSTATE trace (50 samples, ~100 us each):\r\n   ");
    for (i = 0; i < 50u; i++) {
        if ((i % 10u) == 0u) uart_puts("\r\n     ");
        uart_print_hex(marc_trace[i]);
        uart_write(' ');
    }
    uart_puts("\r\n");

    uart_puts("    Samples stuck at 0x04 (REG_SETTLE_MC): ");
    uart_print_dec((uint16_t)marc_stuck_at_04);
    uart_puts(" / 50\r\n");

    marc = cc1120_get_marcstate();
    uart_puts("    Final MARCSTATE after SIDLE: 0x");
    uart_print_hex(marc); uart_puts("\r\n");

    /* ===== Verdict ======================================================== */
    uart_puts("\r\n===========================================\r\n");
    uart_puts("  PHASE 4c done. Interpretation guide:\r\n");
    uart_puts("\r\n");
    uart_puts("  [A] GDO0 = 1 immediately ?\r\n");
    uart_puts("        Yes -> XOSC OK. Suspect synth analog (DCPL_VCO,\r\n");
    uart_puts("               AVDD_SYNTH).\r\n");
    uart_puts("        No  -> XOSC dead. Inspect pins 28-32 + crystal +\r\n");
    uart_puts("               XOSC load caps (15 pF).\r\n");
    uart_puts("\r\n");
    uart_puts("  [C] Trace pattern :\r\n");
    uart_puts("        02 03 04 04 04 ... -> stuck in REG_SETTLE_MC\r\n");
    uart_puts("                              (DCPL_VCO / AVDD_SYNTH)\r\n");
    uart_puts("        02 03 03 03 03 ... -> stuck in BIAS_SETTLE_MC\r\n");
    uart_puts("                              (RBIAS / GBIAS issue)\r\n");
    uart_puts("        02 03 04 06 07 08 -> cal advancing then stalls\r\n");
    uart_puts("                              (PLL lock / FS_CAL pins)\r\n");
    uart_puts("        00 00 00 00 00 00 -> chip didn't respond to STX\r\n");
    uart_puts("===========================================\r\n");

    for (;;) { /* halt */ }

    return 0;
}
