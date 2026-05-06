/*============================================================================
 * drivers/spi.c — SPI master driver implementation
 *===========================================================================*/
#include "spi.h"
#include "../board/board.h"

void spi_init(void)
{
    board_spi_hw_init();
}

uint8_t spi_transfer_byte(uint8_t tx_byte)
{
    return board_spi_exchange(tx_byte);
}

void spi_transfer(const uint8_t *tx, uint8_t *rx, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint8_t out = tx ? tx[i] : 0x00;
        uint8_t in  = board_spi_exchange(out);
        if (rx)
            rx[i] = in;
    }
}
