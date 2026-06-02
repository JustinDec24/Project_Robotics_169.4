/* 
 * File:   cc1120.h
 * Author: ingek
 *
 * Created on 24. mai 2025, 23:52
 */

#ifndef CC1120_H
#define	CC1120_H

#include <stdint.h>


// ---- Command Flags ----
#define CC1120_READ    0x80
#define CC1120_WRITE   0x00
#define CC1120_BURST   0x40
#define CC1120_EXT_ADDR 0x2F

// --- CC1120 Strobe Commands ---
#define CC1120_SRES   0x30
#define CC1120_SFSTXON 0x31
#define CC1120_SXOFF  0x32
#define CC1120_SCAL   0x33
#define CC1120_SRX    0x34
#define CC1120_STX    0x35
#define CC1120_SIDLE  0x36
#define CC1120_SAFC   0x37
#define CC1120_SWOR   0x38
#define CC1120_SPWD   0x39
#define CC1120_SFRX   0x3A
#define CC1120_SFTX   0x3B
#define CC1120_SWORRST 0x3C
#define CC1120_SNOP   0x3D

#define CC1120_WRITE_BURST  0x40
#define CC1120_READ_BURST   0xC0

#define CC1120_BURST_TXFIFO   0x3F
#define CC1120_BURST_RXFIFO   0x3F




// ---- Public API ----
void CC1120_Reset(void);
uint8_t CC1120_ReadReg(uint16_t addr);
void CC1120_WriteReg(uint16_t addr, uint8_t value);
void CC1120_Strobe(uint8_t command);
void CC1120_ApplyConfig(void);
void CC1120_WriteBurst(uint8_t addr, const uint8_t *buffer, uint8_t length);
void CC1120_ReadRXFIFO(uint8_t *buffer, uint8_t length);
uint8_t CC1120_WriteTXFIFO(const uint8_t* data, uint8_t length);



#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* CC1120_H */

