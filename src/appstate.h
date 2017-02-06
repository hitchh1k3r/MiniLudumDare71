#ifndef APPSTATE_H_
#define APPSTATE_H_

#include "types.h"

typedef enum { STOPPED, PLAYING, MENU, MAZE } game_state;
extern game_state new_state;

#endif