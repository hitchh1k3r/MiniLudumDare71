#include <io.h>
#include <stdio.h>
#include <alloc.h>
#include "appstate.h"
#include "game.h"
#include "input.h"
#include "maze.h"
#include "errors.h"
#include "types.h"

#define NORTH_IMG "SCENES/IMG/DIR/NORTH.BMP"
#define EAST_IMG "SCENES/IMG/DIR/EAST.BMP"
#define SOUTH_IMG "SCENES/IMG/DIR/SOUTH.BMP"
#define WEST_IMG "SCENES/IMG/DIR/WEST.BMP"

#define add_tile(I, NAM, VAR, F, L, T, SX, SY) \
  load_bmp("SCENES/OVERLAY/" #NAM ".BMP", &VAR, OVERLAY); \
  tile.img       = &VAR; \
  tile.scr_x     = SX; \
  tile.scr_y     = SY; \
  tile.left      = L; \
  tile.front     = F; \
  tile.type      = T; \
  tiles[I] = tile;

bitmap key_tr, key_sq;

bitmap base, key_1_0, key_1_1, key_2_0, key_2_1, key_3_0, key_3_1, key_3_2,
       wall_0_1, wall_1_0, wall_1_1, wall_2_0, wall_2_1, wall_2_2, wall_3_0, wall_3_1, wall_3_2, wall_3_3, wall_3_4, wall_4,
       door_t_1_0, door_t_1_1, door_t_2_0, door_t_2_1, door_t_3_0, door_t_3_1, door_t_3_2, door_t_4,
       door_s_1_0, door_s_1_1, door_s_2_0, door_s_2_1, door_s_3_0, door_s_3_1, door_s_3_2, door_s_4,
       door_0_1, door_2_2, door_3_3,
       exit_0_0, exit_0_1, exit_1_0, exit_1_1, exit_2_0, exit_2_1, exit_3_0, exit_3_1, exit_3_2;

typedef enum { EMPTY, WALL, DOOR, DOOR_S, DOOR_T, KEY_S, KEY_T, KEY_EMPTY, EXIT } tile_type;

typedef struct {
  bitmap* img;
  int scr_x, scr_y;
  int left, front;
  tile_type type;
  bool has_trans;
} tile_definition;

tile_definition tiles[47];

#define MAX_LEVELS 8
char* mazes[MAX_LEVELS] = {
  "WW\nWE\nW \nWP",
  "W WWWW\nW WW  \nW    W\nWPWW W\nWWE   ",
  "W WE\nW W \nW W \nW W \n    \nW W \nW W \nWPW ",
  "WWW\nW E\nWSs\nW  \nWWP",
  "WWWWWW\nW   TE\nW WSss\nW W   \nWtW P ",
  "WE\nWS\nWT\nW \ns \nW \nWP\nWt",
  "sWEWtWtWsW\nSWSWSWSWSW\n         t\nTWTW WTWTW\nsWtW WsWsW\ntWsW WsWtW\nSWSWSWSWSW\n         s\nTWTWPWTWTW\nsWsWWWtWsW",
  "W W\nW W\nWEW\nWsW\nWSW\nWPW"
};
char* level_names[MAX_LEVELS] = {
  "Hello World",
  "Maze",
  "Endless",
  "One Door",
  "Two Doors",
  "Droste",
  "Kicking In Doors",
  "The Final Door"
};
tile_type* map = NULL;
int map_width;
int map_height;
int can_interact = 0;
int hover;
bool user_maze;
bool scene_dirty;
bool win = false;

byte pos_x;
byte pos_y;
byte face;
byte keys_sq;
byte keys_tr;

/* Serializable mazestate: */
byte level;

void update_flags()
{
  tile_type tile;
  int n_x = pos_x;
  int n_y = pos_y;

  can_interact = 0;
  if(win)
  {
    return;
  }

  if(face == 1)
  {
    n_x += 1;
  }
  else if(face == 2)
  {
    n_y += 1;
  }
  else if(face == 3)
  {
    n_x += map_width - 1;
  }
  else
  {
    n_y += map_height - 1;
  }

  n_x %= map_width;
  n_y %= map_height;

  tile = map[n_x + (n_y * map_width)];

  if(tile == KEY_S || tile == KEY_T)
  {
    can_interact = 1;
  }
  else if(tile == EMPTY || tile == EXIT)
  {
    can_interact = 3;
  }
  else if(tile == DOOR_S || tile == DOOR_T)
  {
    if((tile == DOOR_S && keys_sq > 0) || (tile == DOOR_T && keys_tr > 0))
    {
      can_interact = 1;
    }
    else
    {
      can_interact = 2;
    }
  }
}

tile_type char_to_enum(char c)
{
  switch(c)
  {
    case 'W':
      return WALL;
    case 'S':
      return DOOR_S;
    case 's':
      return KEY_S;
    case 'T':
      return DOOR_T;
    case 't':
      return KEY_T;
    case 'E':
      return EXIT;
  }
  return EMPTY;
}

void load_maze_data(char* data)
{
  int r, c;
  int w = 0, h = 0;
  int cw;
  char* dr;

  dr = data;
  while(*dr != '\0')
  {
    cw = 0;
    for(; *dr != '\0' && *dr != '\n'; dr++)
    {
      if(*dr != '\r')
      {
        ++cw;
      }
    }
    if(cw > 0)
    {
      if(cw > w)
      {
        w = cw;
      }
      ++h;
    }
    if(*dr != '\0')
    {
      dr++;
    }
  }

  if(map != NULL)
  {
    free(map);
  }
  if((map = (tile_type*)malloc(w * h * sizeof(tile_type))) == NULL)
  {
    crash("Not enough memory to load Maze!");
  }

  map_width =  w;
  map_height = h;
  dr = data;
  for(r = 0; *dr != '\0' && r < h; ++r)
  {
    for(c = 0; c < w && *dr != '\0' && *dr != '\n'; dr++, ++c)
    {
      map[c + (r * w)] = char_to_enum(*dr);
      if(*dr == 'P')
      {
        pos_x = c;
        pos_y = r;
      }
    }
    for(; c < w; ++c)
    {
      map[c + (r * w)] = EMPTY;
    }
    if(*dr == '\r')
    {
      ++dr;
    }
    if(*dr != '\0')
    {
      ++dr;
    }
  }
}

void load_maze()
{
  bool maze_loaded = false;
  char* f_buf = NULL;
  long f_len;
  FILE* fp;

  pos_x = 0;
  pos_y = 0;
  face = 0;
  keys_sq = 0;
  keys_tr = 0;
  user_maze = false;

  if(access(MAZE_FILE, 0) == 0 && (fp = fopen(MAZE_FILE, "rb")) != NULL)
  {
    fseek(fp, 0, SEEK_END);
    f_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    f_buf = malloc(f_len+1);
    if(f_buf != NULL)
    {
      fread(f_buf, 1, f_len, fp);
    }
    f_buf[f_len] = '\0';
    fclose(fp);

    if(f_buf != NULL)
    {
      load_maze_data(f_buf);
      maze_loaded = true;
      user_maze = true;
    }
  }

  if(!maze_loaded)
  {
    if(level >= MAX_LEVELS)
    {
      load_maze_data(mazes[MAX_LEVELS - 1]);
    }
    else
    {
      load_maze_data(mazes[level]);
    }
  }
}

void maze_reset()
{
  level = 0;
  load_maze();
}

void maze_serialize(byte_buffer* buf)
{
  byte_buffer_add(buf, &level, sizeof(byte));
}

void maze_deserialize(byte_buffer* buf)
{
  byte_buffer_get(buf, &level, sizeof(byte));
  load_maze();
}

void maze_init()
{
  tile_definition tile;

  load_bmp("SCENES/OVERLAY/BASE.BMP", &base, OVERLAY);

  load_bmp("UI/KEY_SQ.BMP", &key_sq, ICON);
  load_bmp("UI/KEY_TR.BMP", &key_tr, ICON);

  add_tile( 0, 1KEY0, key_1_0, 1, 0, KEY_EMPTY, 103, 139);
  add_tile( 1, 1KEY1, key_1_1, 1, 1, KEY_EMPTY,   0, 139);
  add_tile( 2, 2KEY0, key_2_0, 2, 0, KEY_EMPTY, 126, 127);
  add_tile( 3, 2KEY1, key_2_1, 2, 1, KEY_EMPTY,   0, 126);
  add_tile( 4, 3KEY0, key_3_0, 3, 0, KEY_EMPTY, 141, 114);
  add_tile( 5, 3KEY1, key_3_1, 3, 1, KEY_EMPTY,  63, 114);
  add_tile( 6, 3KEY2, key_3_2, 3, 2, KEY_EMPTY,   0, 114);

  add_tile( 7, 0WALL1, wall_0_1, 0,  1, WALL,   0,  0);
  add_tile( 8, 1WALL0, wall_1_0, 1,  0, WALL,  27, 17);
  add_tile( 9, 1WALL1, wall_1_1, 1,  1, WALL,   0, 17);
  add_tile(10, 2WALL0, wall_2_0, 2,  0, WALL,  79, 49);
  add_tile(11, 2WALL1, wall_2_1, 2,  1, WALL,   0, 49);
  add_tile(12, 2WALL2, wall_2_2, 2,  2, WALL,   0, 67);
  add_tile(13, 3WALL0, wall_3_0, 3,  0, WALL, 114, 71);
  add_tile(14, 3WALL1, wall_3_1, 3,  1, WALL,  21, 71);
  add_tile(15, 3WALL2, wall_3_2, 3,  2, WALL,   0, 71);
  add_tile(16, 3WALL3, wall_3_3, 3,  3, WALL,   0, 80);
  add_tile(17, 3WALL4, wall_3_4, 3,  4, WALL,   0, 85);
  add_tile(18, 4WALL,  wall_4,   4, -1, WALL, 138, 86);

  add_tile(19, 0DOOR1, door_0_1, 0, 1, DOOR, 0,  0);
  add_tile(20, 2DOOR2, door_2_2, 2, 2, DOOR, 0, 67);
  add_tile(21, 3DOOR3, door_3_3, 3, 3, DOOR, 0, 80);

  add_tile(22, 1DOOR0_S, door_s_1_0, 1,  0, DOOR_S,  27, 17);
  add_tile(23, 1DOOR1_S, door_s_1_1, 1,  1, DOOR_S,   0, 17);
  add_tile(24, 2DOOR0_S, door_s_2_0, 2,  0, DOOR_S,  79, 49);
  add_tile(25, 2DOOR1_S, door_s_2_1, 2,  1, DOOR_S,   0, 49);
  add_tile(26, 3DOOR0_S, door_s_3_0, 3,  0, DOOR_S, 114, 71);
  add_tile(27, 3DOOR1_S, door_s_3_1, 3,  1, DOOR_S,  21, 71);
  add_tile(28, 3DOOR2_S, door_s_3_2, 3,  2, DOOR_S,   0, 71);
  add_tile(29, 4DOOR_S,  door_s_4,   4, -1, DOOR_S, 138, 86);

  add_tile(30, 1DOOR0_T, door_t_1_0, 1,  0, DOOR_T,  27, 17);
  add_tile(31, 1DOOR1_T, door_t_1_1, 1,  1, DOOR_T,   0, 17);
  add_tile(32, 2DOOR0_T, door_t_2_0, 2,  0, DOOR_T,  79, 49);
  add_tile(33, 2DOOR1_T, door_t_2_1, 2,  1, DOOR_T,   0, 49);
  add_tile(34, 3DOOR0_T, door_t_3_0, 3,  0, DOOR_T, 114, 71);
  add_tile(35, 3DOOR1_T, door_t_3_1, 3,  1, DOOR_T,  21, 71);
  add_tile(36, 3DOOR2_T, door_t_3_2, 3,  2, DOOR_T,   0, 71);
  add_tile(37, 4DOOR_T,  door_t_4,   4, -1, DOOR_T, 138, 86);

  add_tile(38, 0EXIT0,   exit_0_0,   0,  0, EXIT,   1, 183);
  add_tile(39, 0EXIT1,   exit_0_1,   0,  1, EXIT,   0, 183);
  add_tile(40, 1EXIT0,   exit_1_0,   1,  0, EXIT,  30, 151);
  add_tile(41, 1EXIT1,   exit_1_1,   1,  1, EXIT,   0, 151);
  add_tile(42, 2EXIT0,   exit_2_0,   2,  0, EXIT,  82, 129);
  add_tile(43, 2EXIT1,   exit_2_1,   2,  1, EXIT,   0, 129);
  add_tile(44, 3EXIT0,   exit_3_0,   3,  0, EXIT, 117, 114);
  add_tile(45, 3EXIT1,   exit_3_1,   3,  1, EXIT,  29, 114);
  add_tile(46, 3EXIT2,   exit_3_2,   3,  2, EXIT,   0, 114);

  maze_reset();
}

void overlay_tile(tile_type type, int forward, int left)
{
  int i;
  int match;
  int nearness;
  int length;
  bool flip;

  if(type != EMPTY)
  {
    flip = false;
    nearness = 3; /* Nearness Scale: 0 exact, 1 generic form, 2 similar object, 3 no match */
    if(left < 0)
    {
      left = -left;
      flip = true;
    }
    length = sizeof(tiles) / sizeof(tiles[0]);

    for(i = 0; i < length; ++i)
    {
      if(tiles[i].front == forward && (tiles[i].left == left || tiles[i].left == -1))
      {
        if(type == tiles[i].type)
        {
          match = i;
          nearness = 0;
          break;
        }
        else if(type == DOOR_S || type == DOOR_T)
        {
          if(nearness > 1 && tiles[i].type == DOOR)
          {
            match = i;
            nearness = 1;
          }
          else if(nearness > 2 && tiles[i].type == WALL)
          {
            match = i;
            nearness = 2;
          }
        }
        else if(type == KEY_S || type ==  KEY_T || type ==  KEY_EMPTY)
        {
          if(tiles[i].type == KEY_EMPTY)
          {
            match = i;
            nearness = 1;
            break;
          }
        }
      }
    }

    if(nearness < 3)
    {
      i = tiles[match].scr_x;
      if(tiles[match].left == -1)
      {
        i -= left * (tiles[match].img->width-1);
      }
      if(flip)
      {
        stamp_overlay(&composer, tiles[match].img, DISPLAY_WIDTH - i - 1, tiles[match].scr_y, true);
      }
      else
      {
        stamp_overlay(&composer, tiles[match].img, i, tiles[match].scr_y, false);
      }

      if(type == KEY_S || type == KEY_T)
      {
        if(forward == 1 && left == 0)
        {
          stamp_scale_icon(&composer, type == KEY_S ? &key_sq : &key_tr, 132, 116, 3);
        }
        else if(forward == 2 && left == 0)
        {
          stamp_scale_icon(&composer, type == KEY_S ? &key_sq : &key_tr, 141, 109, 2);
        }
        else if(forward == 2 && left == 1)
        {
          stamp_scale_icon(&composer, type == KEY_S ? &key_sq : &key_tr, (flip ? DISPLAY_WIDTH - 49 : 13), 109, 2);
        }
        else if(forward == 3 && left == 0)
        {
          stamp_icon(&composer, type == KEY_S ? &key_sq : &key_tr, 150, 106);
        }
        else if(forward == 3 && left == 1)
        {
          stamp_icon(&composer, type == KEY_S ? &key_sq : &key_tr, (flip ? DISPLAY_WIDTH - 100 : 82), 106);
        }
        else if(forward == 3 && left == 2)
        {
          stamp_icon(&composer, type == KEY_S ? &key_sq : &key_tr, (flip ? DISPLAY_WIDTH - 32 : 14), 106);
        }
      }
    }
  }
}

void overlay_from_map(int map_x, int map_y, int forward, int left)
{
  while(map_x < 0)
  {
    map_x += map_width;
  }
  while(map_x >= map_width)
  {
    map_x -= map_width;
  }
  while(map_y < 0)
  {
    map_y += map_height;
  }
  while(map_y >= map_height)
  {
    map_y -= map_height;
  }

  overlay_tile(map[map_x + (map_y * map_width)], forward, left);
}

void draw_maze()
{
  int i, xl, yl, xf, yf;

  if(scene_dirty)
  {
    scene_dirty = false;
    if(face == 1)
    {
      load_bmp(EAST_IMG, &scene, PHOTO);
    }
    else if(face == 2)
    {
      load_bmp(SOUTH_IMG, &scene, PHOTO);
    }
    else if(face == 3)
    {
      load_bmp(WEST_IMG, &scene, PHOTO);
    }
    else
    {
      load_bmp(NORTH_IMG, &scene, PHOTO);
    }
    attenuate_pallet(1, 5);
  }

  stamp_overlay(&composer, &base, 0, 0, false);

  if(face == 1)
  {
    xl =  0;
    yl = -1;
    xf =  1;
    yf =  0;
  }
  else if(face == 2)
  {
    xl =  1;
    yl =  0;
    xf =  0;
    yf =  1;
  }
  else if(face == 3)
  {
    xl =  0;
    yl =  1;
    xf = -1;
    yf =  0;
  }
  else
  {
    xl = -1;
    yl =  0;
    xf =  0;
    yf = -1;
  }

  overlay_from_map(pos_x - (4 * xl) + (4 * xf), pos_y - (4 * yl) + (4 * yf), 4, -4);
  overlay_from_map(pos_x + (4 * xl) + (4 * xf), pos_y + (4 * yl) + (4 * yf), 4,  4);
  overlay_from_map(pos_x - (3 * xl) + (4 * xf), pos_y - (3 * yl) + (4 * yf), 4, -3);
  overlay_from_map(pos_x + (3 * xl) + (4 * xf), pos_y + (3 * yl) + (4 * yf), 4,  3);
  overlay_from_map(pos_x - (2 * xl) + (4 * xf), pos_y - (2 * yl) + (4 * yf), 4, -2);
  overlay_from_map(pos_x + (2 * xl) + (4 * xf), pos_y + (2 * yl) + (4 * yf), 4,  2);
  overlay_from_map(pos_x - (1 * xl) + (4 * xf), pos_y - (1 * yl) + (4 * yf), 4, -1);
  overlay_from_map(pos_x + (1 * xl) + (4 * xf), pos_y + (1 * yl) + (4 * yf), 4,  1);
  overlay_from_map(pos_x +            (4 * xf), pos_y +            (4 * yf), 4,  0);

  overlay_from_map(pos_x - (4 * xl) + (3 * xf), pos_y - (4 * yl) + (3 * yf), 3, -4);
  overlay_from_map(pos_x + (4 * xl) + (3 * xf), pos_y + (4 * yl) + (3 * yf), 3,  4);
  overlay_from_map(pos_x - (3 * xl) + (3 * xf), pos_y - (3 * yl) + (3 * yf), 3, -3);
  overlay_from_map(pos_x + (3 * xl) + (3 * xf), pos_y + (3 * yl) + (3 * yf), 3,  3);
  overlay_from_map(pos_x - (2 * xl) + (3 * xf), pos_y - (2 * yl) + (3 * yf), 3, -2);
  overlay_from_map(pos_x + (2 * xl) + (3 * xf), pos_y + (2 * yl) + (3 * yf), 3,  2);
  overlay_from_map(pos_x - (1 * xl) + (3 * xf), pos_y - (1 * yl) + (3 * yf), 3, -1);
  overlay_from_map(pos_x + (1 * xl) + (3 * xf), pos_y + (1 * yl) + (3 * yf), 3,  1);
  overlay_from_map(pos_x +            (3 * xf), pos_y +            (3 * yf), 3,  0);

  overlay_from_map(pos_x - (2 * xl) + (2 * xf), pos_y - (2 * yl) + (2 * yf), 2, -2);
  overlay_from_map(pos_x + (2 * xl) + (2 * xf), pos_y + (2 * yl) + (2 * yf), 2,  2);
  overlay_from_map(pos_x - (1 * xl) + (2 * xf), pos_y - (1 * yl) + (2 * yf), 2, -1);
  overlay_from_map(pos_x + (1 * xl) + (2 * xf), pos_y + (1 * yl) + (2 * yf), 2,  1);
  overlay_from_map(pos_x +            (2 * xf), pos_y +            (2 * yf), 2,  0);

  overlay_from_map(pos_x - (1 * xl) + (1 * xf), pos_y - (1 * yl) + (1 * yf), 1, -1);
  overlay_from_map(pos_x + (1 * xl) + (1 * xf), pos_y + (1 * yl) + (1 * yf), 1,  1);
  overlay_from_map(pos_x +            (1 * xf), pos_y +            (1 * yf), 1,  0);

  overlay_from_map(pos_x - (1 * xl)           , pos_y - (1 * yl)           , 0, -1);
  overlay_from_map(pos_x + (1 * xl)           , pos_y + (1 * yl)           , 0,  1);
  overlay_from_map(pos_x                      , pos_y                      , 0,  0);

  for(i = 0; i < keys_sq; ++i)
  {
    stamp_icon(&composer, &key_sq, DISPLAY_WIDTH - 21 - keys_sq - i, 3 + (2 * (keys_sq - i)));
  }

  for(i = 0; i < keys_tr; ++i)
  {
    stamp_icon(&composer, &key_tr, DISPLAY_WIDTH - 21 - keys_tr - i, 3 + (keys_sq > 0 ? (11 + (2 * keys_sq)) : 0) + (2 * (keys_tr - i)));
  }

  if(win)
  {
    if(user_maze)
    {
      draw_string(&composer, "You Win", (DISPLAY_WIDTH - get_string_width("You Win")) / 2, (DISPLAY_HEIGHT / 2) - 13, COLOR_SELECT);
    }
    else
    {
      draw_string(&composer, "Door Unlocked", (DISPLAY_WIDTH - get_string_width("Door Unlocked")) / 2, (DISPLAY_HEIGHT / 2) - 13, COLOR_SELECT);
    }
    draw_string(&composer, "Press Escape", (DISPLAY_WIDTH - get_string_width("Press Escape")) / 2, (DISPLAY_HEIGHT / 2) + 2, COLOR_SELECT);
  }
  else if(user_maze)
  {
    draw_string(&composer, "MAZE TXT", (DISPLAY_WIDTH - get_string_width("MAZE TXT")) / 2, 2, COLOR_SELECT);
  }
  else
  {
    draw_string(&composer, level_names[level], (DISPLAY_WIDTH - get_string_width(level_names[level])) / 2, 2, COLOR_SELECT);
  }

  bitmap_underlay(&composer, &scene, 0, 0);
}

void maze_activate()
{
  win = false;
  hover = 0;
  scene_dirty = true;

  if(level >= MAX_LEVELS || user_maze)
  {
    load_maze();
  }

  update_flags();
  draw_maze();
}

void move_forward()
{
  tile_type tile;
  int n_x = pos_x;
  int n_y = pos_y;

  if(win)
  {
    return;
  }

  if(face == 1)
  {
    n_x += 1;
  }
  else if(face == 2)
  {
    n_y += 1;
  }
  else if(face == 3)
  {
    n_x += map_width - 1;
  }
  else
  {
    n_y += map_height - 1;
  }

  n_x %= map_width;
  n_y %= map_height;

  tile = map[n_x + (n_y * map_width)];

  if(tile == EMPTY)
  {
    pos_x = n_x;
    pos_y = n_y;
  }

  if(tile == EXIT)
  {
    ++level;
    if(level < MAX_LEVELS && !user_maze)
    {
      load_maze();
    }
    else
    {
      pos_x = n_x;
      pos_y = n_y;
      win = true;
      door_unlock = true;
    }
  }

  update_flags();
}

void turn_left()
{
  if(win)
  {
    return;
  }

  face += 3;
  face %= 4;
  scene_dirty = true;
  update_flags();
}

void turn_right()
{
  if(win)
  {
    return;
  }

  face += 1;
  face %= 4;
  scene_dirty = true;
  update_flags();
}

void interact()
{
  tile_type tile;
  int n_x = pos_x;
  int n_y = pos_y;

  if(win)
  {
    return;
  }

  if(can_interact > 0)
  {
    if(face == 1)
    {
      n_x += 1;
    }
    else if(face == 2)
    {
      n_y += 1;
    }
    else if(face == 3)
    {
      n_x += map_width - 1;
    }
    else
    {
      n_y += map_height - 1;
    }

    n_x %= map_width;
    n_y %= map_height;

    tile = map[n_x + (n_y * map_width)];

    if(tile == KEY_S || tile == KEY_T)
    {
      if(tile == KEY_S)
      {
        ++keys_sq;
      }
      else
      {
        ++keys_tr;
      }
      map[n_x + (n_y * map_width)] = KEY_EMPTY;
    }
    else if((tile == DOOR_S && keys_sq > 0) || (tile == DOOR_T && keys_tr > 0))
    {
      if(tile == DOOR_S)
      {
        --keys_sq;
      }
      else
      {
        --keys_tr;
      }
      map[n_x + (n_y * map_width)] = EMPTY;
    }
  }

  update_flags();
}

void maze_update()
{
  if(keyboard_state.key_code != 0)
  {
    if(keyboard_state.key_code == 27) /* ESC */
    {
      new_state = PLAYING;
    }
    else
    {
      if(keyboard_state.key_code == 119 || /* w */
         keyboard_state.key_code ==  87 || /* W */
         keyboard_state.key_code ==  72)   /* UP */
      {
        move_forward();
        draw_maze();
        mouse_active = false;
      }
      else if(keyboard_state.key_code == 97 || /* a */
              keyboard_state.key_code == 65 || /* A */
              keyboard_state.key_code == 75)   /* LEFT */
      {
        turn_left();
        draw_maze();
        mouse_active = false;
      }
      else if(keyboard_state.key_code == 100 || /* d */
              keyboard_state.key_code ==  68 || /* D */
              keyboard_state.key_code ==  77)   /* RIGHT */
      {
        turn_right();
        draw_maze();
        mouse_active = false;
      }
      else if(keyboard_state.key_code == 32 || /* SPACE */
              keyboard_state.key_code == 13)   /* ENTER */
      {
        interact();
        draw_maze();
        mouse_active = false;
      }
    }
  }
  if(mouse_active && mouse_state.clicked)
  {
    if(mouse_state.x_pos < 60)
    {
      turn_left();
      draw_maze();
    }
    else if(mouse_state.x_pos > DISPLAY_WIDTH - 60)
    {
      turn_right();
      draw_maze();
    }
    else if(mouse_state.y_pos > 60 && mouse_state.y_pos < DISPLAY_HEIGHT - 60 && can_interact > 0)
    {
      if(can_interact == 3)
      {
        move_forward();
      }
      else
      {
        interact();
      }
      draw_maze();
    }
  }
}

void maze_draw()
{
  bitmap_blit(&composer, 0, 0);

  if(mouse_active && !win)
  {
    if(mouse_state.x_pos < 60)
    {
      draw_bordered_icon(&hand_left,   mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
    }
    else if(mouse_state.x_pos > DISPLAY_WIDTH - 60)
    {
      draw_bordered_icon(&hand_right,  mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
    }
    else if(mouse_state.y_pos > 60 && mouse_state.y_pos < DISPLAY_HEIGHT - 60 && can_interact > 0)
    {
      if(can_interact == 1)
      {
        draw_bordered_icon(&hand,      mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_SELECT);
      }
      else if(can_interact == 2)
      {
        draw_bordered_icon(&hand_lock, mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
      }
      else
      {
        draw_bordered_icon(&hand_up,   mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
      }
    }
    else
    {
      draw_bordered_icon(&hand,        mouse_state.x_pos - 2, mouse_state.y_pos, COLOR_BLACK);
    }
  }
}
