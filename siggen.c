#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "ui.h"
#include "out.h"

int main(void)
{
  /* Configure all pins as inputs and enable pull-up resistors */
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
  PORTB = 0xFF;
  PORTC = 0xFF;
  PORTD = 0xFF;

  DDRB = 1;

  UI_init();
  OUT_init();
  while (1)
  {
    PORTB = 0xFF;
    _delay_ms(300);
    PORTB = 0xFE;
    _delay_ms(300);

    UI_cyclic();
    OUT_cyclic();
  }
}
