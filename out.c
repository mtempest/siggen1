#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ui.h"
#include "out.h"
#include "store.h"

// The period in clock cycles from frequency in mHz
// is p = F_CPU / (f / 1000) i.e. p = (1000 * F_CPU) / f
// but 1000 * F_CPU may overflow a 32-bit integer
// so find the smallest power of 2 k such that 
// (1000/k) * F_CPU does not overflow
// i.e. (1000/k) * F_CPU < 0xFFFFFFFF
// or (1000/k) < 0xFFFFFFFF/F_CPU
// Then p = (1000/k * F_CPU) / (f/k)
#if 1000 < 0xFFFFFFFF/F_CPU
  #define F_CPU_MUL 1000
  #define F_OUT_DIV 1
#elif 500 < 0xFFFFFFFF/F_CPU 
  #define F_CPU_MUL 500
  #define F_OUT_DIV 2
#elif 250 < 0xFFFFFFFF/F_CPU
  #define F_CPU_MUL 250
  #define F_OUT_DIV 4
#elif 125 < 0xFFFFFFFF/F_CPU 
  #define F_CPU_MUL 125
  #define F_OUT_DIV 8
#else
  #error Cannot compute frequency scaling parameters
#endif

static uint8_t on;
static uint8_t freq_mode = OUT_SQUARE;
static uint8_t waveform;
static uint8_t duty_cycle = 50;
static uint8_t amplitude = 100;
static int8_t fine_cal;
static int8_t medium_cal;

static uint32_t period_ns;
static uint32_t freq_mHz = 1000000;

static void range_limit(uint32_t* n);

void OUT_init(void)
{
  fine_cal = STORE_get_fine_cal();
  medium_cal = STORE_get_medium_cal();

  /* Setup timer 1 in fast PWM mode with TOP=ICR1
     and force output compare */
  TCCR1A = (1<<FOC1A)|(1<<FOC1B)|(1<<WGM11)|(0<<WGM10);
  TCCR1B = (1<<WGM13)|(1<<WGM12);

  OUT_recompute_actual();
  OUT_set_on(1);

  DDRB |= (1<<PB1);
}

void OUT_cyclic(void)
{
}

void OUT_recompute_actual(void)
{
  uint32_t period_clocks;
  uint8_t prescaler_bits;
  uint16_t prescaler;
  uint16_t oc;
  uint32_t f_cpu;
  uint32_t f_period_ns;

  f_cpu = (uint32_t)((int32_t)F_CPU + 2048L*medium_cal + 32L*fine_cal);
  f_period_ns = (1000UL*1000UL*1000UL + f_cpu/2) / f_cpu;
  if (freq_mode == OUT_PERIOD_MODE)
  {
    range_limit(&period_ns);

    /* Convert the period in ns to a prescaler setting
       and a period in CPU clock cycles */

    /* First convert to CPU clock cycles, since this is the resolution
       of the timer itself. */
    period_clocks = (period_ns + f_period_ns/2) / f_period_ns;
  }
  else
  {
    range_limit(&freq_mHz);

    /* Convert the frequency in mHz to a prescaler setting
       and a period in CPU clock cycles */

    /* First convert to CPU clock cycles, since this is the resolution
       of the timer itself. */
    period_clocks = (F_CPU_MUL * f_cpu) / ((freq_mHz + F_OUT_DIV/2) / F_OUT_DIV);
  }

  /* The period in clock cycles will in general be larger than 16 bits.
     So determine the smallest prescaler value that produces
     a clock period that fits in a 16-bit register. */
  if (period_clocks < 65536)
  {
    prescaler = 1;
    prescaler_bits = (0<<CS12)|(0<<CS11)|(1<<CS10);
  }
  else if ((period_clocks + 4) <= 65536*8)
  {
    prescaler = 8;
    prescaler_bits = (0<<CS12)|(1<<CS11)|(0<<CS10);
    period_clocks = (period_clocks + 4) / 8;
  }
  else if ((period_clocks + 32) <= 65536*64)
  {
    prescaler = 64;
    prescaler_bits = (0<<CS12)|(1<<CS11)|(1<<CS10);
    period_clocks = (period_clocks + 32) / 64;
  }
  else if ((period_clocks + 128) <= 65536*256)
  {
    prescaler = 156;
    prescaler_bits = (1<<CS12)|(0<<CS11)|(0<<CS10);
    period_clocks = (period_clocks + 128) / 256;
  }
  else
  {
    prescaler = 1024;
    prescaler_bits = (1<<CS12)|(0<<CS11)|(1<<CS10);
    period_clocks = (period_clocks + 512) / 1024;
    if (period_clocks > 65536)
    {
      period_clocks = 65536;
    }
  }

  /* Set up the timer accordingly */
  TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10));
  TCCR1B |= prescaler_bits;
  oc = (uint16_t)((period_clocks * duty_cycle + 50) / 100);
  if (oc > 0)
  {
    oc--;
  }
  cli();
  OCR1A = 0;
  ICR1 = (uint16_t)period_clocks - 1;
  TCNT1 = 0;
  OCR1A = oc;
  sei();

  // Compute the actual period
  period_ns = period_clocks * prescaler * f_period_ns;

  // Compute the actual frequency
  freq_mHz = (F_CPU_MUL * f_cpu) / ((period_clocks * prescaler + F_OUT_DIV/2) / F_OUT_DIV);
}

static void range_limit(uint32_t* n)
{
  if (*n < 250)
  {
    *n = 250;
  }
  if (*n > 4UL*1000UL*1000UL*1000UL)
  {
    *n = 4UL*1000UL*1000UL*1000UL;
  }
}

void OUT_set_freq_mode(uint8_t new_value)
{
  freq_mode = new_value;//TODO
}
uint8_t OUT_get_freq_mode(void)
{
  return freq_mode;
}

void OUT_set_on(uint8_t new_value)
{
  on = new_value;
  TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));
  if (on)
  {
    // PWM, going low on compare match
    TCCR1A |= (1<<COM1A1)|(0<<COM1A0);
  }
}
uint8_t OUT_get_on(void)
{
  return on;
}

void OUT_set_fine_cal(int8_t new_value)
{
  fine_cal = new_value;
  STORE_set_fine_cal(fine_cal);
  OUT_recompute_actual();
}
int8_t OUT_get_fine_cal(void)
{
  return fine_cal;
}

void OUT_set_medium_cal(int8_t new_value)
{
  medium_cal = new_value;
  STORE_set_medium_cal(medium_cal);
  OUT_recompute_actual();
}
int8_t OUT_get_medium_cal(void)
{
  return medium_cal;
}

void OUT_set_waveform(uint8_t new_value)
{
  waveform = new_value;
  //TODO
}
uint8_t OUT_get_waveform(void)
{
  return waveform;
}

void OUT_set_freq_mHz(uint32_t new_value)
{
  freq_mHz = new_value;
}
uint32_t OUT_get_freq_mHz(void)
{
  return freq_mHz;
}

void OUT_set_period_ns(uint32_t new_value)
{
  period_ns = new_value;
}
uint32_t OUT_get_period_ns(void)
{
  return period_ns;
}

void OUT_set_duty_cycle_percent(uint8_t new_value)
{
  duty_cycle = new_value; //TODO
}
uint8_t OUT_get_duty_cycle_percent(void)
{
  return duty_cycle;
}

void OUT_set_amplitude_percent(uint8_t new_value)
{
  amplitude = new_value;//TODO
}
uint8_t OUT_get_amplitude_percent(void)
{
  return amplitude;
}

