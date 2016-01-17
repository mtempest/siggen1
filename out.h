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

#define OUT_PERIOD_MODE 0
#define OUT_FREQ_MODE   1

#define OUT_MAX_NON_SQUARE_FREQUENCY_mHz (50UL*1000UL*1000UL)

#define OUT_MIN_NON_SQUARE_PERIOD_NS ((uint32_t)(1e12 / OUT_MAX_NON_SQUARE_FREQUENCY_mHz + 0.5))

void OUT_init(void);

void OUT_cyclic(void);

void OUT_recompute_actual(void);

void OUT_set_on(uint8_t new_value);
uint8_t OUT_get_on(void);

void OUT_set_fine_cal(int8_t new_value);
int8_t OUT_get_fine_cal(void);

void OUT_set_medium_cal(int8_t new_value);
int8_t OUT_get_medium_cal(void);

void OUT_set_waveform(uint8_t new_value);
uint8_t OUT_get_waveform(void);

void OUT_set_freq_mode(uint8_t new_value);
uint8_t OUT_get_freq_mode(void);

void OUT_set_freq_mHz(uint32_t new_value);
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
