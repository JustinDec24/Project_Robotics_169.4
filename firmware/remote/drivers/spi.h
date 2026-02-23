/*============================================================================
 * drivers/spi.h — SPI master driver
 *
 * Provides single-byte and multi-byte full-duplex transfers.
 * Chip-select is NOT managed here — caller (cc1120.c) handles CS.
 *===========================================================================*/
#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stddef.h>

void    spi_init(void);
uint8_t spi_transfer_byte(uint8_t tx_byte);
void    spi_transfer(const uint8_t *tx, uint8_t *rx, size_t n);

#endif /* SPI_H */
