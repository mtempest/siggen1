#ifndef __OUT_H_
#define __OUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OUT_WAVEFORM_FIRST 0
#define OUT_SQUARE   0
#define OUT_TRIANGLE 1
#define OUT_SINE     2
#define OUT_WAVEFORM_LAST  2

void OUT_init(void);

void OUT_cyclic(void);

void OUT_set_on(uint8_t new_value);
uint8_t OUT_get_on(void);

void OUT_set_waveform(uint8_t new_value);
uint8_t OUT_get_waveform(void);

uint32_t OUT_get_freq_mHz(void);

void OUT_set_period_ns(uint32_t new_value);
uint32_t OUT_get_period_ns(void);

void OUT_set_duty_cycle_percent(uint8_t new_value);
uint8_t OUT_get_duty_cycle_percent(void);

void OUT_set_amplitude_percent(uint8_t new_value);
uint8_t OUT_get_amplitude_percent(void);

#ifdef __cplusplus
}
#endif

#endif
