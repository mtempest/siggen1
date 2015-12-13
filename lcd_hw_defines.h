/* Derived from LCD5110_Graph/hardware/avr/HW_AVR_defines.h */
// *** Hardwarespecific defines ***
#define cbi(reg, bitmask) *reg &= ~bitmask
#define sbi(reg, bitmask) *reg |= bitmask
//#define pulseClock cbi(P_SCK, B_SCK); asm ("nop"); sbi(P_SCK, B_SCK)
#define resetLCD sbi(P_DC, B_DC); /*sbi(P_MOSI, B_MOSI); sbi(P_SCK, B_SCK);*/ sbi(P_CS, B_CS); cbi(P_RST, B_RST); _delay_us(10); sbi(P_RST, B_RST)

#define fontbyte(x) pgm_read_byte(&cfont.font[x])  
#define bitmapbyte(x) pgm_read_byte(&bitmap[x])  

#define regtype volatile uint8_t
#define regsize volatile uint8_t
#define bitmapdatatype uint8_t*

#define byte uint8_t
#define boolean uint8_t

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
