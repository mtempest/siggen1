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

// 5-bit R-2R DAC
#define DAC_CENTRE 16
#define DAC_AMPL   15

#define WAVEFORM_LENGTH 32

#define SIN__1_OVER_32_PI  0.09802 /* sin(pi *  1/32) */
#define SIN__3_OVER_32_PI  0.29028 /* sin(pi *  3/32) */
#define SIN__5_OVER_32_PI  0.47140 /* sin(pi *  5/32) */
#define SIN__7_OVER_32_PI  0.63439 /* sin(pi *  7/32) */
#define SIN__9_OVER_32_PI  0.77301 /* sin(pi *  9/32) */
#define SIN_11_OVER_32_PI  0.88192 /* sin(pi * 11/32) */
#define SIN_13_OVER_32_PI  0.95694 /* sin(pi * 13/32) */
#define SIN_15_OVER_32_PI  0.99518 /* sin(pi * 15/32) */

static uint8_t on;
static uint8_t freq_mode = OUT_FREQ_MODE;
static uint8_t waveform;
static uint8_t duty_cycle = 50;
static uint8_t amplitude = 100;
static int8_t fine_cal;
static int8_t medium_cal;

static uint32_t period_ns;
static uint32_t freq_mHz = 1000000;

static uint8_t waveform_data[WAVEFORM_LENGTH];

static void range_limit(uint32_t* n);
static void recompute_waveform(void);

void OUT_init(void)
{
  fine_cal = STORE_get_fine_cal();
  medium_cal = STORE_get_medium_cal();

  /* Setup timer 1 in fast PWM mode,
     going low on compare match
     with TOP=ICR1
     and force output compare */
  TCCR1A = (1<<FOC1A)|(1<<FOC1B)|(1<<WGM11)|(0<<WGM10)|(1<<COM1A1)|(0<<COM1A0);
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
  uint32_t timer_period_ns;
  uint32_t timer_freq_mHz;

  if (freq_mode == OUT_PERIOD_MODE)
  {
    if (period_ns < OUT_MIN_NON_SQUARE_PERIOD_NS)
    {
      waveform = OUT_SQUARE;
    }
  }
  else /* frequency mode */
  {
    if (freq_mHz > OUT_MAX_NON_SQUARE_FREQUENCY_mHz)
    {
      waveform = OUT_SQUARE;
    }
  }

  DDRD &= ~((1<<PD0)|(1<<PD1)|(1<<PD2)|(1<<PD3)|(1<<PD4));
  TIMSK &= (~1<<TOIE1);
  if (waveform == OUT_SQUARE)
  {
    timer_period_ns = period_ns;
    timer_freq_mHz = freq_mHz;
  }
  else
  {
    timer_period_ns = (period_ns + WAVEFORM_LENGTH/2)/WAVEFORM_LENGTH;
    timer_freq_mHz = freq_mHz * WAVEFORM_LENGTH;
  }

  f_cpu = (uint32_t)((int32_t)F_CPU + 2048L*medium_cal + 32L*fine_cal);
  f_period_ns = (1000UL*1000UL*1000UL + f_cpu/2) / f_cpu;

  if (freq_mode == OUT_PERIOD_MODE)
  {
    range_limit(&timer_period_ns);

    /* Convert the period in ns to a prescaler setting
       and a period in CPU clock cycles */

    /* First convert to CPU clock cycles, since this is the resolution
       of the timer itself. */
    period_clocks = (timer_period_ns + f_period_ns/2) / f_period_ns;
  }
  else
  {
    range_limit(&timer_freq_mHz);

    /* Convert the frequency in mHz to a prescaler setting
       and a period in CPU clock cycles */

    /* First convert to CPU clock cycles, since this is the resolution
       of the timer itself. */
    period_clocks = (F_CPU_MUL * f_cpu) / ((timer_freq_mHz + F_OUT_DIV/2) / F_OUT_DIV);
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
  timer_period_ns = period_clocks * prescaler * f_period_ns;

  // Compute the actual frequency
  timer_freq_mHz = (F_CPU_MUL * f_cpu) / ((period_clocks * prescaler + F_OUT_DIV/2) / F_OUT_DIV);

  if (waveform == OUT_SQUARE)
  {
    period_ns = timer_period_ns;
    freq_mHz = timer_freq_mHz;
  }
  else
  {
    period_ns = timer_period_ns * WAVEFORM_LENGTH;
    freq_mHz = timer_freq_mHz / WAVEFORM_LENGTH;
    recompute_waveform();
    TIMSK |= 1<<TOIE1;
    DDRD |= (1<<PD0)|(1<<PD1)|(1<<PD2)|(1<<PD3)|(1<<PD4);
  }
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

static void recompute_waveform(void)
{
  uint8_t i;
  uint8_t half_inverse_symmetrical;
  uint8_t quadrant_symmetrical;

  switch (waveform)
  {
  case OUT_SQUARE:
  default:
    for (i = 0; i < WAVEFORM_LENGTH/4; i++)
    {
      waveform_data[i] = DAC_CENTRE + DAC_AMPL;
    }
    quadrant_symmetrical = 1;
    half_inverse_symmetrical = 1;
    break;

  case OUT_TRIANGLE:
    for (i = 0; i <= WAVEFORM_LENGTH/4; i++)
    {
      waveform_data[i] = DAC_CENTRE + DAC_AMPL * (int)i / (WAVEFORM_LENGTH/4);
    }
    for (i = 1; i < WAVEFORM_LENGTH/4; i++)
    {
      waveform_data[WAVEFORM_LENGTH/4 + i] = waveform_data[WAVEFORM_LENGTH/4+1 - i];
    }
    quadrant_symmetrical = 0;
    half_inverse_symmetrical = 1;
    break;

  case OUT_SINE:
    waveform_data[0] = (uint16_t)(DAC_CENTRE + SIN__1_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[1] = (uint16_t)(DAC_CENTRE + SIN__3_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[2] = (uint16_t)(DAC_CENTRE + SIN__5_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[3] = (uint16_t)(DAC_CENTRE + SIN__7_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[4] = (uint16_t)(DAC_CENTRE + SIN__9_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[5] = (uint16_t)(DAC_CENTRE + SIN_11_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[6] = (uint16_t)(DAC_CENTRE + SIN_13_OVER_32_PI * DAC_AMPL + 0.5);
    waveform_data[7] = (uint16_t)(DAC_CENTRE + SIN_15_OVER_32_PI * DAC_AMPL + 0.5);
    half_inverse_symmetrical = 1;
    quadrant_symmetrical = 1;
    break;
  }

  if (quadrant_symmetrical)
  {
    /* second quadrant is the reverse of the first quadrant */
    for (i = 0; i < WAVEFORM_LENGTH/4; i++)
    {
      waveform_data[WAVEFORM_LENGTH/4 + i] = waveform_data[WAVEFORM_LENGTH/4-1 - i];
    }
  }

  if (half_inverse_symmetrical)
  {
    /* second half is the reverse of the first half,
       mirrored about the time axis */
    for (i = 0; i < WAVEFORM_LENGTH/2; i++)
    {
      waveform_data[WAVEFORM_LENGTH - 1 - i] = DAC_CENTRE - (waveform_data[i] - DAC_CENTRE);
    }
  }

  /* Scale the data taking the amplitude into account */
  for (i = 0; i < WAVEFORM_LENGTH; i++)
  {
    waveform_data[i] = (uint16_t)(((int32_t)waveform_data[i] - DAC_CENTRE) * amplitude / 100 + DAC_CENTRE);
  }
}

ISR(TIMER1_OVF_vect)
{
  uint8_t next_index = TCNT0; // TCNT0 is static storage for the waveform index
  next_index++;
  next_index %= WAVEFORM_LENGTH;
  TCNT0 = next_index;
  PORTD = waveform_data[next_index];
}

void OUT_set_freq_mode(uint8_t new_value)
{
  freq_mode = new_value;
}
uint8_t OUT_get_freq_mode(void)
{
  return freq_mode;
}

void OUT_set_on(uint8_t new_value)
{
  on = new_value; // TODO
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

  if (waveform != OUT_SQUARE)
  {
    if (freq_mode == OUT_PERIOD_MODE)
    {
      while (period_ns < OUT_MIN_NON_SQUARE_PERIOD_NS)
      {
        period_ns *= 10;
      }
    }
    else /* frequency mode */
    {
      if (freq_mHz > OUT_MAX_NON_SQUARE_FREQUENCY_mHz)
      {
        freq_mHz /= 10;
      }
    }
  }

  OUT_recompute_actual();
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
  duty_cycle = new_value;
}
uint8_t OUT_get_duty_cycle_percent(void)
{
  return duty_cycle;
}

void OUT_set_amplitude_percent(uint8_t new_value)
{
  amplitude = new_value;
  OUT_recompute_actual();
}
uint8_t OUT_get_amplitude_percent(void)
{
  return amplitude;
}

