#ifndef MAZE_H_
#define MAZE_H_

#include "graphics.h"
#include "util.h"

#define MAZE_FILE "maze.txt"

extern bool user_maze;

void maze_init();
void maze_activate();
void maze_update();
void maze_draw();
void maze_reset();
void maze_serialize(byte_buffer* buf);
void maze_deserialize(byte_buffer* buf);

#endif
