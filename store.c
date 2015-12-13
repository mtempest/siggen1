#include <avr/eeprom.h>
#include <stdint.h>
#include <string.h>

#include "store.h"

/* This is where the settings are stored in EEPROM */
#define EEPROM_START 0

/* Number of cycles to wait before starting transfer to EEPROM */
#define WAIT_BEFORE_WRITING 30

static struct
{
  uint8_t osccal;
  int8_t fine_cal;
  uint8_t contrast;
  uint8_t backlight;
  uint8_t waveform;
  uint8_t checksum;
} settings;

static volatile uint8_t tick;
static uint8_t wait_count;

static uint8_t compute_checksum(void);

void STORE_init(void)
{
  uint8_t calc_checksum;
  eeprom_read_block((void *)&settings, (void*)EEPROM_START, sizeof(settings));
  calc_checksum = compute_checksum();
  if (calc_checksum != settings.checksum)
  {
    STORE_reset();
  }
  
}

void STORE_reset(void)
{
  memset(&settings, 0, sizeof(settings));
  settings.contrast = 64;
  wait_count = 1;
}

void STORE_tick(void)
{
  tick = 1;
}

void STORE_cyclic(void)
{
  if (tick)
  {
    tick = 0;
    if (wait_count > 0)
    {
      wait_count --;
      if (wait_count == 0)
      {
        settings.checksum = compute_checksum();
        eeprom_update_block((const void *)&settings, (void*)EEPROM_START, sizeof(settings));
      }
    }
  }
}

void STORE_set_osccal(uint8_t new_value)
{
  settings.osccal = new_value;
  wait_count = WAIT_BEFORE_WRITING;
}
uint8_t STORE_get_osccal(void)
{
  return settings.osccal;
}

void STORE_set_fine_cal(int8_t new_value)
{
  settings.fine_cal = new_value;
  wait_count = WAIT_BEFORE_WRITING;
}
int8_t STORE_get_fine_cal(void)
{
  return settings.fine_cal;
}

void STORE_set_contrast(uint8_t new_value)
{
  settings.contrast = new_value;
  wait_count = WAIT_BEFORE_WRITING;
}
uint8_t STORE_get_contrast(void)
{
  return settings.contrast;
}

void STORE_set_backlight(uint8_t new_value)
{
  settings.backlight = new_value;
  wait_count = WAIT_BEFORE_WRITING;
}
uint8_t STORE_get_backlight(void)
{
  return settings.backlight;
}

void STORE_set_waveform(uint8_t new_value)
{
  settings.waveform = new_value;
  wait_count = WAIT_BEFORE_WRITING;
}
uint8_t STORE_get_waveform(void)
{
  return settings.waveform;
}

static uint8_t compute_checksum(void)
{
  uint8_t sum;
  uint8_t *p;

  sum = 0xAA;
  p = (uint8_t*)&settings;
  while (p < &settings.checksum)
  {
    sum += *p;
    p++;
  }

  return sum;
}
