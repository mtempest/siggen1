#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "LCD5110_Graph.h"

#include "fonts.h"

#include "ui.h"
#include "out.h"

#define LINE_1 0
#define LINE_2 24
#define LINE_3 32
#define LINE_4 40

LCD5110 myGLCD;

static void show_freq(void);
static void show_units(void);
static void show_on_off(void);
static void show_waveform(void);
static void show_parameter(void);

void UI_init(void)
{
  /* Set MOSI and SCK output */
  DDRB |= (1<<PB3)|(1<<PB5);

  /* Enable SPI, Master, CPOL=1, CPHA=1 */
  /* Also: MSB first, clock=fosc/4 */
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA) | 3;

  /* Set the pins for DC, RST and CS to output */
  DDRD |= (1<<PD7) | (1<<PD6) | (1<<PD5);

  myGLCD.InitLCD(0x46);

}

void UI_cyclic(void)
{
  myGLCD.setFont(SmallFont);
  show_units();
  show_on_off();
  show_waveform();
  show_parameter();

  myGLCD.setFont(BigNumbers);
  show_freq();

  myGLCD.update();
}

static void show_units(void)
{
  //TODO get the units
  //TODO put the strings in progmem
  char* s;
  if (1)
  {
    s = "kHz";
  }
  else
  {
    s = " Hz";
  }

  myGLCD.print(s, RIGHT, LINE_2);
}

static void show_on_off(void)
{
  //TODO get the on/off state
  //TODO put the strings in progmem
  char* s;
  if (1)
  {
    s = " ON ";
  }
  else
  {
    s = "off";
  }

  myGLCD.print(s, CENTER, LINE_3);
}

static void show_waveform(void)
{
  //TODO get the waveform state
  //TODO put the strings in progmem
  char* s;
  if (1)
  {
    s = "triangle";
  }
  else if (0)
  {
    s = "sine    ";
  }
  else
  {
    s = "square  ";
  }

  myGLCD.print(s, 0, LINE_2);
}

static void show_freq(void)
{
  //TODO Get the frequency
  myGLCD.print("123.45", CENTER, LINE_1);
}

static void show_parameter(void)
{
  //TODO Get the real parameter and value
  myGLCD.print("Param:Value", CENTER, LINE_4);
}
