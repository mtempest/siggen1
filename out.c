#include <stdint.h>
#include <avr/io.h>

#include "ui.h"
#include "out.h"

static uint8_t waveform;
static uint8_t duty_cycle;
static uint8_t amplitude;

static uint32_t period_ns;
static uint32_t freq_mHz;

void OUT_init(void)
{
  period_ns = 1000000;
  duty_cycle = 50;
  amplitude = 100;

  freq_mHz = 1000000000/period_ns;
}

void OUT_cyclic(void)
{
}


void OUT_set_on(uint8_t new_value)
{
}
uint8_t OUT_get_on(void)
{
  return 0;//TODO
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

uint32_t OUT_get_freq_mHz(void)
{
  return freq_mHz;
}

void OUT_set_period_ns(uint32_t new_value)
{
  period_ns = new_value; //TODO
  freq_mHz = 1000000000/period_ns;
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

