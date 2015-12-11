#include "ui.h"
#include "out.h"

int main(void)
{
  UI_init();
  OUT_init();
  while (1)
  {
    UI_cyclic();
    OUT_cyclic();
  }
}
