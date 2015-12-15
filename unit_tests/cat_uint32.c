#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "format.h"

int test(uint32_t n, int8_t chars, char* expected, uint8_t steps);

int main(void)
{
  int fail = 0;

  if (!fail) fail = test(1, 1, "-", 0);
  if (!fail) fail = test(1, 2, "--", 0);

  if (!fail) fail = test(1, 3, "1.0", 0);
  if (!fail) fail = test(10, 3, "10.", 0);
  if (!fail) fail = test(100, 3, "100", 0);
  if (!fail) fail = test(1000, 3, "1.0", 1);
  if (!fail) fail = test(10*1000, 3, "10.", 1);
  if (!fail) fail = test(100*1000, 3, "100", 1);
  if (!fail) fail = test(1000*1000, 3, "1.0", 2);
  if (!fail) fail = test(10*1000*1000, 3, "10.", 2);
  if (!fail) fail = test(100*1000*1000, 3, "100", 2);
  if (!fail) fail = test(1000*1000*1000, 3, "1.0", 3);
  if (!fail) fail = test(250, 3, "250", 0);
  if (!fail) fail = test(999, 3, "999", 0);

  if (!fail) fail = test(1, 4, "1.00", 0);
  if (!fail) fail = test(10, 4, "10.0", 0);
  if (!fail) fail = test(100, 4, "100.", 0);
  if (!fail) fail = test(1000, 4, "1.00", 1);
  if (!fail) fail = test(10*1000, 4, "10.0", 1);
  if (!fail) fail = test(100*1000, 4, "100.", 1);
  if (!fail) fail = test(1000*1000, 4, "1.00", 2);
  if (!fail) fail = test(10*1000*1000, 4, "10.0", 2);
  if (!fail) fail = test(100*1000*1000, 4, "100.", 2);
  if (!fail) fail = test(1000*1000*1000, 4, "1.00", 3);

  if (!fail) fail = test(1, 5, "1.000", 0);
  if (!fail) fail = test(10, 5, "10.00", 0);
  if (!fail) fail = test(100, 5, "100.0", 0);
  if (!fail) fail = test(1000, 5, "1.000", 1);
  if (!fail) fail = test(10*1000, 5, "10.00", 1);
  if (!fail) fail = test(100*1000, 5, "100.0", 1);
  if (!fail) fail = test(1000*1000, 5, "1.000", 2);
  if (!fail) fail = test(10*1000*1000, 5, "10.00", 2);
  if (!fail) fail = test(100*1000*1000, 5, "100.0", 2);
  if (!fail) fail = test(1000*1000*1000, 5, "1.000", 3);

  if (!fail) fail = test(1, 6, "1.0000", 0);
  if (!fail) fail = test(10, 6, "10.000", 0);
  if (!fail) fail = test(100, 6, "100.00", 0);
  if (!fail) fail = test(1000, 6, "1.0000", 1);
  if (!fail) fail = test(10*1000, 6, "10.000", 1);
  if (!fail) fail = test(100*1000, 6, "100.00", 1);
  if (!fail) fail = test(1000*1000, 6, "1.0000", 2);
  if (!fail) fail = test(10*1000*1000, 6, "10.000", 2);
  if (!fail) fail = test(100*1000*1000, 6, "100.00", 2);
  if (!fail) fail = test(1000*1000*1000, 6, "1.0000", 3);

  if (!fail) fail = test(1, 7, "1.00000", 0);
  if (!fail) fail = test(10, 7, "10.0000", 0);
  if (!fail) fail = test(100, 7, "100.000", 0);
  if (!fail) fail = test(1000, 7, "1.00000", 1);
  if (!fail) fail = test(10*1000, 7, "10.0000", 1);
  if (!fail) fail = test(100*1000, 7, "100.000", 1);
  if (!fail) fail = test(1000*1000, 7, "1.00000", 2);
  if (!fail) fail = test(10*1000*1000, 7, "10.0000", 2);
  if (!fail) fail = test(100*1000*1000, 7, "100.000", 2);
  if (!fail) fail = test(1000*1000*1000, 7, "1.00000", 3);

  if (!fail) fail = test(1, 8, "1.000000", 0);
  if (!fail) fail = test(10, 8, "10.00000", 0);
  if (!fail) fail = test(100, 8, "100.0000", 0);
  if (!fail) fail = test(1000, 8, "1.000000", 1);
  if (!fail) fail = test(10*1000, 8, "10.00000", 1);
  if (!fail) fail = test(100*1000, 8, "100.0000", 1);
  if (!fail) fail = test(1000*1000, 8, "1.000000", 2);
  if (!fail) fail = test(10*1000*1000, 8, "10.00000", 2);
  if (!fail) fail = test(100*1000*1000, 8, "100.0000", 2);
  if (!fail) fail = test(1000*1000*1000, 8, "1.000000", 3);

  return fail;
}

int test(uint32_t n, int8_t chars, char* expected, uint8_t steps)
{
  char buffer[128];
  uint8_t actual_steps;

  memset(buffer, 'A', sizeof(buffer));
  buffer[0] = 0;
  actual_steps = FORMAT_cat_uint32(buffer, n, chars);
  if (strcmp(buffer, expected) || (actual_steps != steps))
  {
    printf("FAIL: n=%u, chars=%d, expected='%s'/%u, got='%s'/%u\n",
           n, chars,
           expected, steps,
           buffer, actual_steps);
    return 1;
  }
  return 0;
}
