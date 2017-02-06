#include <dos.h>
#include <conio.h>
#include "errors.h"
#include "input.h"

struct mouse_state mouse_state;
struct keyboard_state keyboard_state;

bool mouse_active;

bool has_mouse;
bool last_click;
int last_x;
int last_y;

void init_mouse()
{
  union REGS regs;

  regs.x.ax = 0;
  int86(0x33, &regs, &regs);
  has_mouse = regs.x.ax ? true : false;
  mouse_active = has_mouse;
  mouse_state.x_pos = 160;
  mouse_state.y_pos = 100;
  mouse_state.clicked = false;
}

void hide_mouse()
{
  union REGS regs;

  if(has_mouse)
  {
    regs.x.ax = 2;
    int86(0x33, &regs, &regs);
  }
}

void poll_mouse()
{
  union REGS regs;
  bool click;
  int x, y;

  if(has_mouse)
  {
    regs.x.ax = 3;
    int86(0x33, &regs, &regs);
    x = regs.x.cx >> 1;
    y = regs.x.dx;
    if(x != last_x || y != last_y)
    {
      last_x = x;
      last_y = y;
      mouse_state.x_pos = x;
      mouse_state.y_pos = y;
      mouse_active = true;
    }
    mouse_state.clicked = false;
    click = (regs.x.bx & 1);
    if(last_click != click)
    {
      last_click = click;
      if(click)
      {
        mouse_state.clicked = true;
      }
      mouse_active = true;
    }
  }
}

void poll_keyboard()
{
  if(kbhit())
  {
    keyboard_state.key_code = getch();
  }
  else
  {
    keyboard_state.key_code = 0;
  }
}
