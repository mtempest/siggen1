#include <string.h>
#include <stdint.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "format.h"

void FORMAT_cat_int8(char*s, int8_t n)
{
  uint8_t u;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  // Convert to unsigned, appending minus sign if needed
  if (n < 0)
  {
    *s = '-';
    s++;
    *s = 0;
    u = (uint8_t)(-n);
  }
  else
  {
    u = (uint8_t)n;
  }

  // append the digits
  FORMAT_cat_uint8(s, u);
}

void FORMAT_cat_uint8(char*s, uint8_t n)
{
  uint8_t num_digits;
  uint8_t i;
  uint8_t next_n;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  // Work out the number of digits
  if (n > 99)
  {
    num_digits = 3;
  }
  else if (n > 9)
  {
    num_digits = 2;
  }
  else
  {
    num_digits = 1;
  }

  // Convert to string
  for (i = 1; i <= num_digits; i++)
  {
    next_n = n/10;
    s[num_digits-i] = '0' + n - next_n*10;
    n = next_n;
  }
  s[num_digits] = 0;
}

uint8_t FORMAT_cat_uint32(char* s, uint32_t n, int8_t chars)
{
  int8_t num_digits;
  uint8_t unit_steps;
  int8_t dp_position;
  int8_t required_digits;
  int8_t exponent;

  // Find the end of the string
  while (*s != 0)
  {
    s++;
  }

  if (chars < 3)
  {
    // This function needs at least 3 character spaces to work in
    // for up to 3 digits before the decimal point
    while (chars > 0)
    {
      *s = '-';
      s++;
      chars--;
    }
    *s = 0;
    return 0;
  }

#ifdef DEBUG
  printf("\nn=%u, chars=%u\n", n, chars);
#endif

  if (chars == 3)
  {
    required_digits = 3;
  }
  else
  {
    required_digits = chars - 1;
  }

  // how many digits does n have?
  if (     n < 10UL)
  {
    num_digits = 1;
  }
  else if (n < 100UL)
  {
    num_digits = 2;
  }
  else if (n < 1000UL)
  {
    num_digits = 3;
  }
  else if (n < 10UL*1000UL)
  {
    num_digits = 4;
  }
  else if (n < 100UL*1000UL)
  {
    num_digits = 5;
  }
  else if (n < 1000UL*1000UL)
  {
    num_digits = 6;
  }
  else if (n < 10UL*1000UL*1000UL)
  {
    num_digits = 7;
  }
  else if (n < 100UL*1000UL*1000UL)
  {
    num_digits = 8;
  }
  else if (n < 1000UL*1000UL*1000UL)
  {
    num_digits = 9;
  }
  else
  {
    num_digits = 10;
  }

#ifdef DEBUG
  printf("num_digits=%u, required_digits=%u\n", num_digits, required_digits);
#endif

  if (num_digits < required_digits)
  {
    /* The decimal point goes after the last digit of n
       before the trailing zeros */
    exponent = 0;
    dp_position = num_digits;

    /* Left-align the number in the available space
       i.e. add trailing zeros */
    for (int8_t i = num_digits; i < required_digits; i++)
    {
      n *= 10;
    }
  }
  else
  {
    int8_t excess_digits;

    // Start with the decimal point after the last digit of n
    exponent = 0;
    dp_position = num_digits;

    // Some digits must be removed
    excess_digits = num_digits - required_digits;
    exponent += excess_digits;
    dp_position -= excess_digits;

    // Divide n by a power of 10 to get rid of the excess digits
    if (excess_digits > 0)
    {
      uint32_t divisor;
      divisor = 1;
      switch (excess_digits)
      {
      case 0: divisor = 1UL;                break;
      case 1: divisor = 10UL;               break;
      case 2: divisor = 100UL;              break;
      case 3: divisor = 1000UL;             break;
      case 4: divisor = 10UL*1000UL;        break;
      case 5: divisor = 100UL*1000UL;       break;
      case 6: divisor = 1000UL*1000UL;      break;
      case 7: divisor = 10UL*1000UL*1000UL; break;
      default:
        // overflow
        for (int8_t i = 0; i < chars; i++)
        {
          s[i] = '-';
        }
        s[chars] = 0;
        return 0;
      }
      n = (n + divisor/2) / divisor;
    }

#ifdef DEBUG
    printf("excess_digits=%d, ",
           excess_digits);
#endif
  }

  // Shift the decimal point to the left until there are
  // no more than 3 digits before the decimal point
  // and the exponent is a multiple of 3
  while ((dp_position > 3) || ((exponent % 3) != 0))
  {
    dp_position--;
    exponent++;
  }
  unit_steps = exponent / 3;

#ifdef DEBUG
    printf("n is now %u, dp_position=%d, exponent=%d, steps=%u\n",
           n, dp_position, exponent, unit_steps);
#endif

  // Convert to string
  for (int8_t i = 1; i <= required_digits; i++)
  {
    uint32_t next_n;
    next_n = n/10;
    s[required_digits - i] = '0' + (uint8_t)(n - next_n*10);
    n = next_n;
  }

#ifdef DEBUG
  printf("digits:'%.*s'\n", required_digits, s);
#endif

  if (dp_position < chars)
  {
    int8_t i;
    // Move the digits after the decimal point along
    // to make space for the decimal point itself
    for (i = required_digits; i > dp_position; i--)
    {
      s[i] = s[i-1];
    }
    s[i] = '.';
  }

  s[chars] = 0;

#ifdef DEBUG
  printf("result: %s/%u\n", s, unit_steps);
#endif

  return unit_steps;
}

