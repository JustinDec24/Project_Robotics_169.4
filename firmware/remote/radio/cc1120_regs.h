/*============================================================================
 * radio/cc1120_regs.h — CC1120 register addresses and command strobes
 *
 * Addresses taken from the CC1120 datasheet (SWRS098).
 *
 * TODO: Fill all register values from SmartRF Studio export for 169.4 MHz.
 *       Copy the register VALUE table into cc1120.c :: cc1120_init_minimal().
 *       The addresses below are correct; only the *values to write* are
 *       project-specific and must come from SmartRF / TI docs.
 *===========================================================================*/
#ifndef CC1120_REGS_H
#define CC1120_REGS_H

/* ===== SPI header bits ====================================================*/
#define CC1120_READ             0x80
#define CC1120_BURST            0x40
#define CC1120_WRITE            0x00

/* ===== Command strobes (0x30 – 0x3D) =====================================*/
#define CC1120_SRES             0x30    /* Reset chip                        */
#define CC1120_SFSTXON          0x31    /* Enable and calibrate              */
#define CC1120_SXOFF            0x32    /* Crystal off                       */
#define CC1120_SCAL             0x33    /* Calibrate synthesizer             */
#define CC1120_SRX              0x34    /* Enable RX                         */
#define CC1120_STX              0x35    /* Enable TX                         */
#define CC1120_SIDLE            0x36    /* Exit RX/TX to IDLE                */
#define CC1120_SAFC             0x37    /* AFC adjust                        */
#define CC1120_SWOR             0x38    /* Start eWOR                        */
#define CC1120_SPWD             0x39    /* Power down when CSn high          */
#define CC1120_SFRX             0x3A    /* Flush RX FIFO                     */
#define CC1120_SFTX             0x3B    /* Flush TX FIFO                     */
#define CC1120_SWORRST          0x3C    /* Reset eWOR timer                  */
#define CC1120_SNOP             0x3D    /* No operation (read status byte)   */

/* ===== FIFO access ========================================================*/
#define CC1120_FIFO             0x3F    /* Standard FIFO access address      */

/* ===== Standard configuration registers (0x00 – 0x2E) ====================*/
#define CC1120_IOCFG3           0x00
#define CC1120_IOCFG2           0x01
#define CC1120_IOCFG1           0x02
#define CC1120_IOCFG0           0x03    /* TODO: GDO0 mapping                */
#define CC1120_SYNC3            0x04
#define CC1120_SYNC2            0x05
#define CC1120_SYNC1            0x06
#define CC1120_SYNC0            0x07
#define CC1120_SYNC_CFG1        0x08
#define CC1120_SYNC_CFG0        0x09
#define CC1120_DEVIATION_M      0x0A
#define CC1120_MODCFG_DEV_E     0x0B
#define CC1120_DCFILT_CFG       0x0C
#define CC1120_PREAMBLE_CFG1    0x0D
#define CC1120_PREAMBLE_CFG0    0x0E
#define CC1120_IQIC             0x0F
#define CC1120_CHAN_BW          0x10
#define CC1120_MDMCFG1          0x11
#define CC1120_MDMCFG0          0x12
#define CC1120_SYMBOL_RATE2     0x13
#define CC1120_SYMBOL_RATE1     0x14
#define CC1120_SYMBOL_RATE0     0x15
#define CC1120_AGC_REF          0x16
#define CC1120_AGC_CS_THR       0x17
#define CC1120_AGC_GAIN_ADJUST  0x18
#define CC1120_AGC_CFG3         0x19
#define CC1120_AGC_CFG2         0x1A
#define CC1120_AGC_CFG1         0x1B
#define CC1120_AGC_CFG0         0x1C
#define CC1120_FIFO_CFG         0x1D
#define CC1120_DEV_ADDR         0x1E
#define CC1120_SETTLING_CFG     0x1F
#define CC1120_FS_CFG           0x20
#define CC1120_WOR_CFG1         0x21
#define CC1120_WOR_CFG0         0x22
#define CC1120_WOR_EVENT0_MSB   0x23
#define CC1120_WOR_EVENT0_LSB   0x24
#define CC1120_RXDCM_TIME       0x25
#define CC1120_PKT_CFG2         0x26
#define CC1120_PKT_CFG1         0x27
#define CC1120_PKT_CFG0         0x28
#define CC1120_RFEND_CFG1       0x29
#define CC1120_RFEND_CFG0       0x2A
#define CC1120_PA_CFG1          0x2B
#define CC1120_PA_CFG0          0x2C
#define CC1120_ASK_CFG          0x2D
#define CC1120_PKT_LEN          0x2E

/* ===== Extended register space (two-byte SPI header: 0x2F + ext_addr) =====*/
#define CC1120_EXT_ADDR         0x2F    /* First SPI byte for extended regs  */

/* Extended registers used by this firmware: */
#define CC1120_IF_MIX_CFG       0x00
#define CC1120_FREQOFF_CFG      0x01
#define CC1120_FREQ2            0x0C
#define CC1120_FREQ1            0x0D
#define CC1120_FREQ0            0x0E
#define CC1120_IF_ADC0          0x11
#define CC1120_FS_DIG1          0x12
#define CC1120_FS_DIG0          0x13
#define CC1120_FS_CAL0          0x14
#define CC1120_FS_DIVTWO        0x17
#define CC1120_FS_DSM0          0x19
#define CC1120_FS_DVC0          0x1B
#define CC1120_FS_PFD           0x1F
#define CC1120_FS_PRE           0x20
#define CC1120_FS_REG_DIV_CML   0x21
#define CC1120_FS_SPARE         0x22
#define CC1120_FS_VCO4          0x23
#define CC1120_FS_VCO2          0x25
#define CC1120_FS_VCO0          0x27
#define CC1120_XOSC5            0x32
#define CC1120_XOSC1            0x36

/* Extended status registers (read-only): */
#define CC1120_MARCSTATE        0x73
#define CC1120_RSSI1            0x71
#define CC1120_LQI_VAL          0x72
#define CC1120_NUM_TXBYTES      0x7D
#define CC1120_NUM_RXBYTES      0x7E

/* ===== MARCSTATE values ===================================================*/
#define MARCSTATE_IDLE          0x01
#define MARCSTATE_RX            0x0D
#define MARCSTATE_TX            0x13
#define MARCSTATE_RX_FIFO_ERR   0x11
#define MARCSTATE_TX_FIFO_ERR   0x16

/* ===== Status byte bits ===================================================*/
#define CC1120_STATUS_CHIP_RDY  0x80
#define CC1120_STATUS_STATE     0x70
#define CC1120_STATUS_FIFO      0x0F

#endif /* CC1120_REGS_H */
