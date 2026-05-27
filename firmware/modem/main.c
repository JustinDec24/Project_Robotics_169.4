/*============================================================================
 * main.c — PHASE 4b: try RX WITHOUT auto-cal AND WITHOUT manual SCAL.
 *
 * Phase 4 showed: with auto-cal enabled, entering RX locks the chip
 * (SNOP=0xFF, MARC=0x1F), suggesting calibration is failing.
 *
 * Phase 4b disables auto-cal entirely (SETTLING_CFG[5:4] = 00) and also
 * does NOT issue manual SCAL. Then strobes SRX. The chip will enter RX
 * but with uncalibrated synth — it likely won't receive well, but the
 * idea is to see whether it gets stuck or not.
 *
 *   chip RESPONDS to SPI in RX -> calibration was the cause, chip is fine
 *   chip stuck (SNOP=0xFF)      -> entering RX itself is the issue
 *===========================================================================*/
#include "config.h"
#include "board/board.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/timer.h"
#include "radio/cc1120.h"
#include "radio/cc1120_regs.h"

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

int main(void)
{
    bool init_ok;
    uint8_t status, marc;
    int t;

    board_init();
    uart_init(UART_BAUD_DEFAULT);
    spi_init();
    timer_init();
    global_int_enable();

    Delay_ms(100);

    uart_puts("\r\n\r\n===========================================\r\n");
    uart_puts("  PHASE 4b — RX without auto-cal, without SCAL\r\n");
    uart_puts("===========================================\r\n");

    /* ---- 1. Full init (registers + flush, NO SCAL — auto-cal enabled) ----*/
    uart_puts("\r\n[1] cc1120_init_minimal()...");
    init_ok = cc1120_init_minimal();
    if (!init_ok) {
        uart_puts(" FAILED — halting.\r\n");
        for (;;) { /* halt */ }
    }
    uart_puts(" OK\r\n");

    /* ---- 2. Check state in IDLE -----------------------------------------*/
    cc1120_set_idle();
    status = cc1120_strobe(CC1120_SNOP);
    marc = cc1120_get_marcstate();
    uart_puts("\r\n[2] Before SRX (IDLE expected):\r\n");
    uart_puts("    SNOP: 0x"); uart_print_hex(status); uart_puts("\r\n");
    uart_puts("    MARC: 0x"); uart_print_hex(marc); uart_puts("\r\n");

    /* ---- 3. SRX strobe — auto-cal kicks in -------------------------------*/
    uart_puts("\r\n[3] SRX strobe (auto-cal on transition)...\r\n");
    cc1120_strobe(CC1120_SRX);

    /* ---- 5. Poll several times --------------------------------------------*/
    for (t = 0; t < 5; t++) {
        Delay_ms(20);
        status = cc1120_strobe(CC1120_SNOP);
        marc = cc1120_get_marcstate();
        uart_puts("    t=");
        uart_write((uint8_t)('0' + t));
        uart_puts(": SNOP=0x"); uart_print_hex(status);
        uart_puts(" MARC=0x"); uart_print_hex(marc); uart_puts("\r\n");
    }

    /* ---- 6. Back to IDLE --------------------------------------------------*/
    uart_puts("\r\n[5] Force SIDLE...\r\n");
    cc1120_strobe(CC1120_SIDLE);
    Delay_ms(10);
    status = cc1120_strobe(CC1120_SNOP);
    marc = cc1120_get_marcstate();
    uart_puts("    After SIDLE: SNOP=0x"); uart_print_hex(status);
    uart_puts(" MARC=0x"); uart_print_hex(marc); uart_puts("\r\n");

    uart_puts("\r\n===========================================\r\n");
    uart_puts("  PHASE 4b done.\r\n");
    uart_puts("  Touch the chip. Did it stay cool?\r\n");
    uart_puts("===========================================\r\n");

    for (;;) { /* halt */ }

    return 0;
}
