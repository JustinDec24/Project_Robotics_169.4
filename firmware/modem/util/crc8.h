/*============================================================================
 * util/crc8.h — CRC-8 for UART framing integrity
 *
 * Polynomial: 0x07 (CRC-8/SMBUS), init = 0x00.
 * Used ONLY for the PC <-> Remote UART link.
 * RF CRC is handled by the CC1120 hardware.
 *===========================================================================*/
#ifndef CRC8_H
#define CRC8_H

#include <stdint.h>
#include <stddef.h>

uint8_t crc8_calc(const uint8_t *data, size_t len);
uint8_t crc8_update(uint8_t crc, uint8_t byte_val);

#endif /* CRC8_H */
