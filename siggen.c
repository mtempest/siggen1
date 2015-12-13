#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "ui.h"
#include "out.h"
#include "store.h"

int main(void)
{
  /* Configure all pins as inputs and enable pull-up resistors */
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
  PORTB = 0xFF;
  PORTC = 0xFF;
  PORTD = 0xFF;

  UI_init();
  OUT_init();
  STORE_init();

  sei();

  while (1)
  {
    UI_cyclic();
    OUT_cyclic();
    STORE_cyclic();
  }
}
