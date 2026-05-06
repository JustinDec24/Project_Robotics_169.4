/*============================================================================
 * blink_test/main.c — Smoke test for the PIC18F26Q10-I/SS board.
 *
 * What it does:
 *   - Configures HFINTOSC at 1 MHz (default after reset, kept for simplicity).
 *   - Sets RC2 as a digital output and toggles it ~1 Hz.
 *   - If the LED on RC2 blinks at ~1 Hz, your toolchain, programmer wiring,
 *     power supply and LED pin are all OK -> you can move on to the real
 *     firmware.
 *
 * Build / flash:
 *   1. MPLAB X -> File -> New Project
 *      -> Microchip Embedded -> Standalone Project
 *      -> Family: All Families, Device: PIC18F26Q10
 *      -> Tool: PICkit 4 (must be plugged in USB)
 *      -> Compiler: XC8 (latest)
 *      -> Project name: blink_test
 *
 *   2. Right-click "Source Files" -> Add Existing Item -> select THIS file.
 *
 *   3. Right-click the project -> Properties:
 *        - Conf: [default] -> PICkit 4 -> Power:
 *            UNCHECK "Power target circuit from PICkit 4"
 *            (your board is already USB-bus-powered through the FT231).
 *        - Conf: [default] -> PICkit 4 -> Program Options:
 *            "Use low voltage programming entry" -> ON
 *            (matches the LVP=ON config bit below).
 *
 *   4. Hit the "Make and Program Device" button (green-arrow icon).
 *      The PICkit 4 needs to see: VDD, GND, MCLR (+ 10k pull-up), ICSPCLK
 *      (RB6) and ICSPDAT (RB7) — those are already wired on your J1.
 *
 * Expected: LED on RC2 blinks at ~1 Hz. Approximate cadence: HFINTOSC at
 * 1 MHz means __delay_ms(500) blocks for 500 ms, so 1 s period.
 *===========================================================================*/

#include <xc.h>

/* The board has no PIC crystal; FEXTOSC is OFF, RSTOSC = HFINTOSC at 1 MHz.
 * No power-on switch needed: we run on the reset clock. */
#pragma config FEXTOSC = OFF
#pragma config RSTOSC  = HFINTOSC_1MHZ

#pragma config CLKOUTEN = OFF
#pragma config CSWEN    = ON
#pragma config FCMEN    = OFF

#pragma config MCLRE    = EXTMCLR
#pragma config PWRTE    = ON
#pragma config LPBOREN  = OFF
#pragma config BOREN    = SBORDIS

#pragma config BORV     = VBOR_245
#pragma config ZCD      = OFF
#pragma config PPS1WAY  = OFF
#pragma config STVREN   = ON
#pragma config XINST    = OFF

#pragma config WDTCPS   = WDTCPS_31
#pragma config WDTE     = OFF
#pragma config WDTCWS   = WDTCWS_7
#pragma config WDTCCS   = SC

#pragma config WRT0  = OFF
#pragma config WRT1  = OFF
#pragma config WRT2  = OFF
#pragma config WRT3  = OFF
#pragma config WRTC  = OFF
#pragma config WRTB  = OFF
#pragma config WRTD  = OFF
#pragma config SCANE = ON
#pragma config LVP   = ON           /* allow LVP entry from PICkit 4 */

#pragma config CP    = OFF
#pragma config CPD   = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF

/* Required by __delay_ms() / __delay_us(). 1 MHz HFINTOSC after reset. */
#define _XTAL_FREQ  1000000UL

void main(void)
{
    /* --- Configure RC2 as a digital push-pull output ------------------- */
    ANSELCbits.ANSELC2 = 0;     /* digital, not analog */
    TRISCbits.TRISC2   = 0;     /* output */
    LATCbits.LATC2     = 0;     /* start LOW (LED off) */

    for (;;) {
        LATCbits.LATC2 ^= 1;    /* toggle LED */
        __delay_ms(500);        /* ~0.5 s on / 0.5 s off -> 1 Hz blink */
    }
}
