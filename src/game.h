#ifndef GAME_H_
#define GAME_H_

#include <stdio.h>
#include "graphics.h"
#include "util.h"

extern bitmap scene;
extern bool door_unlock;
extern int scene_index;

void game_init();
void game_activate();
void game_update();
void game_draw();
void game_reset();
void game_serialize(byte_buffer* buf);
void game_deserialize(byte_buffer* buf);

typedef enum { NORMAL, LEFT, RIGHT, DOWN, UP, LOCKED, TURN_LEFT, TURN_RIGHT } link_type;

typedef struct
{
  char* image_file;
  char* map_file;
  int links[7];
  link_type link_types[7];
} scene_data;

#endif
