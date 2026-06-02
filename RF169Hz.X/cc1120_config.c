#include "cc1120_config.h"

//*********************Configuration continuous TX unmodulation***************//
//const cc1120_reg_setting_t cc1120_config[] = {
//    {0x0000, 0xB0}, // IOCFG3
//    {0x0001, 0x06}, // IOCFG2
//    {0x0002, 0xB0}, // IOCFG1
//    {0x0003, 0x06}, // IOCFG0
//    {0x0004, 0x93}, // SYNC3
//    {0x0005, 0x0B}, // SYNC2
//    {0x0006, 0x51}, // SYNC1
//    {0x0007, 0xDE}, // SYNC0
//    {0x0008, 0x0B}, // SYNC_CFG1
//    {0x0009, 0x17}, // SYNC_CFG0
//    {0x000A, 0x06}, // DEVIATION_M
//    {0x000B, 0x03}, // MODCFG_DEV_E
//    {0x000C, 0x1C}, // DCFILT_CFG
//    {0x000D, 0x00}, // PREAMBLE_CFG1
//    {0x000E, 0x2A}, // PREAMBLE_CFG0
//    {0x000F, 0x40}, // FREQ_IF_CFG
//    {0x0010, 0xC6}, // IQIC
//    {0x0011, 0x14}, // CHAN_BW
//    {0x0012, 0x06}, // MDMCFG1
//    {0x0013, 0x05}, // MDMCFG0
//    {0x0014, 0x43}, // SYMBOL_RATE2
//    {0x15, 0xA9}, // SYMBOL_RATE1
//    {0x16, 0x2A}, // SYMBOL_RATE0
//    {0x0017, 0x20}, // AGC_REF
//    {0x18, 0x19}, // AGC_CS_THR
//    {0x19, 0x00}, // AGC_GAIN_ADJUST
//    {0x1A, 0x91}, // AGC_CFG3
//    {0x1B, 0x20}, // AGC_CFG2
//    {0x001C, 0xA9}, // AGC_CFG1
//    {0x001D, 0xCF}, // AGC_CFG0
//    {0x001E, 0x00}, // FIFO_CFG
//    {0x1F, 0x00}, // DEV_ADDR
//    {0x20, 0x0B}, // SETTLING_CFG
//    {0x0021, 0x1A}, // FS_CFG
//    {0x22, 0x08}, // WOR_CFG1
//    {0x23, 0x21}, // WOR_CFG0
//    {0x24, 0x00}, // WOR_EVENT0_MSB
//    {0x25, 0x00}, // WOR_EVENT0_LSB
//    {0x0026, 0x05}, // PKT_CFG2
//    {0x0027, 0x00}, // PKT_CFG1
//    {0x0028, 0x20}, // PKT_CFG0
//    {0x29, 0x0F}, // RFEND_CFG1
//    {0x2A, 0x00}, // RFEND_CFG0
//    {0x2B, 0x3F}, // PA_CFG2
//    {0x2C, 0x56}, // PA_CFG1
//    {0x2D, 0x7C}, // PA_CFG0
//    {0x2E, 0x03}, // PKT_LEN
//    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
//    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
//    {0x2F02, 0x0B}, // TOC_CFG
//    {0x2F03, 0x00}, // MARC_SPARE
//    {0x2F04, 0x00}, // ECG_CFG
//    {0x2F05, 0x01}, // CFM_DATA_CFG
//    {0x2F06, 0x01}, // EXT_CTRL
//    {0x2F07, 0x00}, // RCCAL_FINE
//    {0x2F08, 0x00}, // RCCAL_COARSE
//    {0x2F09, 0x00}, // RCCAL_OFFSET
//    {0x2F0A, 0x00}, // FREQOFF1
//    {0x2F0B, 0x00}, // FREQOFF0
//    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
//    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
//    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
//    {0x2F0F, 0x02}, // IF_ADC2
//    {0x2D10, 0xA6}, // IF_ADC1
//    {0x2D11, 0x04}, // IF_ADC0
//    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
//    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
//    {0x2F14, 0x00}, // FS_CAL3, Frequency Synthesizer Calibration Reg. 3
//    {0x2F15, 0x20}, // FS_CAL2, Frequency Synthesizer Calibration Reg. 2
//    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
//    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
//    {0x2F18, 0x28}, // FS_CHP
//    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
//    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
//    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
//    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration
//    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
//    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
//    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
//    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
//    {0x2F23, 0x14}, // FS_VCO4, FS Voltage Controlled Oscillator Configuration Reg.4
//    {0x2F24, 0x00}, // FS_VCO3, FS Voltage Controlled Oscillator Configuration Reg.3
//    {0x2F25, 0x00}, // FS_VCO2, FS Voltage Controlled Oscillator Configuration Reg.2
//    {0x2F26, 0x00}, // FS_VCO1, FS Voltage Controlled Oscillator Configuration Reg.1
//    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.0
//    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
//    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
//    {0x2F91, 0x08}
//    // The following should be moved to extended register access if used
//    // and require your driver to handle addresses > 0x2F
////    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
//    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
//    // ...
//};



//****************************1.2 kbps (2-FSK)********************************//

//const cc1120_reg_setting_t cc1120_config[] = {
//    {0x0000, 0xB0}, // IOCFG3
//    {0x0001, 0x06}, // IOCFG2
//    {0x0002, 0xB0}, // IOCFG1
//    {0x0003, 0x06}, // IOCFG0
//    {0x0004, 0x93}, // SYNC3
//    {0x0005, 0x0B}, // SYNC2
//    {0x0006, 0x51}, // SYNC1
//    {0x0007, 0xDE}, // SYNC0
//    {0x0008, 0x0B}, // SYNC_CFG1
//    {0x0009, 0x17}, // SYNC_CFG0
//    {0x000A, 0x06}, // DEVIATION_M
//    {0x000B, 0x03}, // MODCFG_DEV_E
//    {0x000C, 0x1C}, // DCFILT_CFG
//    {0x000D, 0x14}, // PREAMBLE_CFG1
//    {0x000E, 0x2A}, // PREAMBLE_CFG0
//    {0x000F, 0x40}, // FREQ_IF_CFG
//    {0x0010, 0xC6}, // IQIC
//    {0x0011, 0x14}, // CHAN_BW
//    {0x0012, 0x46}, // MDMCFG1
//    {0x0013, 0x05}, // MDMCFG0
//    {0x0014, 0x43}, // SYMBOL_RATE2
//    {0x0015, 0xA9}, // SYMBOL_RATE1
//    {0x0016, 0x2A}, // SYMBOL_RATE0
//    {0x0017, 0x20}, // AGC_REF
//    {0x0018, 0x19}, // AGC_CS_THR
//    {0x0019, 0x00}, // AGC_GAIN_ADJUST
//    {0x001A, 0x91}, // AGC_CFG3
//    {0x001B, 0x20}, // AGC_CFG2
//    {0x001C, 0xA9}, // AGC_CFG1
//    {0x001D, 0xCF}, // AGC_CFG0
//    {0x001E, 0x00}, // FIFO_CFG
//    {0x001F, 0x00}, // DEV_ADDR
//    {0x0020, 0x0B}, // SETTLING_CFG
//    {0x0021, 0x1A}, // FS_CFG
//    {0x0022, 0x08}, // WOR_CFG1
//    {0x0023, 0x21}, // WOR_CFG0
//    {0x0024, 0x00}, // WOR_EVENT0_MSB
//    {0x0025, 0x00}, // WOR_EVENT0_LSB
//    {0x0026, 0x00}, // PKT_CFG2
//    {0x0027, 0x04}, // PKT_CFG1
//    {0x0028, 0x20}, // PKT_CFG0
//    {0x0029, 0x0F}, // RFEND_CFG1
//    {0x002A, 0x00}, // RFEND_CFG0
//    {0x002B, 0x7F}, // PA_CFG2
//    {0x002C, 0x56}, // PA_CFG1
//    {0x002D, 0x7C}, // PA_CFG0
//    {0x2E, 0xFF}, // PKT_LEN
//    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
//    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
//    {0x2F02, 0x0B}, // TOC_CFG
//    {0x2F03, 0x00}, // MARC_SPARE
//    {0x2F04, 0x00}, // ECG_CFG
//    {0x2F05, 0x00}, // CFM_DATA_CFG
//    {0x2F06, 0x01}, // EXT_CTRL
//    {0x2F07, 0x00}, // RCCAL_FINE
//    {0x2F08, 0x00}, // RCCAL_COARSE
//    {0x2F09, 0x00}, // RCCAL_OFFSET
//    {0x2F0A, 0x00}, // FREQOFF1
//    {0x2F0B, 0x00}, // FREQOFF0
//    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
//    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
//    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
//    {0x2F0F, 0x02}, // IF_ADC2
//    {0x2F10, 0xA6}, // IF_ADC1
//    {0x2F11, 0x04}, // IF_ADC0
//    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
//    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
//    {0x2F14, 0x00}, // FS_CAL13
//    {0x2F15, 0x20}, // FS_CAL2
//    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
//    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
//    {0x2F18, 0x28}, // FS_CHP
//    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
//    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
//    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
//    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
////    {0x2F1E, 0x00}, // FS_LBI
//    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
//    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
//    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
//    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
//    {0x2F23, 0x14}, // FS_VCO4
//    {0x2F24, 0x00}, // FS_VCO3
//    {0x2F25, 0x00}, // FS_VCO2
//    {0x2F26, 0x00}, // FS_VCO1
//    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.
//    {0x2F28, 0x00}, // GBIAS6
//    {0x2F29, 0x02}, // GBIAS5
//    {0x2F2A, 0x00}, // GBIAS4
//    {0x2F2B, 0x00}, // GBIAS3
//    {0x2F2C, 0x10}, // GBIAS2
//    {0x2F2D, 0x00}, // GBIAS1
//    {0x2F2E, 0x00}, // GBIAS0
//    {0x2F2F, 0x01}, // IFAMP
//    {0x2F30, 0x01}, // LNA
//    {0x2F31, 0x01}, // RXMIX
//    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
//    {0x2F33, 0xA0}, // XOSC4, Crystal Oscillator Configuration Reg. 4
//    {0x2F34, 0x03}, // XOSC3, Crystal Oscillator Configuration Reg. 3
//    {0x2F35, 0x01}, // XOSC2, Crystal Oscillator Configuration Reg. 2
//    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
////    {0x2F37, 0x00}, // XOSC0, Crystal Oscillator Configuration Reg. 0
//    {0x2F38, 0x00}, // ANALOG_SPARE
//    {0x2F39, 0x00}, //PA_CFG3
//    // The following should be moved to extended register access if used
//    // and require your driver to handle addresses > 0x2F
////    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
//    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
//    // ...
//};


//****************************data rate 2.4 kbps (4-FSK)**********************//
//const cc1120_reg_setting_t cc1120_config[] = {
//    {0x0000, 0xB0}, // IOCFG3
//    {0x0001, 0x06}, // IOCFG2
//    {0x0002, 0xB0}, // IOCFG1
//    {0x0003, 0x06}, // IOCFG0
//    {0x0004, 0x93}, // SYNC3
//    {0x0005, 0x0B}, // SYNC2
//    {0x0006, 0x51}, // SYNC1
//    {0x0007, 0xDE}, // SYNC0
//    {0x0008, 0x0B}, // SYNC_CFG1
//    {0x0009, 0x17}, // SYNC_CFG0
//    {0x000A, 0x06}, // DEVIATION_M
//    {0x000B, 0x23}, // MODCFG_DEV_E
//    {0x000C, 0x1C}, // DCFILT_CFG
//    {0x000D, 0x14}, // PREAMBLE_CFG1
//    {0x000E, 0x2A}, // PREAMBLE_CFG0
//    {0x000F, 0x40}, // FREQ_IF_CFG
//    {0x0010, 0xC6}, // IQIC
//    {0x0011, 0x14}, // CHAN_BW
//    {0x0012, 0x46}, // MDMCFG1
//    {0x0013, 0x05}, // MDMCFG0
//    {0x0014, 0x43}, // SYMBOL_RATE2
//    {0x0015, 0xA9}, // SYMBOL_RATE1
//    {0x0016, 0x2A}, // SYMBOL_RATE0
//    {0x0017, 0x20}, // AGC_REF
//    {0x0018, 0x19}, // AGC_CS_THR
//    {0x0019, 0x00}, // AGC_GAIN_ADJUST
//    {0x001A, 0x91}, // AGC_CFG3
//    {0x001B, 0x20}, // AGC_CFG2
//    {0x001C, 0xA9}, // AGC_CFG1
//    {0x001D, 0xCF}, // AGC_CFG0
//    {0x001E, 0x00}, // FIFO_CFG
//    {0x001F, 0x00}, // DEV_ADDR
//    {0x0020, 0x0B}, // SETTLING_CFG
//    {0x0021, 0x1A}, // FS_CFG
//    {0x0022, 0x08}, // WOR_CFG1
//    {0x0023, 0x21}, // WOR_CFG0
//    {0x0024, 0x00}, // WOR_EVENT0_MSB
//    {0x0025, 0x00}, // WOR_EVENT0_LSB
//    {0x0026, 0x00}, // PKT_CFG2
//    {0x0027, 0x04}, // PKT_CFG1
//    {0x0028, 0x20}, // PKT_CFG0
//    {0x0029, 0x0F}, // RFEND_CFG1
//    {0x002A, 0x00}, // RFEND_CFG0
//    {0x002B, 0x7F}, // PA_CFG2
//    {0x002C, 0x56}, // PA_CFG1
//    {0x002D, 0x7C}, // PA_CFG0
//    {0x2E, 0xFF}, // PKT_LEN
//    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
//    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
//    {0x2F02, 0x0B}, // TOC_CFG
//    {0x2F03, 0x00}, // MARC_SPARE
//    {0x2F04, 0x00}, // ECG_CFG
//    {0x2F05, 0x00}, // CFM_DATA_CFG
//    {0x2F06, 0x01}, // EXT_CTRL
//    {0x2F07, 0x00}, // RCCAL_FINE
//    {0x2F08, 0x00}, // RCCAL_COARSE
//    {0x2F09, 0x00}, // RCCAL_OFFSET
//    {0x2F0A, 0x00}, // FREQOFF1
//    {0x2F0B, 0x00}, // FREQOFF0
//    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
//    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
//    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
//    {0x2F0F, 0x02}, // IF_ADC2
//    {0x2F10, 0xA6}, // IF_ADC1
//    {0x2F11, 0x04}, // IF_ADC0
//    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
//    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
//    {0x2F14, 0x00}, // FS_CAL13
//    {0x2F15, 0x20}, // FS_CAL2
//    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
//    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
//    {0x2F18, 0x28}, // FS_CHP
//    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
//    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
//    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
//    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
////    {0x2F1E, 0x00}, // FS_LBI
//    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
//    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
//    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
//    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
//    {0x2F23, 0x14}, // FS_VCO4
//    {0x2F24, 0x00}, // FS_VCO3
//    {0x2F25, 0x00}, // FS_VCO2
//    {0x2F26, 0x00}, // FS_VCO1
//    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.
//    {0x2F28, 0x00}, // GBIAS6
//    {0x2F29, 0x02}, // GBIAS5
//    {0x2F2A, 0x00}, // GBIAS4
//    {0x2F2B, 0x00}, // GBIAS3
//    {0x2F2C, 0x10}, // GBIAS2
//    {0x2F2D, 0x00}, // GBIAS1
//    {0x2F2E, 0x00}, // GBIAS0
//    {0x2F2F, 0x01}, // IFAMP
//    {0x2F30, 0x01}, // LNA
//    {0x2F31, 0x01}, // RXMIX
//    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
//    {0x2F33, 0xA0}, // XOSC4, Crystal Oscillator Configuration Reg. 4
//    {0x2F34, 0x03}, // XOSC3, Crystal Oscillator Configuration Reg. 3
//    {0x2F35, 0x01}, // XOSC2, Crystal Oscillator Configuration Reg. 2
//    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
////    {0x2F37, 0x00}, // XOSC0, Crystal Oscillator Configuration Reg. 0
//    {0x2F38, 0x00}, // ANALOG_SPARE
//    {0x2F39, 0x00}, //PA_CFG3
//    // The following should be moved to extended register access if used
//    // and require your driver to handle addresses > 0x2F
////    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
//    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
//    // ...
//};


//****************************data rate 24 kbps (4-FSK)***********************//

//const cc1120_reg_setting_t cc1120_config[] = {
//    {0x0000, 0xB0}, // IOCFG3
//    {0x0001, 0x06}, // IOCFG2
//    {0x0002, 0xB0}, // IOCFG1
//    {0x0003, 0x06}, // IOCFG0
//    {0x0004, 0x93}, // SYNC3
//    {0x0005, 0x0B}, // SYNC2
//    {0x0006, 0x51}, // SYNC1
//    {0x0007, 0xDE}, // SYNC0
//    {0x0008, 0x0B}, // SYNC_CFG1
//    {0x0009, 0x17}, // SYNC_CFG0
//    {0x000A, 0x06}, // DEVIATION_M
//    {0x000B, 0x24}, // MODCFG_DEV_E
//    {0x000C, 0x1C}, // DCFILT_CFG
//    {0x000D, 0x14}, // PREAMBLE_CFG1
//    {0x000E, 0x2A}, // PREAMBLE_CFG0
//    {0x000F, 0x40}, // FREQ_IF_CFG
//    {0x0010, 0x46}, // IQIC
//    {0x0011, 0x04}, // CHAN_BW
//    {0x0012, 0x46}, // MDMCFG1
//    {0x0013, 0x05}, // MDMCFG0
//    {0x0014, 0x78}, // SYMBOL_RATE2
//    {0x0015, 0x93}, // SYMBOL_RATE1
//    {0x0016, 0x75}, // SYMBOL_RATE0
//    {0x0017, 0x20}, // AGC_REF
//    {0x0018, 0x19}, // AGC_CS_THR
//    {0x0019, 0x00}, // AGC_GAIN_ADJUST
//    {0x001A, 0x91}, // AGC_CFG3
//    {0x001B, 0x20}, // AGC_CFG2
//    {0x001C, 0xA9}, // AGC_CFG1
//    {0x001D, 0xCF}, // AGC_CFG0
//    {0x001E, 0x00}, // FIFO_CFG
//    {0x001F, 0x00}, // DEV_ADDR
//    {0x0020, 0x0B}, // SETTLING_CFG
//    {0x0021, 0x1A}, // FS_CFG
//    {0x0022, 0x08}, // WOR_CFG1
//    {0x0023, 0x21}, // WOR_CFG0
//    {0x0024, 0x00}, // WOR_EVENT0_MSB
//    {0x0025, 0x00}, // WOR_EVENT0_LSB
//    {0x0026, 0x00}, // PKT_CFG2
//    {0x0027, 0x04}, // PKT_CFG1
//    {0x0028, 0x20}, // PKT_CFG0
//    {0x0029, 0x0F}, // RFEND_CFG1
//    {0x002A, 0x00}, // RFEND_CFG0
//    {0x002B, 0x7F}, // PA_CFG2
//    {0x002C, 0x56}, // PA_CFG1
//    {0x002D, 0x7C}, // PA_CFG0
//    {0x2E, 0xFF}, // PKT_LEN
//    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
//    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
//    {0x2F02, 0x0B}, // TOC_CFG
//    {0x2F03, 0x00}, // MARC_SPARE
//    {0x2F04, 0x00}, // ECG_CFG
//    {0x2F05, 0x00}, // CFM_DATA_CFG
//    {0x2F06, 0x01}, // EXT_CTRL
//    {0x2F07, 0x00}, // RCCAL_FINE
//    {0x2F08, 0x00}, // RCCAL_COARSE
//    {0x2F09, 0x00}, // RCCAL_OFFSET
//    {0x2F0A, 0x00}, // FREQOFF1
//    {0x2F0B, 0x00}, // FREQOFF0
//    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
//    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
//    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
//    {0x2F0F, 0x02}, // IF_ADC2
//    {0x2F10, 0xA6}, // IF_ADC1
//    {0x2F11, 0x04}, // IF_ADC0
//    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
//    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
//    {0x2F14, 0x00}, // FS_CAL13
//    {0x2F15, 0x20}, // FS_CAL2
//    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
//    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
//    {0x2F18, 0x28}, // FS_CHP
//    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
//    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
//    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
//    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
////    {0x2F1E, 0x00}, // FS_LBI
//    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
//    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
//    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
//    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
//    {0x2F23, 0x14}, // FS_VCO4
//    {0x2F24, 0x00}, // FS_VCO3
//    {0x2F25, 0x00}, // FS_VCO2
//    {0x2F26, 0x00}, // FS_VCO1
//    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.
//    {0x2F28, 0x00}, // GBIAS6
//    {0x2F29, 0x02}, // GBIAS5
//    {0x2F2A, 0x00}, // GBIAS4
//    {0x2F2B, 0x00}, // GBIAS3
//    {0x2F2C, 0x10}, // GBIAS2
//    {0x2F2D, 0x00}, // GBIAS1
//    {0x2F2E, 0x00}, // GBIAS0
//    {0x2F2F, 0x01}, // IFAMP
//    {0x2F30, 0x01}, // LNA
//    {0x2F31, 0x01}, // RXMIX
//    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
//    {0x2F33, 0xA0}, // XOSC4, Crystal Oscillator Configuration Reg. 4
//    {0x2F34, 0x03}, // XOSC3, Crystal Oscillator Configuration Reg. 3
//    {0x2F35, 0x01}, // XOSC2, Crystal Oscillator Configuration Reg. 2
//    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
////    {0x2F37, 0x00}, // XOSC0, Crystal Oscillator Configuration Reg. 0
//    {0x2F38, 0x00}, // ANALOG_SPARE
//    {0x2F39, 0x00}, //PA_CFG3
//    // The following should be moved to extended register access if used
//    // and require your driver to handle addresses > 0x2F
////    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
//    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
//    // ...
//};


//****************************data rate 50 kbps (4-FSK)***********************//
//const cc1120_reg_setting_t cc1120_config[] = {
//    {0x0000, 0xB0}, // IOCFG3
//    {0x0001, 0x06}, // IOCFG2
//    {0x0002, 0xB0}, // IOCFG1
//    {0x0003, 0x06}, // IOCFG0
//    {0x0004, 0x93}, // SYNC3
//    {0x0005, 0x0B}, // SYNC2
//    {0x0006, 0x51}, // SYNC1
//    {0x0007, 0xDE}, // SYNC0
//    {0x0008, 0x0B}, // SYNC_CFG1
//    {0x0009, 0x17}, // SYNC_CFG0
//    {0x000A, 0x9A}, // DEVIATION_M
//    {0x000B, 0x24}, // MODCFG_DEV_E
//    {0x000C, 0x1C}, // DCFILT_CFG
//    {0x000D, 0x14}, // PREAMBLE_CFG1
//    {0x000E, 0x2A}, // PREAMBLE_CFG0
//    {0x000F, 0x40}, // FREQ_IF_CFG
//    {0x0010, 0x46}, // IQIC
//    {0x0011, 0x04}, // CHAN_BW
//    {0x0012, 0x46}, // MDMCFG1
//    {0x0013, 0x05}, // MDMCFG0
//    {0x0014, 0x89}, // SYMBOL_RATE2
//    {0x0015, 0x99}, // SYMBOL_RATE1
//    {0x0016, 0x9A}, // SYMBOL_RATE0
//    {0x0017, 0x20}, // AGC_REF
//    {0x0018, 0x19}, // AGC_CS_THR
//    {0x0019, 0x00}, // AGC_GAIN_ADJUST
//    {0x001A, 0x91}, // AGC_CFG3
//    {0x001B, 0x20}, // AGC_CFG2
//    {0x001C, 0xA9}, // AGC_CFG1
//    {0x001D, 0xCF}, // AGC_CFG0
//    {0x001E, 0x00}, // FIFO_CFG
//    {0x001F, 0x00}, // DEV_ADDR
//    {0x0020, 0x0B}, // SETTLING_CFG
//    {0x0021, 0x1A}, // FS_CFG
//    {0x0022, 0x08}, // WOR_CFG1
//    {0x0023, 0x21}, // WOR_CFG0
//    {0x0024, 0x00}, // WOR_EVENT0_MSB
//    {0x0025, 0x00}, // WOR_EVENT0_LSB
//    {0x0026, 0x00}, // PKT_CFG2
//    {0x0027, 0x04}, // PKT_CFG1
//    {0x0028, 0x20}, // PKT_CFG0
//    {0x0029, 0x0F}, // RFEND_CFG1
//    {0x002A, 0x00}, // RFEND_CFG0
//    {0x002B, 0x7F}, // PA_CFG2
//    {0x002C, 0x56}, // PA_CFG1
//    {0x002D, 0x7C}, // PA_CFG0
//    {0x2E, 0xFF}, // PKT_LEN
//    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
//    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
//    {0x2F02, 0x0B}, // TOC_CFG
//    {0x2F03, 0x00}, // MARC_SPARE
//    {0x2F04, 0x00}, // ECG_CFG
//    {0x2F05, 0x00}, // CFM_DATA_CFG
//    {0x2F06, 0x01}, // EXT_CTRL
//    {0x2F07, 0x00}, // RCCAL_FINE
//    {0x2F08, 0x00}, // RCCAL_COARSE
//    {0x2F09, 0x00}, // RCCAL_OFFSET
//    {0x2F0A, 0x00}, // FREQOFF1
//    {0x2F0B, 0x00}, // FREQOFF0
//    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
//    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
//    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
//    {0x2F0F, 0x02}, // IF_ADC2
//    {0x2F10, 0xA6}, // IF_ADC1
//    {0x2F11, 0x04}, // IF_ADC0
//    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
//    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
//    {0x2F14, 0x00}, // FS_CAL13
//    {0x2F15, 0x20}, // FS_CAL2
//    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
//    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
//    {0x2F18, 0x28}, // FS_CHP
//    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
//    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
//    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
//    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
////    {0x2F1E, 0x00}, // FS_LBI
//    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
//    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
//    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
//    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
//    {0x2F23, 0x14}, // FS_VCO4
//    {0x2F24, 0x00}, // FS_VCO3
//    {0x2F25, 0x00}, // FS_VCO2
//    {0x2F26, 0x00}, // FS_VCO1
//    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.
//    {0x2F28, 0x00}, // GBIAS6
//    {0x2F29, 0x02}, // GBIAS5
//    {0x2F2A, 0x00}, // GBIAS4
//    {0x2F2B, 0x00}, // GBIAS3
//    {0x2F2C, 0x10}, // GBIAS2
//    {0x2F2D, 0x00}, // GBIAS1
//    {0x2F2E, 0x00}, // GBIAS0
//    {0x2F2F, 0x01}, // IFAMP
//    {0x2F30, 0x01}, // LNA
//    {0x2F31, 0x01}, // RXMIX
//    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
//    {0x2F33, 0xA0}, // XOSC4, Crystal Oscillator Configuration Reg. 4
//    {0x2F34, 0x03}, // XOSC3, Crystal Oscillator Configuration Reg. 3
//    {0x2F35, 0x01}, // XOSC2, Crystal Oscillator Configuration Reg. 2
//    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
////    {0x2F37, 0x00}, // XOSC0, Crystal Oscillator Configuration Reg. 0
//    {0x2F38, 0x00}, // ANALOG_SPARE
//    {0x2F39, 0x00}, //PA_CFG3
//    // The following should be moved to extended register access if used
//    // and require your driver to handle addresses > 0x2F
////    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
//    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
//    // ...
//};

//***************************data rate 60 kbps (4-GFSK)***********************//
const cc1120_reg_setting_t cc1120_config[] = {
    {0x0000, 0xB0}, // IOCFG3
    {0x0001, 0x06}, // IOCFG2
    {0x0002, 0xB0}, // IOCFG1
    {0x0003, 0x06}, // IOCFG0
    {0x0004, 0x93}, // SYNC3
    {0x0005, 0x0B}, // SYNC2
    {0x0006, 0x51}, // SYNC1
    {0x0007, 0xDE}, // SYNC0
    {0x0008, 0x08}, // SYNC_CFG1
    {0x0009, 0x17}, // SYNC_CFG0
    {0x000A, 0xEB}, // DEVIATION_M
    {0x000B, 0x2C}, // MODCFG_DEV_E
    {0x000C, 0x1C}, // DCFILT_CFG
    {0x000D, 0x14}, // PREAMBLE_CFG1
    {0x000E, 0x2A}, // PREAMBLE_CFG0
    {0x000F, 0x40}, // FREQ_IF_CFG
    {0x0010, 0x46}, // IQIC
    {0x0011, 0x42}, // CHAN_BW
    {0x0012, 0x46}, // MDMCFG1
    {0x0013, 0x05}, // MDMCFG0
    {0x0014, 0x8E}, // SYMBOL_RATE2
    {0x0015, 0xB8}, // SYMBOL_RATE1
    {0x0016, 0x52}, // SYMBOL_RATE0
    {0x0017, 0x20}, // AGC_REF
    {0x0018, 0x19}, // AGC_CS_THR
    {0x0019, 0x00}, // AGC_GAIN_ADJUST
    {0x001A, 0x91}, // AGC_CFG3
    {0x001B, 0x20}, // AGC_CFG2
    {0x001C, 0xA9}, // AGC_CFG1
    {0x001D, 0xCF}, // AGC_CFG0
    {0x001E, 0x00}, // FIFO_CFG
    {0x001F, 0x00}, // DEV_ADDR
    {0x0020, 0x0B}, // SETTLING_CFG
    {0x0021, 0x1A}, // FS_CFG
    {0x0022, 0x08}, // WOR_CFG1
    {0x0023, 0x21}, // WOR_CFG0
    {0x0024, 0x00}, // WOR_EVENT0_MSB
    {0x0025, 0x00}, // WOR_EVENT0_LSB
    {0x0026, 0x00}, // PKT_CFG2
    {0x0027, 0x04}, // PKT_CFG1
    {0x0028, 0x20}, // PKT_CFG0
    {0x0029, 0x0F}, // RFEND_CFG1
    {0x002A, 0x00}, // RFEND_CFG0
    {0x002B, 0x7F}, // PA_CFG2
    {0x002C, 0x56}, // PA_CFG1
    {0x002D, 0x7C}, // PA_CFG0
    {0x2E, 0xFF}, // PKT_LEN
    {0x2F00, 0x00}, // IF_MIX_CFG, IF Mix Configuration
    {0x2F01, 0x22}, // FREQOFF_CFG, Frequency Offset Correction Configuration
    {0x2F02, 0x0B}, // TOC_CFG
    {0x2F03, 0x00}, // MARC_SPARE
    {0x2F04, 0x00}, // ECG_CFG
    {0x2F05, 0x00}, // CFM_DATA_CFG
    {0x2F06, 0x01}, // EXT_CTRL
    {0x2F07, 0x00}, // RCCAL_FINE
    {0x2F08, 0x00}, // RCCAL_COARSE
    {0x2F09, 0x00}, // RCCAL_OFFSET
    {0x2F0A, 0x00}, // FREQOFF1
    {0x2F0B, 0x00}, // FREQOFF0
    {0x2F0C, 0x69}, // FREQ2, Frequency Configuration [23:16]
    {0x2F0D, 0xA0}, // FREQ1, Frequency Configuration [15:8]
    {0x2F0E, 0x00}, // FREQ0, Frequency Configuration [7:0]
    {0x2F0F, 0x02}, // IF_ADC2
    {0x2F10, 0xA6}, // IF_ADC1
    {0x2F11, 0x04}, // IF_ADC0
    {0x2F12, 0x00}, // FS_DIG1, Frequency Synthesizer Digital Reg. 1
    {0x2F13, 0x5F}, // FS_DIG0, Frequency Synthesizer Digital Reg. 0
    {0x2F14, 0x00}, // FS_CAL13
    {0x2F15, 0x20}, // FS_CAL2
    {0x2F16, 0x40}, // FS_CAL1, Frequency Synthesizer Calibration Reg. 1
    {0x2F17, 0x0E}, // FS_CAL0, Frequency Synthesizer Calibration Reg. 0
    {0x2F18, 0x28}, // FS_CHP
    {0x2F19, 0x03}, // FS_DIVTWO, Frequency Synthesizer Divide by 2
    {0x2F1A, 0x00}, // FS_DSM1, FS Digital Synthesizer Module Configuration Reg. 1
    {0x2F1B, 0x33}, // FS_DSM0, FS Digital Synthesizer Module Configuration Reg. 0
    {0x2F1C, 0xFF}, // FS_DVC1, Frequency Synthesizer Divider Chain Configuration 
    {0x2F1D, 0x17}, // FS_DVC0, Frequency Synthesizer Divider Chain Configuration 
//    {0x2F1E, 0x00}, // FS_LBI
    {0x2F1F, 0x50}, // FS_PFD, Frequency Synthesizer Phase Frequency Detector Configu
    {0x2F20, 0x6E}, // FS_PRE, Frequency Synthesizer Prescaler Configuration
    {0x2F21, 0x14}, // FS_REG_DIV_CML, Frequency Synthesizer Divider Regulator Config
    {0x2F22, 0xAC}, // FS_SPARE, Frequency Synthesizer Spare
    {0x2F23, 0x14}, // FS_VCO4
    {0x2F24, 0x00}, // FS_VCO3
    {0x2F25, 0x00}, // FS_VCO2
    {0x2F26, 0x00}, // FS_VCO1
    {0x2F27, 0xB4}, // FS_VCO0, FS Voltage Controlled Oscillator Configuration Reg.
    {0x2F28, 0x00}, // GBIAS6
    {0x2F29, 0x02}, // GBIAS5
    {0x2F2A, 0x00}, // GBIAS4
    {0x2F2B, 0x00}, // GBIAS3
    {0x2F2C, 0x10}, // GBIAS2
    {0x2F2D, 0x00}, // GBIAS1
    {0x2F2E, 0x00}, // GBIAS0
    {0x2F2F, 0x01}, // IFAMP
    {0x2F30, 0x01}, // LNA
    {0x2F31, 0x01}, // RXMIX
    {0x2F32, 0x0E}, // XOSC5, Crystal Oscillator Configuration Reg. 5
    {0x2F33, 0xA0}, // XOSC4, Crystal Oscillator Configuration Reg. 4
    {0x2F34, 0x03}, // XOSC3, Crystal Oscillator Configuration Reg. 3
    {0x2F35, 0x01}, // XOSC2, Crystal Oscillator Configuration Reg. 2
    {0x2F36, 0x03}, // XOSC1, Crystal Oscillator Configuration Reg. 1
//    {0x2F37, 0x00}, // XOSC0, Crystal Oscillator Configuration Reg. 0
    {0x2F38, 0x00}, // ANALOG_SPARE
    {0x2F39, 0x00}, //PA_CFG3
    // The following should be moved to extended register access if used
    // and require your driver to handle addresses > 0x2F
//    {0x30, 0x05}, // PKT_LEN - Packet length (for fixed length packets)
    // {0x31, 0x00}, // IF_MIX_CFG (extended register)
    // ...
};

const uint16_t cc1120_config_size = sizeof(cc1120_config) / sizeof(cc1120_config[0]);
