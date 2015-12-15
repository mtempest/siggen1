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
#include "format.h"

/* Y coordinates for each line */
#define LINE_1 0
#define LINE_2 24
#define LINE_3 32
#define LINE_4 40

/* Number of characters on the first line */
#define LINE_1_LENGTH 6

enum
{
  PARAM_FIRST,

  PARAM_NUMBER1 = PARAM_FIRST,
  PARAM_NUMBER2,
  PARAM_NUMBER3,
  PARAM_NUMBER4,
  PARAM_NUMBER5,
  PARAM_SCALE,
  PARAM_ALTERNATE,
  PARAM_WAVEFORM,
  PARAM_DUTY_CYCLE,
  PARAM_AMPLITUDE,
  PARAM_CONTRAST,
  PARAM_BACKLIGHT,
  PARAM_FINE_CALIBRATE,
  PARAM_OSCCAL,

  PARAM_LAST = PARAM_OSCCAL
};
static uint8_t selected_param;
static uint8_t units_steps;
static volatile int8_t wait_after_freq_change;

static volatile uint8_t up_press;
static volatile uint8_t down_press;
static volatile uint8_t next_press;
static volatile uint8_t prev_press;

static char scratch[15];

static void check_button(volatile uint8_t* port,
                         uint8_t mask,
                         uint8_t* history,
                         uint8_t* count,
                         volatile uint8_t* press);

static uint8_t check_up_down(uint8_t *value, uint8_t inclusive_max);

static void edit_number(void);

static void show_number(void);
static void show_units(void);
static void show_on_off_edit(void);
static void show_waveform(void);
static void show_parameter(void);

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
  if (wait_after_freq_change == 1)
  {
    wait_after_freq_change = 0;
    OUT_recompute_actual();
  }

  if (next_press)
  {
    next_press = 0;
    if (selected_param < PARAM_LAST)
    {
      selected_param++;
    }
    else
    {
      selected_param = PARAM_FIRST;
    }
  }
  if (prev_press)
  {
    prev_press = 0;
    if (selected_param > PARAM_FIRST)
    {
      selected_param--;
    }
    else
    {
      selected_param = PARAM_LAST;
    }
  }

  myGLCD.clrScr();

  myGLCD.setFont(BigNumbers);
  show_number();

  myGLCD.setFont(SmallFont);
  show_units();
  show_on_off_edit();
  show_waveform();
  show_parameter();

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

  if (wait_after_freq_change > 1)
  {
    wait_after_freq_change --;
  }
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
  if ((*history & 0xCF) == 0x03)
  {
    // press
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

static void show_units(void)
{
  static char units_string[4];
  if (OUT_get_freq_mode() == OUT_FREQ_MODE)
  {
    // base units is mHz
    strcpy_P(units_string, PSTR("?Hz"));
    switch (units_steps)
    {
    case 0: units_string[0] = 'm'; break;
    case 1: units_string[0] = ' '; break;
    case 2: units_string[0] = 'k'; break;
    case 3: units_string[0] = 'M'; break;
    default:units_string[0] = '?'; break;
    }
  }
  else
  {
    // base units is ns
    strcpy_P(units_string, PSTR("?s"));
    switch (units_steps)
    {
    case 0: units_string[0] = 'n'; break;
    case 1: units_string[0] = 'u'; break;
    case 2: units_string[0] = 'm'; break;
    case 3: units_string[0] = ' '; break;
    default:units_string[0] = '?'; break;
    }
  }
  myGLCD.print(units_string, RIGHT, LINE_2);
}

static void show_on_off_edit(void)
{
  const char* s;
  if (wait_after_freq_change != 0)
  {
    s = PSTR("Edit");
  }
  else if (OUT_get_on())
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
    s = PSTR("sine");
  }
  else
  {
    s = PSTR("square");
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

static void edit_number(void)
{
  char *p;
  uint32_t n;
  uint8_t up;
  uint8_t down;
  uint8_t unit_steps;
  int8_t exponent;
  uint8_t dp_position;
  uint8_t digit_position;

  if (up_press)
  {
    up_press = 0;
    up = 1;
  }
  else
  {
    up = 0;
  }
  if (down_press)
  {
    down_press = 0;
    down = 1;
  }
  else
  {
    down = 0;
  }
  if (up || down)
  {
    if (OUT_get_freq_mode() == OUT_FREQ_MODE)
    {
      n = OUT_get_freq_mHz();
    }
    else
    {
      n = OUT_get_period_ns();
    }
    if (selected_param == PARAM_SCALE)
    {
      if (up)
      {
        if (n < 400UL*1000UL*1000UL)
        {
          n *= 10;
        }
      }
      else // down
      {
        if (n > 2500)
        {
          n /= 10;
        }
      }
    }
    else // change digit
    {
      // convert n to string
      unit_steps = FORMAT_cat_uint32(scratch, n, LINE_1_LENGTH);

      // find the decimal point
      p = scratch;
      dp_position = 0;
      while ((*p != '.') && (*p != 0))
      {
        dp_position++;
        p++;
      }

      // change the current digit
      digit_position = selected_param - PARAM_NUMBER1;
      if (digit_position >= dp_position)
      {
        digit_position++;
      }
      if (up)
      {
        if (scratch[digit_position] < '9')
        {
          scratch[digit_position]++;
        }
        else
        {
          scratch[digit_position] = '0';
        }
      }
      else /* down */
      {
        if (scratch[digit_position] > '0')
        {
          scratch[digit_position]--;
        }
        else
        {
          scratch[digit_position] = '9';
        }
      }

      // convert back to number
      n = 0;
      p = scratch;
      while (*p != 0)
      {
        if (*p != '.')
        {
          n *= 10;
          n += *p - '0';
        }
        p++;
      }

      // correct for missing digits
      exponent = unit_steps * 3;
      exponent -= strlen(scratch) - dp_position - 1;
      while (exponent > 0)
      {
        n *= 10;
        exponent--;
      }

    }
    if (OUT_get_freq_mode() == OUT_FREQ_MODE)
    {
      OUT_set_freq_mHz(n);
    }
    else
    {
      OUT_set_period_ns(n);
    }
    wait_after_freq_change = 150;
  }
}

static void show_number(void)
{
  uint32_t u32;
  if (OUT_get_freq_mode() == OUT_FREQ_MODE)
  {
    u32 = OUT_get_freq_mHz();
  }
  else
  {
    u32 = OUT_get_period_ns();
  }
  scratch[0] = 0;
  units_steps = FORMAT_cat_uint32(scratch, u32, LINE_1_LENGTH);
  myGLCD.print(scratch, CENTER, LINE_1);
}

static void show_parameter(void)
{
  static char s[15];
  static const char percent[] PROGMEM = "%";
  uint8_t unit_steps;
  scratch[0] = '\0';
  uint32_t u32;
  uint8_t u8;
  int8_t i8;
  static uint8_t freq_mode;

  freq_mode = (OUT_get_freq_mode() == OUT_FREQ_MODE);
  switch (selected_param)
  {
  case PARAM_NUMBER1:
  case PARAM_NUMBER2:
  case PARAM_NUMBER3:
  case PARAM_NUMBER4:
  case PARAM_NUMBER5:
    if (freq_mode)
    {
      strcpy_P(s, PSTR("Freq. digit "));
    }
    else
    {
      strcpy_P(s, PSTR("Period digit "));
    }
    FORMAT_cat_uint8(s, selected_param+1);
    edit_number();
    break;

  case PARAM_SCALE:
    strcpy_P(s, PSTR("Decimal point"));
    edit_number();
    break;

  case PARAM_ALTERNATE:
    if (freq_mode)
    {
      strcpy_P(s, PSTR("T(ns):"));
      u32 = OUT_get_period_ns();
      unit_steps = FORMAT_cat_uint32(s, u32, 8);
      switch (unit_steps)
      {
      case 0: break;
      case 1: s[2] = 'u'; break; //TODO: use a mu symbol
      case 2: s[2] = 'm'; break;
      case 3: s[2] = ' '; break;
      default:s[2] = '?'; break;
      }
    }
    else
    {
      strcpy_P(s, PSTR("F(mHz):"));
      u32 = OUT_get_freq_mHz();
      unit_steps = FORMAT_cat_uint32(s, u32, 7);
      switch (unit_steps)
      {
      case 0: break;
      case 1: s[2] = ' '; break;
      case 2: s[2] = 'k'; break;
      case 3: s[2] = 'M'; break;
      default:s[2] = '?'; break;
      }
    }
    if (up_press || down_press)
    {
      up_press = 0;
      down_press = 0;
      if (freq_mode)
      {
        u8 = OUT_PERIOD_MODE;
      }
      else
      {
        u8 = OUT_FREQ_MODE;
      }
      OUT_set_freq_mode(u8);
    }
    break;

  case PARAM_WAVEFORM:
    strcpy_P(s, PSTR("Waveform"));
    // Waveform is always displayed on line 2
    // so no need to repeat it here
    break;

  case PARAM_DUTY_CYCLE:
    strcpy_P(s, PSTR("Duty-cycle:"));
    u8 = OUT_get_duty_cycle_percent();
    FORMAT_cat_uint8(s, u8);
    strcat_P(s, percent);
    if (check_up_down(&u8, 100))
    {
      OUT_set_duty_cycle_percent(u8);
    }
    break;

  case PARAM_AMPLITUDE:
    strcpy_P(s, PSTR("Amplitude:"));
    u8 = OUT_get_amplitude_percent();
    FORMAT_cat_uint8(s, u8);
    strcat_P(s, percent);
    if (check_up_down(&u8, 100))
    {
      OUT_set_amplitude_percent(u8);
    }
    break;

  case PARAM_CONTRAST:
    strcpy_P(s, PSTR("Contrast:"));
    u8 = STORE_get_contrast();
    FORMAT_cat_uint8(s, u8);
    if (check_up_down(&u8, 127))
    {
      myGLCD.setContrast(u8);
      STORE_set_contrast(u8);
    }
    break;

  case PARAM_BACKLIGHT:
    strcpy_P(s, PSTR("Backlight:"));
    FORMAT_cat_uint8(s, STORE_get_backlight());
    break;

  case PARAM_FINE_CALIBRATE:
    strcpy_P(s, PSTR("Fine cal:"));
    i8 = OUT_get_fine_cal();
    FORMAT_cat_int8(s, i8);
    u8 = i8 + 128;
    if (check_up_down(&u8, 255))
    {
      i8 = u8 - 128;
      OUT_set_fine_cal(i8);
    }
    break;

  case PARAM_OSCCAL:
    strcpy_P(s, PSTR("OSCCAL:"));
    u8 = STORE_get_osccal();
    FORMAT_cat_uint8(s, u8);
    if (check_up_down(&u8, 255))
    {
      OSCCAL = u8;
      STORE_set_osccal(u8);
    }
    break;

  default:
    selected_param = PARAM_FIRST;
    break;
  }
  myGLCD.print(s, CENTER, LINE_4);
}

