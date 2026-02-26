/*============================================================================
 * radio/cc1120.c — CC1120 low-level SPI command implementation
 *===========================================================================*/
#include "cc1120.h"
#include "cc1120_regs.h"
#include "../board/board.h"
#include "../drivers/spi.h"

static volatile uint8_t radio_event_flags = 0;
static volatile bool gdo_irq_pending = false;

/* ===== Internal SPI helpers ===============================================*/

static uint8_t spi_send_byte(uint8_t tx)
{
    return spi_transfer_byte(tx);
}

/* ===== Command strobes ====================================================*/

uint8_t cc1120_strobe(uint8_t strobe_addr)
{
    cc1120_cs_assert();
    uint8_t status = spi_send_byte(strobe_addr);
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
    cc1120_cs_assert();
    spi_send_byte(CC1120_READ | addr);
    uint8_t val = spi_send_byte(0x00);
    cc1120_cs_deassert();
    return val;
}

void cc1120_reg_write(uint8_t addr, uint8_t val)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_WRITE | addr);
    spi_send_byte(val);
    cc1120_cs_deassert();
}

void cc1120_reg_burst_read(uint8_t addr, uint8_t *buf, size_t len)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_READ | CC1120_BURST | addr);
    for (size_t i = 0; i < len; i++)
        buf[i] = spi_send_byte(0x00);
    cc1120_cs_deassert();
}

void cc1120_reg_burst_write(uint8_t addr, const uint8_t *buf, size_t len)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_WRITE | CC1120_BURST | addr);
    for (size_t i = 0; i < len; i++)
        spi_send_byte(buf[i]);
    cc1120_cs_deassert();
}

/* ===== Extended register access ===========================================*/

uint8_t cc1120_ext_reg_read(uint8_t ext_addr)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_READ | CC1120_EXT_ADDR);
    spi_send_byte(ext_addr);
    uint8_t val = spi_send_byte(0x00);
    cc1120_cs_deassert();
    return val;
}

void cc1120_ext_reg_write(uint8_t ext_addr, uint8_t val)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_WRITE | CC1120_EXT_ADDR);
    spi_send_byte(ext_addr);
    spi_send_byte(val);
    cc1120_cs_deassert();
}

/* ===== FIFO access ========================================================*/

void cc1120_write_txfifo(const uint8_t *buf, size_t len)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_WRITE | CC1120_BURST | CC1120_FIFO);
    for (size_t i = 0; i < len; i++)
        spi_send_byte(buf[i]);
    cc1120_cs_deassert();
}

void cc1120_read_rxfifo(uint8_t *buf, size_t len)
{
    cc1120_cs_assert();
    spi_send_byte(CC1120_READ | CC1120_BURST | CC1120_FIFO);
    for (size_t i = 0; i < len; i++)
        buf[i] = spi_send_byte(0x00);
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

void cc1120_reset(void)
{
    /* Manual reset sequence per CC1120 datasheet Section 6.1:
     * 1. Pull CSn low, then high (>40 us) to init SPI
     * 2. Pull CSn low, wait for MISO (SO) to go low
     * 3. Send SRES strobe
     * 4. Wait for MISO to go low (chip ready)
     */
    cc1120_cs_deassert();
    Delay_us(30);
    cc1120_cs_assert();
    Delay_us(30);
    cc1120_cs_deassert();
    Delay_us(45);

    cc1120_cs_assert();
    /* TODO: Ideally wait for MISO low here.  For now, use a fixed delay. */
    Delay_us(50);
    spi_send_byte(CC1120_SRES);
    /* Wait for chip ready after reset */
    Delay_ms(10);
    cc1120_cs_deassert();
}

/* ===== Convenience ========================================================*/

void cc1120_set_idle(void)
{
    cc1120_strobe_sidle();
    /* Wait until actually in IDLE */
    uint8_t timeout = 200;
    while (cc1120_get_marcstate() != MARCSTATE_IDLE && --timeout) {
        Delay_us(100);
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

/* ===== Minimal init =======================================================*/

bool cc1120_init_minimal(void)
{
    cc1120_reset();

    /* Verify SPI link by reading a known register.
     * IOCFG0 default is 0x09 per the CC1120 datasheet (reset value).
     * TODO: Verify this expected value against your CC1120 revision.
     */
    uint8_t partnum = cc1120_ext_reg_read(0x8F);  /* PARTNUMBER ext reg */
    (void)partnum;

    /*
     * ===================================================================
     * TODO: LOAD REGISTER CONFIGURATION FROM SmartRF Studio EXPORT
     *
     * Generate a register table with SmartRF Studio for:
     *   - 169.4 MHz carrier
     *   - Desired modulation (2-GFSK recommended for sub-GHz)
     *   - Desired data rate
     *   - Variable-length packet mode
     *   - CRC enabled (auto by CC1120)
     *   - Append status (RSSI + LQI) after RX payload
     *
     * Then write each register here.  Example pattern:
     *
     *   cc1120_reg_write(CC1120_IOCFG0, 0x06);   // GDO0 = sync word
     *   cc1120_reg_write(CC1120_SYNC3,  0x93);
     *   cc1120_reg_write(CC1120_SYNC2,  0x0B);
     *   cc1120_reg_write(CC1120_SYNC1,  0x51);
     *   cc1120_reg_write(CC1120_SYNC0,  0xDE);
     *   cc1120_reg_write(CC1120_SYNC_CFG1, 0x08);
     *   cc1120_reg_write(CC1120_DEVIATION_M, ...);
     *   cc1120_reg_write(CC1120_MODCFG_DEV_E, ...);
     *   ...
     *   cc1120_reg_write(CC1120_PKT_CFG0, 0x20); // variable length mode
     *   cc1120_reg_write(CC1120_PKT_CFG1, 0x05); // CRC + addr check off
     *   cc1120_reg_write(CC1120_RFEND_CFG1, 0x0F); // RX after TX
     *   cc1120_reg_write(CC1120_PKT_LEN, RF_MAX_PKT_SIZE);
     *
     *   // Extended registers (frequency, IF, FS, etc.):
     *   cc1120_ext_reg_write(CC1120_FREQ2, 0x??);
     *   cc1120_ext_reg_write(CC1120_FREQ1, 0x??);
     *   cc1120_ext_reg_write(CC1120_FREQ0, 0x??);
     *   ...
     * ===================================================================
     */

    /* TODO: Configure GDO mapping and interrupt polarity in SmartRF export.
     * Common choices:
     *   0x01 = RX FIFO above threshold
     *   0x06 = Sync word received (assert) / end of packet (deassert)
     *   0x02 = TX FIFO above threshold
     * The mapping depends on your intended ISR usage.
     *
     * cc1120_reg_write(CC1120_IOCFG0, 0x06);
     */

    /* Flush FIFOs and enter IDLE */
    cc1120_flush_rx();
    cc1120_flush_tx();

    return true;
}

/* ===== Radio event subsystem ==============================================*/

void isr_cc1120_gdo(void)
{
    radio_event_flags |= RADIO_EVT_GDO_EDGE;
    gdo_irq_pending = true;
}

void cc1120_process_events(void)
{
    if (!gdo_irq_pending)
        return;
    gdo_irq_pending = false;

    /*
     * Classify the GDO edge into a semantic event.
     *
     * With a single GDO pin (e.g. IOCFG0 = 0x06, PKT_SYNC_RXTX) the same
     * signal fires for both RX-complete and TX-complete.  We disambiguate by
     * inspecting the radio state after the fact.
     *
     * TODO: If you use two GDO pins (one for RX, one for TX), split into
     *       separate ISR entry-points and set the flags directly there.
     */
    uint8_t marc = cc1120_get_marcstate();

    if (marc == MARCSTATE_RX_FIFO_ERR) {
        radio_event_flags |= RADIO_EVT_RX_OVERFLOW;
        return;
    }
    if (marc == MARCSTATE_TX_FIFO_ERR) {
        radio_event_flags |= RADIO_EVT_TX_UNDERFLOW;
        return;
    }

    /* Single-GDO heuristic: RX bytes pending => RX_DONE, otherwise TX_DONE. */
    if (cc1120_get_num_rxbytes() > 0) {
        radio_event_flags |= RADIO_EVT_RX_DONE;
    } else {
        radio_event_flags |= RADIO_EVT_TX_DONE;
    }
}

uint8_t cc1120_events_get_and_clear(uint8_t mask)
{
    global_int_disable();
    uint8_t matched = radio_event_flags & mask;
    radio_event_flags &= (uint8_t)~mask;
    global_int_enable();
    return matched;
}

void cc1120_events_clear_all(void)
{
    global_int_disable();
    radio_event_flags = 0;
    gdo_irq_pending   = false;
    global_int_enable();
}
