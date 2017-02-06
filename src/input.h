#ifndef INPUT_H_
#define INPUT_H_

#include "types.h"

extern bool mouse_active;

extern struct mouse_state
{
  int x_pos;
  int y_pos;
  bool clicked;
} mouse_state;

extern struct keyboard_state
{
  byte key_code;
} keyboard_state;

void init_mouse();
void hide_mouse();
void poll_mouse();
void poll_keyboard();

#endif
