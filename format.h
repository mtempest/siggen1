#ifndef __FORMAT_H_
#define __FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

void FORMAT_cat_int8(char* s, int8_t n);

void FORMAT_cat_uint8(char* s, uint8_t n);

uint8_t FORMAT_cat_uint32(char* s, uint32_t n, int8_t chars);

#ifdef __cplusplus
}
#endif

#endif
