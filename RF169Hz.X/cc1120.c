#include "cc1120.h"
#include "mcc_generated_files/system/system.h"  // For SPI1 and pin macros
#include "cc1120_config.h"


void CC1120_Reset(void)
{
    RESET_SetLow();
    __delay_ms(1);
    RESET_SetHigh();
    __delay_ms(5);
}

//uint8_t CC1120_ReadReg(uint16_t addr)
//{
//    uint8_t result;
//
//    CSN_SetLow();
//    while (!SPI1_IsTxReady());
//    SPI1_ByteExchange(CC1120_READ | CC1120_EXT_ADDR); 
//    SPI1_ByteExchange((uint8_t)(addr & 0xFF));     
//    result = SPI1_ByteExchange(0x00);
//    CSN_SetHigh();
//
//    return result;
//}

uint8_t CC1120_ReadReg(uint16_t addr)
{
    uint8_t result;
    uint16_t timeout;

    CSN_SetLow();

    if (addr <= 0x2F) {
        // Standard register read
        SPI1_ByteExchange(CC1120_READ | (uint8_t)(addr & 0x3F));
        result = SPI1_ByteExchange(0x00);
    } else {
        // Extended register read
        SPI1_ByteExchange(0x2F | 0x80);             // READ + EXTENDED_ACCESS
        SPI1_ByteExchange((uint8_t)(addr & 0xFF));  // LSB of register
        result = SPI1_ByteExchange(0x00);           // Dummy write to read value
    }

    CSN_SetHigh();
    return result;
}

void CC1120_WriteReg(uint16_t addr, uint8_t value)
{
    CSN_SetLow();

    if (addr <= 0x2F) {
        // Standard register write
        SPI1_ByteExchange(CC1120_WRITE | (uint8_t)(addr & 0x3F));
        SPI1_ByteExchange(value);
    } else {
        // Extended register write
        SPI1_ByteExchange(0x2F | 0x00);             // WRITE + EXTENDED_ACCESS
        SPI1_ByteExchange((uint8_t)(addr & 0xFF));  // LSB of register
        SPI1_ByteExchange(value);
    }

    CSN_SetHigh();
}

//uint8_t CC1120_ReadReg(uint16_t addr)
//{
//    uint8_t result;
//    uint8_t header;
//    uint16_t timeout;
//
//    if (addr > 0x2F) {
//        // Extended address
//        header = CC1120_READ | CC1120_EXT_ADDR;
//    } else {
//        // Standard address
//        header = CC1120_READ | (uint8_t)addr;
//    }
//
//    CSN_SetLow();
//
//    timeout = 1000;
//    while (!SPI1_IsTxReady() && --timeout);
//    if (timeout == 0) {
//        EUSART1_Write('1');
//        CSN_SetHigh();
//        return 0xFF;
//    }
//    SPI1_ByteExchange(header);
//
//    if (addr > 0x2F) {
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('2');
//            CSN_SetHigh();
//            return 0xFF;
//        }
//        SPI1_ByteExchange((uint8_t)(addr & 0xFF));
//    }
//
//    timeout = 1000;
//    while (!SPI1_IsTxReady() && --timeout);
//    if (timeout == 0) {
//        EUSART1_Write('3');
//        CSN_SetHigh();
//        return 0xFF;
//    }
//    result = SPI1_ByteExchange(0x00);
//
//    CSN_SetHigh();
//    return result;
//}

//void CC1120_WriteReg(uint16_t addr, uint8_t value)
//{
//    uint16_t timeout;
//
//    CSN_SetLow();
//
//    if (addr <= 0x2F) {
//        // Standard register write
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('W'); EUSART1_Write('1');
//            CSN_SetHigh();
//            return;
//        }
//        SPI1_ByteExchange(CC1120_WRITE | (addr & 0x3F));
//
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('W'); EUSART1_Write('2');
//            CSN_SetHigh();
//            return;
//        }
//        SPI1_ByteExchange(value);
//    } else {
//        // Extended register write
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('W'); EUSART1_Write('3');
//            CSN_SetHigh();
//            return;
//        }
//        SPI1_ByteExchange(CC1120_WRITE | CC1120_EXT_ADDR);
//
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('W'); EUSART1_Write('4');
//            CSN_SetHigh();
//            return;
//        }
//        SPI1_ByteExchange((uint8_t)(addr & 0xFF));
//
//        timeout = 1000;
//        while (!SPI1_IsTxReady() && --timeout);
//        if (timeout == 0) {
//            EUSART1_Write('W'); EUSART1_Write('5');
//            CSN_SetHigh();
//            return;
//        }
//        SPI1_ByteExchange(value);
//    }
//
//    CSN_SetHigh();
//}

void CC1120_Strobe(uint8_t command)
{
    CSN_SetLow();
    __delay_us(10);
    SPI1_ByteExchange(command);
    __delay_us(10);
    CSN_SetHigh();
}

void CC1120_ApplyConfig(void)
{
    for (uint16_t i = 0; i < cc1120_config_size; ++i)
    {
        CC1120_WriteReg(cc1120_config[i].addr, cc1120_config[i].value);
        __delay_us(100);  // Optional short delay
    }
}

void CC1120_WriteBurst(uint8_t addr, const uint8_t *buffer, uint8_t length)
{
    CSN_SetLow();
    __delay_ms(10);
    while (!SPI1_IsTxReady());
    SPI1_ByteExchange(CC1120_WRITE_BURST | addr);

    for (uint8_t i = 0; i < length; ++i) {
        while (!SPI1_IsTxReady());
        SPI1_ByteExchange(buffer[i]);
    }
    __delay_ms(10);
    CSN_SetHigh();
}

void CC1120_ReadRXFIFO(uint8_t *buffer, uint8_t length)
{
    CSN_SetLow();
    while (!SPI1_IsTxReady());
    SPI1_ByteExchange(CC1120_READ_BURST | CC1120_BURST_RXFIFO);

    for (uint8_t i = 0; i < length; ++i) {
        while (!SPI1_IsTxReady());
        buffer[i] = SPI1_ByteExchange(0x00);
    }

    CSN_SetHigh();
}

uint8_t CC1120_WriteTXFIFO(const uint8_t* data, uint8_t length)
{
    CSN_SetLow();
    __delay_us(1);

    // Send TX FIFO burst write command (0x7F)
    uint8_t status = SPI1_ByteExchange(CC1120_WRITE_BURST | CC1120_BURST_TXFIFO);

    // First byte = length
    SPI1_ByteExchange(length);

    // Write the payload bytes (length bytes)
    for (uint8_t i = 0; i < length; i++) {
        SPI1_ByteExchange(data[i]);
    }

    __delay_us(1);
    CSN_SetHigh();

    return status;
}



