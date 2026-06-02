/*============================================================================
 * radio/cc1120.c — CC1120 low-level SPI command implementation + init.
 *
 * Frequency math (datasheet section 9.12):
 *   f_VCO = (FREQ / 2^16) * f_xosc
 *   f_RF  = f_VCO / LO_DIVIDER
 *
 *   For 169.4 MHz, f_xosc = 32 MHz, LO_DIVIDER = 20 (band 4 in FS_CFG),
 *   FREQ = 169.4e6 * 65536 * 20 / 32e6 = 0x69E000.
 *   So FREQ2/1/0 = 0x69 / 0xE0 / 0x00.
 *
 * The PHY register block below targets:
 *   - 2-GFSK, 4.8 kbps symbol rate
 *   - 5 kHz frequency deviation
 *   - 12.5 kHz RX channel bandwidth
 *   - Variable-length packets, hardware CRC, append RSSI/LQI status
 *   - GDO0 = PKT_SYNC_RXTX (rises on sync detect, falls at end of packet)
 *
 * These values are derived from the SmartRF Studio default profile for
 * 169 MHz / narrowband / 2-GFSK and are conservative enough to bring up
 * the link. For final tuning (data rate, AGC, PA), regenerate via
 * SmartRF Studio for your antenna and regulatory constraints, then paste
 * the new values into the table below.
 *===========================================================================*/
#include <xc.h>     /* needed for PORTCbits direct read (CHIP_RDY detect) */
#include "cc1120.h"
#include "cc1120_regs.h"
#include "../board/board.h"
#include "../drivers/spi.h"
#include "../config.h"

static volatile uint8_t radio_event_flags_ = 0;
static volatile bool    gdo_irq_pending_   = false;

/* ===== Internal SPI helpers ===============================================*/

static uint8_t spi_send_byte(uint8_t tx)
{
    return spi_transfer_byte(tx);
}

/* ===== Wait-for-SO-low helper (forward decl) =============================*/

/* Defined later in the file. Polls PORTCbits.RC4 until the CC1120 drops SO
 * (CHIP_RDYn = 0). Must be called after cc1120_cs_assert() and before any
 * SCLK edge, per SWRU295E §9.1.1. */
static uint8_t wait_so_low_us(uint16_t timeout_us);

/* ===== Command strobes ====================================================*/

uint8_t cc1120_strobe(uint8_t strobe_addr)
{
    uint8_t status;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);     /* wait for CHIP_RDY before clocking */
    status = spi_send_byte(strobe_addr);
    cc1120_cs_deassert();
    return status;
}

uint8_t cc1120_strobe_stx(void)   { return cc1120_strobe(CC1120_STX); }
uint8_t cc1120_strobe_srx(void)   { return cc1120_strobe(CC1120_SRX); }
uint8_t cc1120_strobe_sidle(void) { return cc1120_strobe(CC1120_SIDLE); }
uint8_t cc1120_strobe_sfrx(void)  { return cc1120_strobe(CC1120_SFRX); }
uint8_t cc1120_strobe_sftx(void)  { return cc1120_strobe(CC1120_SFTX); }
uint8_t cc1120_strobe_sres(void)  { return cc1120_strobe(CC1120_SRES); }
uint8_t cc1120_strobe_snop(void)  { return cc1120_strobe(CC1120_SNOP); }

/* ===== Standard register read/write =======================================*/

uint8_t cc1120_reg_read(uint8_t addr)
{
    uint8_t val;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_READ | addr);
    val = spi_send_byte(0x00);
    cc1120_cs_deassert();
    return val;
}

void cc1120_reg_write(uint8_t addr, uint8_t val)
{
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_WRITE | addr);
    spi_send_byte(val);
    cc1120_cs_deassert();
}

void cc1120_reg_burst_read(uint8_t addr, uint8_t *buf, size_t len)
{
    size_t i;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_READ | CC1120_BURST | addr);
    for (i = 0; i < len; i++) {
        buf[i] = spi_send_byte(0x00);
    }
    cc1120_cs_deassert();
}

void cc1120_reg_burst_write(uint8_t addr, const uint8_t *buf, size_t len)
{
    size_t i;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_WRITE | CC1120_BURST | addr);
    for (i = 0; i < len; i++) {
        spi_send_byte(buf[i]);
    }
    cc1120_cs_deassert();
}

/* ===== Extended register access ===========================================*/

uint8_t cc1120_ext_reg_read(uint8_t ext_addr)
{
    uint8_t val;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_READ | CC1120_EXT_ADDR);
    spi_send_byte(ext_addr);
    val = spi_send_byte(0x00);
    cc1120_cs_deassert();
    return val;
}

void cc1120_ext_reg_write(uint8_t ext_addr, uint8_t val)
{
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_WRITE | CC1120_EXT_ADDR);
    spi_send_byte(ext_addr);
    spi_send_byte(val);
    cc1120_cs_deassert();
}

/* ===== FIFO access ========================================================*/

void cc1120_write_txfifo(const uint8_t *buf, size_t len)
{
    size_t i;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_WRITE | CC1120_BURST | CC1120_FIFO);
    for (i = 0; i < len; i++) {
        spi_send_byte(buf[i]);
    }
    cc1120_cs_deassert();
}

void cc1120_read_rxfifo(uint8_t *buf, size_t len)
{
    size_t i;
    cc1120_cs_assert();
    (void)wait_so_low_us(2000);
    spi_send_byte(CC1120_READ | CC1120_BURST | CC1120_FIFO);
    for (i = 0; i < len; i++) {
        buf[i] = spi_send_byte(0x00);
    }
    cc1120_cs_deassert();
}

/* ===== Status helpers =====================================================*/

uint8_t cc1120_get_status(void)
{
    return cc1120_strobe_snop();
}

uint8_t cc1120_get_marcstate(void)
{
    return (cc1120_ext_reg_read(CC1120_MARCSTATE) & 0x1F);
}

uint8_t cc1120_get_num_rxbytes(void)
{
    return cc1120_ext_reg_read(CC1120_NUM_RXBYTES);
}

uint8_t cc1120_get_num_txbytes(void)
{
    return cc1120_ext_reg_read(CC1120_NUM_TXBYTES);
}

/* ===== Reset sequence =====================================================*/

/* Wait for SO (CC1120 MISO pin) to go low while CSn is asserted.
 * SO is wired to RC4 -> SDI1 via PPS, but PORT readback still works on the
 * pin regardless of the PPS routing. Returns 1 if SO dropped within timeout,
 * 0 if it stayed high (chip not ready -> reset/XOSC issue). */
static uint8_t wait_so_low_us(uint16_t timeout_us)
{
    while (timeout_us != 0u) {
        if (PORTCbits.RC4 == 0u) {
            return 1u;
        }
        Delay_us(10);
        timeout_us = (timeout_us > 10u) ? (uint16_t)(timeout_us - 10u) : 0u;
    }
    return 0u;
}

void cc1120_reset(void)
{
    /* Manual reset per CC1120 datasheet §9.1 / SWRU295E §3.2.2 Figure 5:
     * - Toggle CSn high-low-high to re-sync the SPI state machine.
     * - Pull CSn low and WAIT for SO to go low (CHIP_RDYn=0) before SRES.
     * - Send SRES strobe.
     * - KEEP CSn LOW and wait for SO to go low AGAIN — that's how the chip
     *   signals it has finished resetting and the XOSC is settled. Without
     *   this wait, the first PARTNUM read after reset returns 0xFF because
     *   the chip is still settling. */
    cc1120_cs_deassert();
    Delay_us(30);
    cc1120_cs_assert();
    Delay_us(30);
    cc1120_cs_deassert();
    Delay_us(45);

    cc1120_cs_assert();
    Delay_us(50);
    (void)wait_so_low_us(5000);     /* CHIP_RDY before SRES */
    spi_send_byte(CC1120_SRES);
    (void)wait_so_low_us(10000);    /* CHIP_RDY after SRES (settle + XOSC) */
    cc1120_cs_deassert();
    Delay_ms(1);
}

/* ===== Mode helpers =======================================================*/

void cc1120_set_idle(void)
{
    uint16_t timeout = 2000;     /* ~20 ms worst case */
    cc1120_strobe_sidle();
    while (cc1120_get_marcstate() != MARCSTATE_IDLE && --timeout) {
        Delay_us(10);
    }
}

void cc1120_set_rx(void)
{
    cc1120_strobe_srx();
}

void cc1120_flush_rx(void)
{
    cc1120_set_idle();
    cc1120_strobe_sfrx();
}

void cc1120_flush_tx(void)
{
    cc1120_set_idle();
    cc1120_strobe_sftx();
}

/* ===== Register configuration table =======================================
 * SmartRF Studio export for this PCB's target config:
 *   - 169.4 MHz carrier (FREQ = 0x69E466, LO_DIV=20, XOSC=32 MHz)
 *   - 2-GFSK modulation, 4.8 kbps symbol rate
 *   - 100 kHz RX bandwidth (wide to ease tuning during bring-up)
 *   - Variable-length packets, CRC, APPEND_STATUS
 *   - GDO0 = PKT_SYNC_RXTX
 *
 * Addresses use the corrected cc1120_regs.h (FREQ_IF_CFG at 0x0F, full
 * PA_CFG2/1/0 block at 0x2B/2C/2D, etc.).
 *============================================================================*/

/* Standard registers (addr 0x00..0x2E). */
static const uint8_t cc1120_std_regs_[][2] = {
    { CC1120_IOCFG3,         0x30u },     /* unused, hi-Z */
    { CC1120_IOCFG2,         0x30u },     /* unused, hi-Z */
    { CC1120_IOCFG1,         0x30u },     /* unused, hi-Z */
    { CC1120_IOCFG0,         0x06u },     /* GDO0 = PKT_SYNC_RXTX */
    { CC1120_SYNC3,          0x93u },
    { CC1120_SYNC2,          0x0Bu },
    { CC1120_SYNC1,          0x51u },
    { CC1120_SYNC0,          0xDEu },
    { CC1120_SYNC_CFG1,      0x0Bu },
    { CC1120_SYNC_CFG0,      0x17u },
    { CC1120_DEVIATION_M,    0x48u },
    { CC1120_MODCFG_DEV_E,   0x05u },     /* 2-FSK + GFSK shaping */
    { CC1120_DCFILT_CFG,     0x1Cu },
    { CC1120_PREAMBLE_CFG1,  0x18u },     /* 4-byte preamble */
    { CC1120_PREAMBLE_CFG0,  0x2Au },
    { CC1120_FREQ_IF_CFG,    0x40u },     /* SmartRF default for 169 MHz narrowband */
    { CC1120_IQIC,           0xC6u },
    { CC1120_CHAN_BW,        0x08u },     /* 100 kHz RX BW */
    { CC1120_MDMCFG1,        0x46u },
    { CC1120_MDMCFG0,        0x05u },
    { CC1120_SYMBOL_RATE2,   0x84u },     /* 4.8 ksps */
    { CC1120_SYMBOL_RATE1,   0x7Au },
    { CC1120_SYMBOL_RATE0,   0xE1u },
    { CC1120_AGC_REF,        0x27u },
    { CC1120_AGC_CS_THR,     0xF1u },
    { CC1120_AGC_GAIN_ADJUST,0x00u },
    { CC1120_AGC_CFG3,       0x91u },
    { CC1120_AGC_CFG2,       0x20u },
    { CC1120_AGC_CFG1,       0xA9u },
    { CC1120_AGC_CFG0,       0xCFu },
    { CC1120_FIFO_CFG,       0x00u },
    { CC1120_DEV_ADDR,       0x00u },
    { CC1120_SETTLING_CFG,   0x0Bu },     /* FSREG_TIME=3 (max), no auto-cal */
    { CC1120_FS_CFG,         0x1Au },     /* LO_DIV=20, 164..192 MHz band */
    { CC1120_WOR_CFG1,       0x08u },
    { CC1120_WOR_CFG0,       0x21u },
    { CC1120_WOR_EVENT0_MSB, 0x00u },
    { CC1120_WOR_EVENT0_LSB, 0x00u },
    { CC1120_PKT_CFG2,       0x04u },     /* CCA disabled */
    { CC1120_PKT_CFG1,       0x05u },     /* APPEND_STATUS, CRC enabled */
    { CC1120_PKT_CFG0,       0x20u },     /* variable length */
    { CC1120_RFEND_CFG1,     0x0Fu },     /* TX -> RX after packet */
    { CC1120_RFEND_CFG0,     0x00u },
    { CC1120_PA_CFG2,        0x7Fu },
    { CC1120_PA_CFG1,        0x56u },
    { CC1120_PA_CFG0,        0x7Cu },
    { CC1120_PKT_LEN,        0xFFu },
};

/* Extended registers (accessed via 0x2F prefix).
 * SmartRF Studio export for 169.4 MHz / 4.8 kbps 2-GFSK / 32 MHz XOSC. */
static const uint8_t cc1120_ext_regs_[][2] = {
    { CC1120_IF_MIX_CFG,     0x00u },
    { CC1120_FREQOFF_CFG,    0x22u },

    /* FREQ for 169.4 MHz, LO_DIV=20, XOSC=32 MHz.
     * FREQ = 169.4e6 * 20 * 65536 / 32e6 = 0x69E466. */
    { CC1120_FREQ2,          0x69u },
    { CC1120_FREQ1,          0xE4u },
    { CC1120_FREQ0,          0x66u },

    /* IF/ADC config */
    { CC1120_IF_ADC2,        0x02u },
    { CC1120_IF_ADC1,        0xA6u },
    { CC1120_IF_ADC0,        0x04u },

    /* Frequency Synthesizer */
    { CC1120_FS_DIG1,        0x00u },
    { CC1120_FS_DIG0,        0x5Fu },
    { CC1120_FS_CAL3,        0x00u },
    { CC1120_FS_CAL2,        0x20u },
    { CC1120_FS_CAL1,        0x40u },
    { CC1120_FS_CAL0,        0x0Eu },
    { CC1120_FS_CHP,         0x28u },
    { CC1120_FS_DIVTWO,      0x03u },
    { CC1120_FS_DSM1,        0x00u },
    { CC1120_FS_DSM0,        0x33u },
    { CC1120_FS_DVC1,        0xFFu },
    { CC1120_FS_DVC0,        0x17u },
    { CC1120_FS_LBI,         0x00u },
    { CC1120_FS_PFD,         0x50u },
    { CC1120_FS_PRE,         0x6Eu },
    { CC1120_FS_REG_DIV_CML, 0x14u },
    { CC1120_FS_SPARE,       0xACu },
    { CC1120_FS_VCO4,        0x14u },
    { CC1120_FS_VCO3,        0x00u },
    { CC1120_FS_VCO2,        0x00u },
    { CC1120_FS_VCO1,        0x00u },
    { CC1120_FS_VCO0,        0xB4u },

    /* Crystal oscillator trim */
    { CC1120_XOSC5,          0x0Eu },
    { CC1120_XOSC4,          0xA0u },
    { CC1120_XOSC3,          0x03u },
    { CC1120_XOSC2,          0x04u },
    { CC1120_XOSC1,          0x03u },
};

/* ===== Minimal init =======================================================*/

bool cc1120_init_minimal(void)
{
    uint16_t i;
    uint8_t  partnum;

    /* Matches the working RF169Hz.X firmware sequence from last year:
     *   SRES -> 10 ms -> SIDLE -> 10 ms -> ApplyConfig -> 10 ms -> SCAL -> SFTX */

    cc1120_reset();
    Delay_ms(10);

    /* Verify SPI link by reading PARTNUMBER (extended reg 0x8F). */
    partnum = cc1120_ext_reg_read(0x8F);
    if (partnum == 0x00u || partnum == 0xFFu) {
        return false;
    }

    /* Explicit SIDLE strobe before writing registers — ensures the chip
     * is in a known IDLE state. */
    cc1120_strobe(CC1120_SIDLE);
    Delay_ms(10);

    /* Write standard registers, with small delays between writes (matches
     * the working RF169Hz.X firmware that uses __delay_us(100) per write). */
    for (i = 0; i < (sizeof(cc1120_std_regs_) / sizeof(cc1120_std_regs_[0])); i++) {
        cc1120_reg_write(cc1120_std_regs_[i][0], cc1120_std_regs_[i][1]);
        Delay_us(100);
    }
    for (i = 0; i < (sizeof(cc1120_ext_regs_) / sizeof(cc1120_ext_regs_[0])); i++) {
        cc1120_ext_reg_write(cc1120_ext_regs_[i][0], cc1120_ext_regs_[i][1]);
        Delay_us(100);
    }
    Delay_ms(10);

    /* Single SCAL strobe — works with the SmartRF Studio config now that
     * SETTLING_CFG has FSREG_TIME = 3 (max regulator settling time). */
    cc1120_strobe(CC1120_SCAL);
    Delay_ms(10);

    /* Flush TX FIFO (the working firmware does SFTX, not SFRX). */
    cc1120_strobe(CC1120_SFTX);
    Delay_ms(10);

    return true;
}

/* ===== TI errata SWRZ039D manual calibration =============================*/
void cc1120_manual_cal(void)
{
    uint8_t original_fs_cal2;
    uint8_t high_fs_vco2, high_fs_vco4, high_fs_chp;
    uint8_t mid_fs_vco2,  mid_fs_vco4,  mid_fs_chp;
    uint16_t timeout;
    uint8_t marc;

    /* 1) Set VCO cap-array to 0. */
    cc1120_ext_reg_write(CC1120_FS_VCO2, 0x00u);

    /* 2) Save original FS_CAL2. */
    original_fs_cal2 = cc1120_ext_reg_read(CC1120_FS_CAL2);

    /* 3) Start with HIGH VCDAC (original + 2). */
    cc1120_ext_reg_write(CC1120_FS_CAL2, (uint8_t)(original_fs_cal2 + 2u));

    /* 4) Calibrate. */
    cc1120_strobe(CC1120_SCAL);

    /* 5) Wait for MARCSTATE = IDLE (= 0x01 after our & 0x1F mask). */
    timeout = 5000u;     /* ~5 s worst case */
    while (timeout > 0u) {
        marc = cc1120_get_marcstate();
        if (marc == MARCSTATE_IDLE) {
            break;
        }
        Delay_ms(1);
        timeout--;
    }

    /* 6) Read cal results from HIGH VCDAC pass. */
    high_fs_vco2 = cc1120_ext_reg_read(CC1120_FS_VCO2);
    high_fs_vco4 = cc1120_ext_reg_read(CC1120_FS_VCO4);
    high_fs_chp  = cc1120_ext_reg_read(CC1120_FS_CHP);

    /* 7) Set VCO cap-array to 0 again. */
    cc1120_ext_reg_write(CC1120_FS_VCO2, 0x00u);

    /* 8) Continue with MID VCDAC (original). */
    cc1120_ext_reg_write(CC1120_FS_CAL2, original_fs_cal2);

    /* 9) Calibrate again. */
    cc1120_strobe(CC1120_SCAL);

    /* 10) Wait for IDLE again. */
    timeout = 5000u;
    while (timeout > 0u) {
        marc = cc1120_get_marcstate();
        if (marc == MARCSTATE_IDLE) {
            break;
        }
        Delay_ms(1);
        timeout--;
    }

    /* 11) Read cal results from MID VCDAC pass. */
    mid_fs_vco2 = cc1120_ext_reg_read(CC1120_FS_VCO2);
    mid_fs_vco4 = cc1120_ext_reg_read(CC1120_FS_VCO4);
    mid_fs_chp  = cc1120_ext_reg_read(CC1120_FS_CHP);

    /* 12) Keep the result with the HIGHER FS_VCO2 (the bug always picks
     *     too low, so the higher candidate is the safer/correct one). */
    if (high_fs_vco2 > mid_fs_vco2) {
        cc1120_ext_reg_write(CC1120_FS_VCO2, high_fs_vco2);
        cc1120_ext_reg_write(CC1120_FS_VCO4, high_fs_vco4);
        cc1120_ext_reg_write(CC1120_FS_CHP,  high_fs_chp);
    } else {
        cc1120_ext_reg_write(CC1120_FS_VCO2, mid_fs_vco2);
        cc1120_ext_reg_write(CC1120_FS_VCO4, mid_fs_vco4);
        cc1120_ext_reg_write(CC1120_FS_CHP,  mid_fs_chp);
    }
}

bool cc1120_init_no_scal(void)
{
    uint16_t i;
    uint8_t  partnum;

    cc1120_reset();

    partnum = cc1120_ext_reg_read(0x8F);
    if (partnum == 0x00u || partnum == 0xFFu) {
        return false;
    }

    /* Write all configuration registers (same as cc1120_init_minimal). */
    for (i = 0; i < (sizeof(cc1120_std_regs_) / sizeof(cc1120_std_regs_[0])); i++) {
        cc1120_reg_write(cc1120_std_regs_[i][0], cc1120_std_regs_[i][1]);
    }
    for (i = 0; i < (sizeof(cc1120_ext_regs_) / sizeof(cc1120_ext_regs_[0])); i++) {
        cc1120_ext_reg_write(cc1120_ext_regs_[i][0], cc1120_ext_regs_[i][1]);
    }

    /* SKIP the SCAL strobe — that is what we are isolating in Phase 3c. */

    cc1120_flush_rx();
    cc1120_flush_tx();

    return true;
}

/* ===== Radio event subsystem ==============================================*/

void isr_cc1120_gdo(void)
{
    radio_event_flags_ |= RADIO_EVT_GDO_EDGE;
    gdo_irq_pending_    = true;
}

void cc1120_process_events(void)
{
    uint8_t marc;

    if (!gdo_irq_pending_) {
        return;
    }
    gdo_irq_pending_ = false;

    /* IOCFG0 = PKT_SYNC_RXTX: a falling edge marks "end of packet" in RX or
     * TX. We disambiguate by inspecting MARCSTATE and FIFO occupancy. */
    marc = cc1120_get_marcstate();

    if (marc == MARCSTATE_RX_FIFO_ERR) {
        radio_event_flags_ |= RADIO_EVT_RX_OVERFLOW;
        return;
    }
    if (marc == MARCSTATE_TX_FIFO_ERR) {
        radio_event_flags_ |= RADIO_EVT_TX_UNDERFLOW;
        return;
    }
    if (cc1120_get_num_rxbytes() > 0) {
        radio_event_flags_ |= RADIO_EVT_RX_DONE;
    } else {
        radio_event_flags_ |= RADIO_EVT_TX_DONE;
    }
}

uint8_t cc1120_events_get_and_clear(uint8_t mask)
{
    uint8_t matched;
    uint8_t tok = board_int_save_disable();
    matched = radio_event_flags_ & mask;
    radio_event_flags_ &= (uint8_t)~mask;
    board_int_restore(tok);
    return matched;
}

void cc1120_events_clear_all(void)
{
    uint8_t tok = board_int_save_disable();
    radio_event_flags_ = 0;
    gdo_irq_pending_   = false;
    board_int_restore(tok);
}
