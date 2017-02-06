#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <time.h>
#include "appstate.h"
#include "errors.h"
#include "types.h"
#include "input.h"
#include "menu.h"
#include "game.h"
#include "maze.h"

int main()
{
  char buffer[255];
  game_state current_state;

  printf("Starting game...");

  /* Initialize: */
  init_graphics();
  init_mouse();
  hide_mouse();

  printf("Loading assets...");

  game_init();
  menu_init();
  maze_init();

  new_state = PLAYING;
  current_state = new_state;

  if(access(SAVE_FILE, 0) == 0)
  {
    load();
  }

  game_activate();

  /* Game Loop: */
  while(current_state != STOPPED)
  {
    /* Poll Input: */
    poll_keyboard();
    poll_mouse();

    /* Update and Draw: */
    switch(current_state)
    {
      case PLAYING:
        {
          game_update();
          game_draw();
        } break;
      case MENU:
        {
          menu_update();
          menu_draw();
        } break;
      case MAZE:
        {
          maze_update();
          maze_draw();
        } break;
    }

    /* If the gamestate has changed, activate the new state: */
    if(new_state != current_state)
    {
      switch(new_state)
      {
        case PLAYING:
          {
            game_activate();
          } break;
        case MENU:
          {
            menu_activate();
          } break;
        case MAZE:
          {
            maze_activate();
          } break;
      }
      current_state = new_state;
    }

    /* Flush the new screen after VSync: */
    swap_buffers();
    color_update();
  }

  /* Cleanup: */
  cleanup_graphics();
  exit(0);
}
