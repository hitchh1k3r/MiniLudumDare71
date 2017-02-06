#include <io.h>
#include "menu.h"
#include "appstate.h"
#include "input.h"
#include "graphics.h"
#include "game.h"
#include "maze.h"
#include "util.h"

#define SERIAL_VERSION 2;

int selection = 0;
int last_selection = 0;
bool can_load = false;
bool can_save = false;

void menu_init()
{
}

void menu_activate()
{
  can_load = (access(SAVE_FILE, 0) == 0);
  can_save = (scene_index != 18);
  last_selection = 0;
  selection = mouse_active ? 0 : 1;
  attenuate_pallet(3, 5);
  stamp_bitmap(&composer, &scene, 0, 0);

  if(rand() % 2 == 0)
  {
    draw_scaled_string(&composer, "Virtual Escape", 5 + 7, 5 + 7, 2, COLOR_BLACK);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 6, 5 + 6, 2, COLOR_DISABLED);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 5, 5 + 5, 2, COLOR_BLACK);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 4, 5 + 4, 2, COLOR_GRAY);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 2, 5 + 2, 2, COLOR_BLACK);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 0, 5 + 0, 2, COLOR_WHITE);
  }
  else
  {
    draw_scaled_string(&composer, "Virtual Escape", 5 + 7, 5 + 7, 2, COLOR_BLACK);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 5, 5 + 5, 2, COLOR_CHROMA_2);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 3, 5 + 3, 2, COLOR_CHROMA_1);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 1, 5 + 1, 2, COLOR_SELECT);
    draw_scaled_string(&composer, "Virtual Escape", 5 + 0, 5 + 0, 2, COLOR_WHITE);
  }

  draw_string(&composer, "Continue", 15, 114, COLOR_GRAY);
  draw_string(&composer, "New Game", 15, 129, COLOR_GRAY);
  draw_string(&composer, "Save",     15, 144, can_save ? COLOR_GRAY : COLOR_DISABLED);
  draw_string(&composer, "Load",     15, 159, can_load ? COLOR_GRAY : COLOR_DISABLED);
  draw_string(&composer, "Quit",     15, 174, COLOR_GRAY);
}

void cont_game()
{
  new_state = PLAYING;
}

void new_game()
{
  game_reset();
  maze_reset();
  new_state = PLAYING;
}

void save()
{
  byte serial_version;
  byte_buffer buf;
  FILE* file;
  dword hash;

  buf.cursor = 0;
  serial_version = SERIAL_VERSION;

  if((file = fopen(SAVE_FILE, "wb")) == NULL)
  {
    return;
  }

  game_serialize(&buf);
  maze_serialize(&buf);

  byte_buffer_add(&buf, &serial_version, sizeof(byte));
  hash = adler32(buf.data, buf.cursor);

  fwrite(&buf.cursor, sizeof(byte),  1,          file);
  fwrite(buf.data,    sizeof(byte),  buf.cursor, file);
  fwrite(&hash,       sizeof(dword), 1,          file);

  fclose(file);

  new_state = PLAYING;
}

void load()
{
  byte_buffer buf;
  FILE* file;
  dword hash;
  byte data_length = 0;
  byte serial_version;

  buf.cursor = 0;
  serial_version = SERIAL_VERSION;

  if((file = fopen(SAVE_FILE, "rb")) == NULL)
  {
    return;
  }

  fread(&data_length, sizeof(byte),  1,           file);
  fread(&buf.data,    sizeof(byte),  data_length, file);
  fread(&hash,        sizeof(dword), 1,           file);

  fclose(file);

  buf.cursor = data_length - 1;
  byte_buffer_add(&buf, &serial_version, sizeof(byte));
  if(hash != adler32(&buf.data, data_length))
  {
    return;
  }

  buf.cursor = 0;
  game_deserialize(&buf);
  maze_deserialize(&buf);

  new_state = PLAYING;
}

void quit()
{
  new_state = STOPPED;
}

void menu_update()
{
  if(keyboard_state.key_code != 0)
  {
    if(keyboard_state.key_code == 27) /* ESC */
    {
      cont_game();
    }
    else if(keyboard_state.key_code == 99 || /* c */
            keyboard_state.key_code == 67)   /* C */
    {
      cont_game();
    }
    else if(keyboard_state.key_code == 110 || /* n */
            keyboard_state.key_code ==  78)   /* N */
    {
      new_game();
    }
    else if(keyboard_state.key_code == 115 || /* s */
            keyboard_state.key_code ==  83)   /* S */
    {
      if(can_save)
      {
        save();
      }
    }
    else if(keyboard_state.key_code == 108 || /* l */
            keyboard_state.key_code ==  76)   /* L */
    {
      if(can_load)
      {
        load();
      }
    }
    else if(keyboard_state.key_code == 113 || /* q */
            keyboard_state.key_code ==  81)   /* Q */
    {
      quit();
    }
    else if(keyboard_state.key_code == 72) /* UP */
    {
      selection += 3;
      selection %= 5;
      ++selection;
      if(selection == 4 && !can_load)
      {
        selection = 3;
      }
      if(selection == 3 && !can_save)
      {
        selection = 2;
      }
      mouse_active = false;
    }
    else if(keyboard_state.key_code == 80) /* DOWN */
    {
      selection %= 5;
      ++selection;
      if(selection == 3 && !can_save)
      {
        selection = 4;
      }
      if(selection == 4 && !can_load)
      {
        selection = 5;
      }
      mouse_active = false;
    }
  }

  if(mouse_active)
  {
    if(mouse_state.x_pos >  13 && mouse_state.x_pos < 89 &&
       mouse_state.y_pos > 112 && mouse_state.y_pos < 187)
    {
      selection = ((mouse_state.y_pos - 112) / 15) + 1;
    }
    else
    {
      selection = 0;
    }
  }
  if(selection == 3 && !can_save)
  {
    selection = 0;
  }
  if(selection == 4 && !can_load)
  {
    selection = 0;
  }

  if(selection > 0 && (mouse_state.clicked ||
                         keyboard_state.key_code == 32 || /* SPACE */
                         keyboard_state.key_code == 13))  /* ENTER */
  {
    switch(selection)
    {
      case 1:
        {
          cont_game();
        } break;
      case 2:
        {
          new_game();
        } break;
      case 3:
        {
          save();
        } break;
      case 4:
        {
          load();
        } break;
      case 5:
        {
          quit();
        } break;
    }
  }

  if(last_selection != selection)
  {
    switch(last_selection)
    {
      case 1:
        {
          draw_string(&composer, "Continue", 15, 114, COLOR_GRAY);
        } break;
      case 2:
        {
          draw_string(&composer, "New Game", 15, 129, COLOR_GRAY);
        } break;
      case 3:
        {
          draw_string(&composer, "Save",     15, 144, can_save ? COLOR_GRAY : COLOR_DISABLED);
        } break;
      case 4:
        {
          draw_string(&composer, "Load",     15, 159, can_load ? COLOR_GRAY : COLOR_DISABLED);
        } break;
      case 5:
        {
          draw_string(&composer, "Quit",     15, 174, COLOR_GRAY);
        } break;
    }
    switch(selection)
    {
      case 1:
        {
          draw_string(&composer, "Continue", 15, 114, COLOR_SELECT);
        } break;
      case 2:
        {
          draw_string(&composer, "New Game", 15, 129, COLOR_SELECT);
        } break;
      case 3:
        {
          draw_string(&composer, "Save",     15, 144, COLOR_SELECT);
        } break;
      case 4:
        {
          draw_string(&composer, "Load",     15, 159, COLOR_SELECT);
        } break;
      case 5:
        {
          draw_string(&composer, "Quit",     15, 174, COLOR_SELECT);
        } break;
    }
    last_selection = selection;
  }
}

void menu_draw()
{
  bitmap_blit(&composer, 0, 0);

  if(mouse_active)
  {
    draw_bordered_icon(&hand, mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
  }
}
