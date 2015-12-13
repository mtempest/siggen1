#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "LCD5110_Graph.h"

#include "fonts.h"

#include "ui.h"
#include "out.h"
#include "lcd.h"

#define LINE_1 0
#define LINE_2 24
#define LINE_3 32
#define LINE_4 40

enum
{
  PARAM_FIRST,

  PARAM_FREQUENCY = PARAM_FIRST,
  PARAM_PERIOD,
  PARAM_SCALE,
  PARAM_WAVEFORM,
  PARAM_DUTY_CYCLE,
  PARAM_AMPLITUDE,
  PARAM_CONTRAST,
  PARAM_FINE_CALIBRATE,
  PARAM_OSCCAL,

  PARAM_LAST
};
static int selected_param;


static void show_freq(void);
static void show_units(void);
static void show_on_off(void);
static void show_waveform(void);
static void show_parameter(void);

static void cat_int8(char*s, int8_t n);
static void cat_uint8(char*s, uint8_t n);
static uint8_t cat_uint32(char*s, uint32_t n, int8_t chars);

void UI_init(void)
{
  LCD_init();

}

void UI_cyclic(void)
{
  myGLCD.setFont(SmallFont);
  show_units();
  show_on_off();
  show_waveform();
  show_parameter();

  myGLCD.setFont(BigNumbers);
  show_freq();

  myGLCD.update();
  
  selected_param++;
}

static void show_units(void)
{
  //TODO get the units
  //TODO put the strings in progmem
  const char* s;
  if (1)
  {
    s = PSTR("kHz");
  }
  else
  {
    s = PSTR(" Hz");
  }

  myGLCD.print_P(s, RIGHT, LINE_2);
}

static void show_on_off(void)
{
  //TODO get the on/off state
  //TODO put the strings in progmem
  const char* s;
  if (1)
  {
    s = PSTR(" ON ");
  }
  else
  {
    s = PSTR("off");
  }

  myGLCD.print_P(s, CENTER, LINE_3);
}

static void show_waveform(void)
{
  //TODO get the waveform state
  //TODO put the strings in progmem
  const char* s;
  if (1)
  {
    s = PSTR("triangle");
  }
  else if (0)
  {
    s = PSTR("sine    ");
  }
  else
  {
    s = PSTR("square  ");
  }

  myGLCD.print_P(s, 0, LINE_2);
}

static void show_freq(void)
{
  //TODO Get the frequency
  myGLCD.print("123.45", CENTER, LINE_1);
}

static void show_parameter(void)
{
  static char s[15];
  static const char percent[] PROGMEM = "%";
  uint8_t unit_steps;
  s[0] = '\0';

  switch (selected_param)
  {
  case PARAM_FREQUENCY:
    strcpy_P(s, PSTR("Frequency"));
    // The value is always shown on the first line
    // so it is not appended here
    break;

  case PARAM_PERIOD:
    // The period is a 32-bit value in nanoseconds
    strcpy_P(s, PSTR("T(ns):"));
    unit_steps = cat_uint32(s, 12345678, 8);
    switch (unit_steps)
    {
    case 0: break;
    case 1: s[2] = 'u'; break; //TODO: use a mu symbol
    case 2: s[2] = 'm'; break;
    case 3: s[2] = ' '; break;
    default:s[2] = '?'; break;
    }
    break;

  case PARAM_SCALE:
    break;

  case PARAM_WAVEFORM:
    strcpy_P(s, PSTR("Waveform"));
    // Waveform is always displayed on line 2
    // so no need to repeat it here
    break;

  case PARAM_DUTY_CYCLE:
    strcpy_P(s, PSTR("Duty-cycle:"));
    cat_uint8(s, 50);
    strcat_P(s, percent);
    break;

  case PARAM_AMPLITUDE:
    strcpy_P(s, PSTR("Amplitude:"));
    cat_uint8(s, 100);
    strcat_P(s, percent);
    break;

  case PARAM_CONTRAST:
    strcpy_P(s, PSTR("Contrast:"));
    cat_uint8(s, 123);
    break;

  case PARAM_FINE_CALIBRATE:
    strcpy_P(s, PSTR("Fine cal:"));
    cat_int8(s, 123);
    break;

  case PARAM_OSCCAL:
    strcpy_P(s, PSTR("OSCCAL:"));
    cat_uint8(s, 123);
    break;

  default:
    selected_param = PARAM_FREQUENCY;
    break;
  }
  //TODO Get the real parameter and value
  myGLCD.print_P(PSTR("              "), 0, LINE_4);
  myGLCD.print(s, CENTER, LINE_4);
}

static void cat_int8(char*s, int8_t n)
{
  uint8_t u;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  // Convert to unsigned, appending minus sign if needed
  if (n < 0)
  {
    *s = '-';
    s++;
    *s = 0;
    u = (uint8_t)(-n);
  }
  else
  {
    u = (uint8_t)n;
  }

  // append the digits
  cat_uint8(s, u);
}

static void cat_uint8(char*s, uint8_t n)
{
  uint8_t num_digits;
  uint8_t i;
  uint8_t next_n;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  // Work out the number of digits
  if (n > 99)
  {
    num_digits = 3;
  }
  else if (n > 9)
  {
    num_digits = 2;
  }
  else
  {
    num_digits = 1;
  }

  // Convert to string
  for (i = 1; i <= num_digits; i++)
  {
    next_n = n/10;
    s[num_digits-i] = '0' + n - next_n*10;
    n = next_n;
  }
  s[num_digits] = 0;
}

static uint8_t cat_uint32(char*s, uint32_t n, int8_t chars)
{
  int8_t num_digits;
  uint8_t unit_steps;
  int8_t dp_pos;
  int8_t excess_digits;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  if (     n > 999999999)
  {
    num_digits = 10;
    unit_steps = 3;
    dp_pos = 1;
  }
  else if (n > 99999999)
  {
    num_digits = 9;
    unit_steps = 2;
    dp_pos = 3;
  }
  else if (n > 9999999)
  {
    num_digits = 8;
    unit_steps = 2;
    dp_pos = 2;
  }
  else if (n > 999999)
  {
    num_digits = 7;
    unit_steps = 2;
    dp_pos = 1;
  }
  else if (n > 99999)
  {
    num_digits = 6;
    unit_steps = 1;
    dp_pos = 3;
  }
  else if (n > 9999)
  {
    num_digits = 5;
    unit_steps = 1;
    dp_pos = 2;
  }
  else if (n > 999)
  {
    num_digits = 4;
    unit_steps = 1;
    dp_pos = 1;
  }
  else if (n > 99)
  {
    num_digits = 3;
    unit_steps = 0;
    dp_pos = 3;
  }
  else if (n > 9)
  {
    num_digits = 2;
    unit_steps = 0;
    dp_pos = 2;
  }
  else
  {
    num_digits = 1;
    unit_steps = 0;
    dp_pos = 1;
  }

  excess_digits = num_digits + 1 - chars;
  if (excess_digits > 0)
  {
    // 
    uint32_t add_to_round;
    uint32_t divisor;
    switch (excess_digits)
    {
    case 1: add_to_round = 5;        divisor = 10;        break;
    case 2: add_to_round = 50;       divisor = 100;       break;
    case 3: add_to_round = 500;      divisor = 1000;      break;
    case 4: add_to_round = 5000;     divisor = 10000;     break;
    case 5: add_to_round = 50000;    divisor = 100000;    break;
    case 6: add_to_round = 500000;   divisor = 1000000;   break;
    case 7: add_to_round = 5000000;  divisor = 10000000;  break;
    case 8: add_to_round = 50000000; divisor = 100000000; break;
    default:
      // overflow
      for (int i = 0; i < chars; i++)
      {
        s[i] = '-';
      }
      s[chars] = 0;
      return 0;
    }
    n = (n + add_to_round) / divisor;
  }

  // Convert to string
  if (dp_pos < chars)
  {
    s[dp_pos] = '.';
  }
  for (int i = 1; i <= chars; i++)
  {
    uint32_t next_n;
    int8_t dp_offset;
    int8_t pos;

    next_n = n/10;

    pos = chars-i;
    if (pos >= dp_pos)
    {
      pos++;
    }
    if (pos < chars)
    {
      s[pos] = '0' + (uint8_t)(n - next_n*10);
    }
    n = next_n;
  }
  s[chars] = 0;

  return unit_steps;
}
