 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
*/

/*
© [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

// rf comm test RX
//
#include "mcc_generated_files/system/system.h"
#include "cc1120.h"
//
void print_hex(uint8_t val) {
    EUSART1_Write((val >> 4) > 9 ? ('A' + ((val >> 4) - 10)) : ('0' + (val >> 4)));
    __delay_ms(10);
    //while (!EUSART1_IsTxDone());
    EUSART1_Write((val & 0xF) > 9 ? ('A' + ((val & 0xF) - 10)) : ('0' + (val & 0xF)));
    __delay_ms(10);
    //while (!EUSART1_IsTxDone());
    EUSART1_Write(' ');
    //while (!EUSART1_IsTxDone());
}

//********************************TX*******************************//
uint8_t getRandomByte(void) {
    static uint16_t seed = 0xACE1;  // Any non-zero seed

    // Simple 16-bit LFSR or LCG
    seed = (seed * 1103515245 + 12345) & 0xFFFF;  // 16-bit version
    return (uint8_t)(seed & 0xFF);
}

static uint8_t txBuffer[125]; 

void fillRandomMessage(void) {
    for (uint16_t i = 0; i < sizeof(txBuffer); i++) {
        txBuffer[i] = getRandomByte();
    }
}

void printRandomMessage(void) {
    EUSART1_Write('\n');
    __delay_ms(5);
    EUSART1_Write('\r');
    __delay_ms(5);
    for (uint16_t i = 0; i < sizeof(txBuffer); i++) {
        print_hex(txBuffer[i]);
        EUSART1_Write(' ');
    }
    EUSART1_Write('\n');
    __delay_ms(5);
    EUSART1_Write('\r');
    __delay_ms(5);
}

//void onGDO2Interrupt(void) // test for send interrupt but doesn't work
//{
//    EUSART1_Write('S');
//    __delay_ms(10);
//}

void main(void) {
    SYSTEM_Initialize();

    RESET_SetLow();
    __delay_ms(1000);
    RESET_SetHigh();
    __delay_ms(1000);
    
    //IO_RC3_SetInterruptHandler(onGDO2Interrupt);

    SPI1_Open(HOST_CONFIG);
    CSN_SetHigh();
    __delay_ms(100);

    CC1120_Strobe(CC1120_SRES);
   __delay_ms(10);
    CC1120_Strobe(CC1120_SIDLE);
    __delay_ms(10);
    CC1120_ApplyConfig();
    __delay_ms(10);
    CC1120_Strobe(CC1120_SCAL);
    __delay_ms(10);
    CC1120_Strobe(CC1120_SFTX);
    __delay_ms(10);

    EUSART1_Write('S'); // Debug marker

    while (1) {

        // Flush TX FIFO just to be safe
        CC1120_Strobe(CC1120_SFTX);
        __delay_ms(10);
        fillRandomMessage();
        printRandomMessage();

        // Write packet to TX FIFO
        CC1120_WriteTXFIFO(txBuffer, sizeof (txBuffer));
        uint8_t txBytes = CC1120_ReadReg(0x2FD6);

        // Strobe STX to start transmission
        CC1120_Strobe(CC1120_STX);

        // Wait for TX to complete: check MARCSTATE or TXBYTES == 0
        uint8_t marcState = 0;
        uint8_t marcState_prev = 0;
        do {
            marcState_prev = marcState;
            marcState = CC1120_ReadReg(0x2F73) & 0x1F; // MARCSTATE (bits 4:0)
            __delay_ms(2000);//message sent entirely even with low data rate 
            txBytes = CC1120_ReadReg(0x2FD6);
        } while (marcState == 0x13); // MARCSTATE == TX

        EUSART1_Write('M');
        __delay_ms(10);
        print_hex(marcState_prev);
        __delay_ms(10);
        // Optionally blink LED or send UART marker
        EUSART1_Write('.');
        __delay_ms(10);
        CC1120_Strobe(CC1120_SIDLE);
        __delay_ms(2000);
    }
}


//**********************RX*********************************//
//
//static uint8_t rxBuffer[255]; 
////interrupt on the descending edge when the packet has been completely received
//void onGDO0Interrupt(void) 
//{
//    __delay_ms(10);
//    uint8_t length = 0;
//
//    CC1120_ReadRXFIFO(&length, 1);
//
//
//        EUSART1_Write('L'); // Length
//        print_hex(length);
//        __delay_ms(5);
//
//        CC1120_ReadRXFIFO(rxBuffer, length); // Read packet
//
//        //EUSART1_Write('R'); // Received!
//        for (int i = 0; i < length; i++)
//            print_hex(rxBuffer[i]);  //UART debug print
//         
//    EUSART1_Write('\n');
//    __delay_ms(5);
//    EUSART1_Write('\r');
//    __delay_ms(5);
//    // Flush FIFO and re-enter RX
//    CC1120_Strobe(CC1120_SIDLE);
//    __delay_ms(5);
//    CC1120_Strobe(CC1120_SFRX);
//    __delay_ms(5);
//    CC1120_Strobe(CC1120_SRX);
//    __delay_ms(5);    
//}
//
//
//// RX test
//void main(void)
//{
//    SYSTEM_Initialize();
//
//    RESET_SetLow(); __delay_ms(1000);
//    RESET_SetHigh(); __delay_ms(1000);
//    
//    IO_RC4_SetInterruptHandler(onGDO0Interrupt);
//
//    INTERRUPT_GlobalInterruptEnable();
//    INTERRUPT_PeripheralInterruptEnable();
//
//
//    SPI1_Open(HOST_CONFIG);
//    CSN_SetHigh(); __delay_ms(100);
//
//    CC1120_Strobe(CC1120_SRES);    __delay_ms(10);
//    CC1120_Strobe(CC1120_SIDLE);   __delay_ms(10);
//    CC1120_ApplyConfig();          __delay_ms(10);  // Same config as TX
//    CC1120_Strobe(CC1120_SCAL);    __delay_ms(10);
//    CC1120_Strobe(CC1120_SFTX);    __delay_ms(10);  // Flush TX FIFO
//    CC1120_Strobe(CC1120_SFRX);    __delay_ms(10);  // Flush RX FIFO
//    CC1120_Strobe(CC1120_SRX);     __delay_ms(10);  // Enter RX mode
//
//    uint8_t rxBuffer[8];
//    uint8_t rxBytes = 0;
//
//    while (1);
//}



