#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "LCD5110_Graph.h"

#include "fonts.h"

#include "ui.h"
#include "out.h"
#include "store.h"
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
  PARAM_BACKLIGHT,
  PARAM_FINE_CALIBRATE,
  PARAM_OSCCAL,

  PARAM_LAST
};
static uint8_t selected_param;
static uint8_t units_from_mHz;

static volatile uint8_t up_press;
static volatile uint8_t down_press;
static volatile uint8_t next_press;

static void check_button(volatile uint8_t* port,
                         uint8_t mask,
                         uint8_t* history,
                         uint8_t* count,
                         volatile uint8_t* press);

static uint8_t check_up_down(uint8_t *value, uint8_t inclusive_max);

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

  // Configure Timer 2 to generate interrupt at approx 50Hz
  // and toggle OC2
  TCCR2 = (1<<WGM21)|(0<<WGM20)|
          (0<<COM21)|(1<<COM20)|
          (1<<CS22)|(1<<CS21)|(1<<CS20);
  OCR2 = (uint8_t)(F_CPU / 1024 / 50 - 1);
  TIMSK |= (1<<OCIE2);

  // Configure PD2, PD3, PD6 as inputs with pull-ups enabled
  DDRD &= ~((1<<PD2)|(1<<PD3)|(1<<PD4));
  PORTD |= (1<<PD2)|(1<<PD3)|(1<<PD4);

}

void UI_cyclic(void)
{
  if (next_press)
  {
    next_press = 0;
    selected_param++;
    if (selected_param == PARAM_LAST)
    {
      selected_param = PARAM_FIRST;
    }
  }

  myGLCD.setFont(SmallFont);
  show_units();
  show_on_off();
  show_waveform();
  show_parameter();

  myGLCD.setFont(BigNumbers);
  show_freq();

  myGLCD.update();
  
  myGLCD.setContrast(STORE_get_contrast());
}

ISR(TIMER2_COMP_vect)
{
  static uint8_t up_history;
  static uint8_t down_history;
  static uint8_t next_history;
  static uint8_t up_count;
  static uint8_t down_count;
  static uint8_t next_count;
  check_button(&PIND, 1<<PD2, &up_history, &up_count, &up_press);
  check_button(&PIND, 1<<PD3, &down_history, &down_count, &down_press);
  check_button(&PIND, 1<<PD4, &next_history, &next_count, &next_press);
  if ((up_history == 0xFF) &&
      (down_history == 0xFF) &&
      (next_history == 0xFF))
  {
    STORE_reset();
  }

  STORE_tick();
}

static void check_button(volatile uint8_t* port,
                         uint8_t mask,
                         uint8_t* history,
                         uint8_t* count,
                         volatile uint8_t* press)
{
  *history <<= 1;
  if ((*port & mask) == 0)
  {
    *history |= 1;

    if (*history == 0xFF)
    {
      // Button is in
      (*count)++;
      if (*count == 10)
      {
        // Auto-repeat
        *press = 1;
        *count = 0;
      }
    }
  }
  else
  {
    *count = 0;
  }
  if (*history == 0xFE)
  {
    // release
    *press = 1;
  }
}

static uint8_t check_up_down(uint8_t *value, uint8_t inclusive_max)
{
  uint8_t pressed;
  pressed = 0;
  if (up_press)
  {
    up_press = 0;
    pressed = 1;
    if (*value < inclusive_max)
    {
      (*value)++;
    }
    else
    {
      *value = 0;
    }
  }
  if (down_press)
  {
    down_press = 0;
    pressed = 1;
    if (*value == 0)
    {
      *value = inclusive_max;
    }
    else
    {
      (*value)--;
    }
  }
  return pressed;
}

static char units_string[4] = "?Hz";
static void show_units(void)
{
  switch (units_from_mHz)
  {
  case 0: units_string[0] = 'm'; break;
  case 1: units_string[0] = ' '; break;
  case 2: units_string[0] = 'k'; break;
  case 3: units_string[0] = 'M'; break;
  default:units_string[0] = '?'; break;
  }
  myGLCD.print_P(units_string, RIGHT, LINE_2);
}

static void show_on_off(void)
{
  const char* s;
  if (OUT_get_on())
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
  const char* s;
  uint8_t waveform;
  waveform = OUT_get_waveform();
  if (waveform == OUT_TRIANGLE)
  {
    s = PSTR("triangle");
  }
  else if (waveform == OUT_SINE)
  {
    s = PSTR("sine    ");
  }
  else
  {
    s = PSTR("square  ");
  }
  if (selected_param == PARAM_WAVEFORM)
  {
    if (check_up_down(&waveform, OUT_WAVEFORM_LAST))
    {
      OUT_set_waveform(waveform);
    }
  }

  myGLCD.print_P(s, 0, LINE_2);
}

static void show_freq(void)
{
  static char s[7];
  s[0] = 0;
  units_from_mHz = cat_uint32(s, OUT_get_freq_mHz(), 6);
  myGLCD.print(s, CENTER, LINE_1);
}

static void show_parameter(void)
{
  static char s[15];
  static const char percent[] PROGMEM = "%";
  uint8_t unit_steps;
  s[0] = '\0';
  uint32_t u32;
  uint8_t u8;
  static const uint32_t period_step[4] = { 1, 1, 1000, 1000000 };

  switch (selected_param)
  {
  case PARAM_FREQUENCY:
    strcpy_P(s, PSTR("Frequency"));
    cat_uint8(s, PIND);
    // The value is always shown on the first line
    // so it is not appended here
    break;

  case PARAM_PERIOD:
    // The period is a 32-bit value in nanoseconds
    strcpy_P(s, PSTR("T(ns):"));
    u32 = OUT_get_period_ns();
    unit_steps = cat_uint32(s, u32, 8);
    switch (unit_steps)
    {
    case 0: break;
    case 1: s[2] = 'u'; break; //TODO: use a mu symbol
    case 2: s[2] = 'm'; break;
    case 3: s[2] = ' '; break;
    default:s[2] = '?'; break;
    }
    if (up_press)
    {
      up_press = 0;
      u32 += period_step[unit_steps];
      OUT_set_period_ns(u32);
    }
    if (down_press)
    {
      down_press = 0;
      u32 -= period_step[unit_steps];
      OUT_set_period_ns(u32);
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
    u8 = OUT_get_duty_cycle_percent();
    cat_uint8(s, u8);
    strcat_P(s, percent);
    if (check_up_down(&u8, 100))
    {
      OUT_set_duty_cycle_percent(u8);
    }
    break;

  case PARAM_AMPLITUDE:
    strcpy_P(s, PSTR("Amplitude:"));
    u8 = OUT_get_amplitude_percent();
    cat_uint8(s, u8);
    strcat_P(s, percent);
    if (check_up_down(&u8, 100))
    {
      OUT_set_amplitude_percent(u8);
    }
    break;

  case PARAM_CONTRAST:
    strcpy_P(s, PSTR("Contrast:"));
    u8 = STORE_get_contrast();
    cat_uint8(s, u8);
    if (check_up_down(&u8, 127))
    {
      myGLCD.setContrast(u8);
      STORE_set_contrast(u8);
    }
    break;

  case PARAM_BACKLIGHT:
    strcpy_P(s, PSTR("Backlight:"));
    cat_uint8(s, STORE_get_backlight());
    break;

  case PARAM_FINE_CALIBRATE:
    strcpy_P(s, PSTR("Fine cal:"));
    cat_int8(s, STORE_get_fine_cal());
    break;

  case PARAM_OSCCAL:
    strcpy_P(s, PSTR("OSCCAL:"));
    u8 = STORE_get_osccal();
    cat_uint8(s, u8);
    if (check_up_down(&u8, 255))
    {
      OSCCAL = u8;
      STORE_set_osccal(u8);
    }
    break;

  default:
    selected_param = PARAM_FREQUENCY;
    break;
  }
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
