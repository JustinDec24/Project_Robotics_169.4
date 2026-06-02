/**
 * Generated Driver File
 * 
 * @file pins.c
 * 
 * @ingroup  pinsdriver
 * 
 * @brief This is generated driver implementation for pins. 
 *        This file provides implementations for pin APIs for all pins selected in the GUI.
 *
 * @version Driver Version 3.0.0
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

#include "../pins.h"

void (*IO_RC3_InterruptHandler)(void);
void (*IO_RC4_InterruptHandler)(void);

void PIN_MANAGER_Initialize(void)
{
   /**
    LATx registers
    */
    LATA = 0x0;
    LATC = 0x0;

    /**
    TRISx registers
    */
    TRISA = 0x26;
    TRISC = 0x1C;

    /**
    ANSELx registers
    */
    ANSELA = 0x4;
    ANSELC = 0x0;

    /**
    WPUx registers
    */
    WPUA = 0x0;
    WPUC = 0x0;
  
    /**
    ODx registers
    */
   
    ODCONA = 0x0;
    ODCONC = 0x0;
    /**
    SLRCONx registers
    */
    SLRCONA = 0x3F;
    SLRCONC = 0x3F;
    /**
    INLVLx registers
    */
    INLVLA = 0x3F;
    INLVLC = 0x3F;

    /**
    PPS registers
    */
    SSP1DATPPS = 0x12; //RC2->MSSP1:SDI1;
    RX1PPS = 0x1; //RA1->EUSART1:RX1;
    RC1PPS = 0x1C;  //RC1->MSSP1:SDO1;
    RA0PPS = 0x13;  //RA0->EUSART1:TX1;
    SSP1CLKPPS = 0x10;  //RC0->MSSP1:SCK1;
    RC0PPS = 0x1B;  //RC0->MSSP1:SCK1;

    /**
    APFCON registers
    */

   /**
    IOCx registers 
    */
    IOCAP = 0x0;
    IOCAN = 0x0;
    IOCAF = 0x0;
    IOCCP = 0x8;
    IOCCN = 0x10;
    IOCCF = 0x0;

    IO_RC3_SetInterruptHandler(IO_RC3_DefaultInterruptHandler);
    IO_RC4_SetInterruptHandler(IO_RC4_DefaultInterruptHandler);

    // Enable PIE0bits.IOCIE interrupt 
    PIE0bits.IOCIE = 1; 
}
  
void PIN_MANAGER_IOC(void)
{
    // interrupt on change for pin IO_RC3}
    if(IOCCFbits.IOCCF3 == 1)
    {
        IO_RC3_ISR();  
    }
    // interrupt on change for pin IO_RC4}
    if(IOCCFbits.IOCCF4 == 1)
    {
        IO_RC4_ISR();  
    }
}
   
/**
   IO_RC3 Interrupt Service Routine
*/
void IO_RC3_ISR(void) {

    // Add custom IOCCF3 code

    // Call the interrupt handler for the callback registered at runtime
    if(IO_RC3_InterruptHandler)
    {
        IO_RC3_InterruptHandler();
    }
    IOCCFbits.IOCCF3 = 0;
}

/**
  Allows selecting an interrupt handler for IOCCF3 at application runtime
*/
void IO_RC3_SetInterruptHandler(void (* InterruptHandler)(void)){
    IO_RC3_InterruptHandler = InterruptHandler;
}

/**
  Default interrupt handler for IOCCF3
*/
void IO_RC3_DefaultInterruptHandler(void){
    // add your IO_RC3 interrupt custom code
    // or set custom function using IO_RC3_SetInterruptHandler()
}
   
/**
   IO_RC4 Interrupt Service Routine
*/
void IO_RC4_ISR(void) {

    // Add custom IOCCF4 code

    // Call the interrupt handler for the callback registered at runtime
    if(IO_RC4_InterruptHandler)
    {
        IO_RC4_InterruptHandler();
    }
    IOCCFbits.IOCCF4 = 0;
}

/**
  Allows selecting an interrupt handler for IOCCF4 at application runtime
*/
void IO_RC4_SetInterruptHandler(void (* InterruptHandler)(void)){
    IO_RC4_InterruptHandler = InterruptHandler;
}

/**
  Default interrupt handler for IOCCF4
*/
void IO_RC4_DefaultInterruptHandler(void){
    // add your IO_RC4 interrupt custom code
    // or set custom function using IO_RC4_SetInterruptHandler()
}
/**
 End of File
*/