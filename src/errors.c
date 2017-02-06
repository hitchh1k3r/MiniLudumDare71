#include <conio.h>
#include <stdio.h> /* TODO (hitch) is this include needed? */
#include "errors.h"
#include "graphics.h"

void crash(char* err)
{
  cleanup_graphics();

  /* Display error message: */
  screen(TEXT_MODE);
  printf("ERROR: %s\n", err);

  /* Consume hanging keypress events: */
  while(kbhit())
  {
    getch();
  }

  /* Wait for key and exit: */
  puts("Press [ANY KEY] to quit!");
  getch();
  exit(1);
}
