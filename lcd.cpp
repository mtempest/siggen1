#include <avr/io.h>

#include "lcd.h"

LCD5110 myGLCD;

void LCD_init(void)
{
  /* Set MOSI and SCK output */
  DDRB |= (1<<PB3)|(1<<PB5);

  /* Enable SPI, Master, CPOL=1, CPHA=1 */
  /* Also: MSB first, clock=fosc/4 */
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA) | 3;

  /* Set the pins for DC, RST and CS to output */
  DDRD |= (1<<PD7) | (1<<PD6) | (1<<PD5);

  myGLCD.InitLCD(0x46);
}
