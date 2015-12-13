#ifndef __STORE_H_
#define __STORE_H_

#ifdef __cplusplus
extern "C" {
#endif

void STORE_init(void);

void STORE_cyclic(void);

void STORE_tick(void);

void STORE_reset(void);

void STORE_set_osccal(uint8_t new_value);
uint8_t STORE_get_osccal(void);

void STORE_set_fine_cal(int8_t new_value);
int8_t STORE_get_fine_cal(void);

void STORE_set_contrast(uint8_t new_value);
uint8_t STORE_get_contrast(void);

void STORE_set_backlight(uint8_t new_value);
uint8_t STORE_get_backlight(void);

void STORE_set_waveform(uint8_t new_value);
uint8_t STORE_get_waveform(void);

#ifdef __cplusplus
}
#endif


#endif
