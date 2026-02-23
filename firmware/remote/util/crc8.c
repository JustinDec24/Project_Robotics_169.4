/*============================================================================
 * util/crc8.c — CRC-8/SMBUS implementation
 *===========================================================================*/
#include "crc8.h"

uint8_t crc8_update(uint8_t crc, uint8_t byte_val)
{
    crc ^= byte_val;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x80)
            crc = (uint8_t)((crc << 1) ^ 0x07);
        else
            crc = (uint8_t)(crc << 1);
    }
    return crc;
}

uint8_t crc8_calc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++)
        crc = crc8_update(crc, data[i]);
    return crc;
}
