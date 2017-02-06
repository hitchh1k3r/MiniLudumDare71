#include <io.h>
#include "game.h"
#include "graphics.h"
#include "appstate.h"
#include "input.h"
#include "maze.h"

#define CONTENT_MAZE_TXT "W  WWWWWWW\r\nW WEt T  W\r\nW s TSW  W\r\nW WWWW   W\r\nW S S    W\r\nW sWW   tW\r\nWWPWWWWWWW"
#define CONTENT_CONGRATS_TXT "You have managed to escape the room.\r\nThank you for playing.\r\nThe \"VR Maze\" game was inspired by a game I made early in my days of playing around in BASIC.\r\nIt was reimplemented here with slightly better graphics, and in C (plus mouse support and a more sophisticated\r\nrendering system ^_^). You can edit MAZE.TXT and play your own mazes.\r\n\r\nMazes are composed of letters that represent the tiles.\r\nW is a wall,\r\nP is the player start location,\r\nE is an exit tile,\r\ns and t are square and triangle keys,\r\nS and T are square and triangles doors,\r\nand spaces (or any unknown letters) are empty spaces.\r\nNothing but maze data can be in maze.txt (each character will be treated as an empty space, and empty each line as an entire row of spaces).\r\n\r\nHere are the mazes from the game to get you understanding the format:\r\n\r\n~Hello World~\r\nWW\r\nWE\r\nW\r\nWP\r\n\r\n\r\n~Maze~\r\nW WWWW\r\nW WW\r\nW    W\r\nWPWW W\r\nWWE\r\n\r\n\r\n~Endless~\r\nW WE\r\nW W\r\nW W\r\nW W\r\n\r\nW W\r\nW W\r\nWPW\r\n\r\n\r\n~One Door~\r\nWWW\r\nW E\r\nWSs\r\nW\r\nWWP\r\n\r\n\r\n~Two Doors~\r\nWWWWWW\r\nW   TE\r\nW WSss\r\nW W\r\nWtW P\r\n\r\n\r\n~Droste~\r\nWE\r\nWS\r\nWT\r\nW\r\ns\r\nW\r\nWP\r\nWt\r\n\r\n\r\n~Kicking In Doors~\r\nsWEWtWtWsW\r\nSWSWSWSWSW\r\n         t\r\nTWTW WTWTW\r\nsWtW WsWsW\r\ntWsW WsWtW\r\nSWSWSWSWSW\r\n         s\r\nTWTWPWTWTW\r\nsWsWWWtWsW\r\n\r\n\r\n~The Final Door~\r\nWsW\r\nWSW\r\nWPW\r\nW W\r\nW W\r\nWEW"

#define add_scene(I,P,M,a,A,b,B,c,C,d,D,e,E,f,F,g,G) scenes[I].image_file=P;scenes[I].map_file=M;scenes[I].links[0]=A;scenes[I].links[1]=B;scenes[I].links[2]=C;scenes[I].links[3]=D;scenes[I].links[4]=E;scenes[I].links[5]=F;scenes[I].links[6]=G;scenes[I].link_types[0]=a;scenes[I].link_types[1]=b;scenes[I].link_types[2]=c;scenes[I].link_types[3]=d;scenes[I].link_types[4]=e;scenes[I].link_types[5]=f;scenes[I].link_types[6]=g;
#define get_link(X,Y) click_map.data[(Y*click_map.height/DISPLAY_HEIGHT*click_map.width)+(X*click_map.width/DISPLAY_WIDTH)]-1;

bitmap scene;
bitmap click_map;
bitmap hand_down, hand_turn_left, hand_turn_right;
bitmap num_0, num_1, num_2, num_3, num_4, num_5, num_6, num_7, num_8, num_9;

bitmap* nums[10] = {&num_0, &num_1, &num_2, &num_3, &num_4, &num_5, &num_6, &num_7, &num_8, &num_9};

int cur_x, cur_y;
scene_data scenes[19];
int hover_link = -1;
link_type hand_type = NORMAL;

/* Serializable gamestate: */
int  scene_index     = 4;
bool comp_plugged_in = false;
bool pi_revealed     = false;
bool pi_toggle       = false;
bool red_seen        = false;
bool blue_seen       = false;
int  thermostat      = 0;
int  combination     = 764;
bool door_unlock     = false;

void create_victory_files()
{
  FILE* fp;

  if(access(MAZE_FILE, 0) != 0 && (fp = fopen(MAZE_FILE, "wb")) != NULL)
  {
    fputs(CONTENT_MAZE_TXT, fp);
    fclose(fp);
  }

  if(access("CONGRATS.TXT", 0) != 0 && (fp = fopen("CONGRATS.TXT", "wb")) != NULL)
  {
    fputs(CONTENT_CONGRATS_TXT, fp);
    fclose(fp);
  }
}

void reload_variables()
{
  if(comp_plugged_in)
  {
    scenes[5].links[1] =  6;
    scenes[5].links[2] = -1;
    if(pi_toggle)
    {
      if(door_unlock)
      {
        scenes[5].image_file = "COMP/COMTV";
        scenes[6].image_file = "COMP/COMTVZ";
      }
      else
      {
        scenes[5].image_file = "COMP/COMT";
        scenes[6].image_file = "COMP/COMTZ";
      }
    }
    else
    {
      if(door_unlock)
      {
        scenes[5].image_file = "COMP/COMPV";
        scenes[6].image_file = "COMP/COMPVZ";
      }
      else
      {
        scenes[5].image_file = "COMP/COMP";
        scenes[6].image_file = "COMP/COMPZ";
      }
    }
  }
  else
  {
    scenes[5].image_file = "COMP/COM_U";
    scenes[5].links[1] = -1;
    scenes[5].links[2] =  5;
  }
  if(combination == 372 && ((red_seen && blue_seen) || user_maze))
  {
    scenes[8].links[1] = 12;
    thermostat = 0;
    scenes[17].image_file = "THERMO/TMPMID";
    scenes[17].links[1] = -1;
    scenes[17].link_types[1] = NORMAL;
    scenes[17].links[2] = -1;
    scenes[17].link_types[2] = NORMAL;
  }
  else
  {
    scenes[8].links[1] = 9;
    if(thermostat == 0)
    {
      scenes[17].image_file = "THERMO/TMPMID";
      scenes[17].links[1] = 2;
      scenes[17].link_types[1] = TURN_LEFT;
      scenes[17].links[2] = 3;
      scenes[17].link_types[2] = TURN_RIGHT;
    }
    else if(thermostat == 1)
    {
      scenes[17].image_file = "THERMO/TMPUP";
      scenes[17].links[1] = 2;
      scenes[17].link_types[1] = TURN_LEFT;
      scenes[17].links[2] = -1;
      scenes[17].link_types[2] = NORMAL;
    }
    else
    {
      scenes[17].image_file = "THERMO/TMPLOW";
      scenes[17].links[1] = -1;
      scenes[17].link_types[1] = NORMAL;
      scenes[17].links[2] = 3;
      scenes[17].link_types[2] = TURN_RIGHT;
    }
  }
  if(pi_toggle)
  {
    scenes[16].image_file = "PI/T";
  }
  else
  {
    scenes[16].image_file = "PI/P";
  }
  if(pi_revealed)
  {
    scenes[1].links[6] = 16;
  }
  else
  {
    scenes[1].links[6] = -1;
  }
  if(door_unlock)
  {
    scenes[13].links[1] = 18;
    scenes[13].link_types[1] = NORMAL;
  }
  else
  {
    scenes[13].links[1] = -1;
    scenes[13].link_types[1] = LOCKED;
  }
}

void click_link()
{
  int m, r;

  if(scene_index == 5 && hover_link == 5)
  {
    comp_plugged_in = true;
    reload_variables();
  }
  else if(scene_index == 11 && hover_link >= 1 && hover_link <= 3)
  {
    if(hover_link == 1)
    {
      m = (combination / 100) % 10;
      r = (combination - (100 * m)) % 1000;
      if(cur_y < 100)
      {
        m += 1;
      }
      else
      {
        m += 9;
      }
      m %= 10;
      combination = (m * 100) + r;
    }
    else if(hover_link == 2)
    {
      m = (combination / 10) % 10;
      r = (combination - (10 * m)) % 1000;
      if(cur_y < 100)
      {
        m += 1;
      }
      else
      {
        m += 9;
      }
      m %= 10;
      combination = (m * 10) + r;
    }
    else if(hover_link == 3)
    {
      m = combination % 10;
      r = (combination - m) % 1000;
      if(cur_y < 100)
      {
        m += 1;
      }
      else
      {
        m += 9;
      }
      m %= 10;
      combination = m + r;
    }

    hover_link = 11;
    if(combination == 372 && ((red_seen && blue_seen) || user_maze))
    {
      hover_link = 12;
      reload_variables();
    }
  }
  else if(scene_index == 8 && hover_link == 10)
  {
    if(pi_toggle && thermostat == 1)
    {
      red_seen = true;
      if(blue_seen && combination == 372)
      {
        combination = 72;
      }
      reload_variables();
    }
    else if(pi_toggle && thermostat == 2)
    {
      blue_seen = true;
      if(blue_seen && combination == 372)
      {
        combination = 370;
      }
      reload_variables();
    }
  }
  else if(scene_index == 12 && hover_link == 12)
  {
    new_state = MAZE;
  }
  else if(scene_index == 16 && hover_link == 16)
  {
    pi_toggle = !pi_toggle;
    reload_variables();
  }
  else if(scene_index == 17 && hover_link >= 2 && hover_link <= 3)
  {
    if(hover_link == 2)
    {
      if(thermostat == 0)
      {
        thermostat = 2;
      }
      else
      {
        thermostat = 0;
      }
    }
    else
    {
      if(thermostat == 0)
      {
        thermostat = 1;
      }
      else
      {
        thermostat = 0;
      }
    }
    hover_link = 17;
    reload_variables();
  }
  else if(scene_index == 14 && hover_link == 15)
  {
    pi_revealed = true;
    reload_variables();
  }
  else if(scene_index == 13 && hover_link == 18)
  {
    create_victory_files();
  }
}

void game_reset()
{
  scene_index     = 4;
  comp_plugged_in = false;
  pi_revealed     = false;
  pi_toggle       = false;
  red_seen        = false;
  blue_seen       = false;
  thermostat      = 0;
  combination     = 764;
  door_unlock     = false;

  reload_variables();
}

void game_serialize(byte_buffer* buf)
{
  byte_buffer_add(buf, &scene_index,     sizeof(int));
  byte_buffer_add(buf, &comp_plugged_in, sizeof(bool));
  byte_buffer_add(buf, &pi_revealed,     sizeof(bool));
  byte_buffer_add(buf, &pi_toggle,       sizeof(bool));
  byte_buffer_add(buf, &red_seen,        sizeof(bool));
  byte_buffer_add(buf, &blue_seen,       sizeof(bool));
  byte_buffer_add(buf, &thermostat,      sizeof(int));
  byte_buffer_add(buf, &combination,     sizeof(int));
  byte_buffer_add(buf, &door_unlock,     sizeof(bool));
}

void game_deserialize(byte_buffer* buf)
{
  byte_buffer_get(buf, &scene_index,     sizeof(int));
  byte_buffer_get(buf, &comp_plugged_in, sizeof(bool));
  byte_buffer_get(buf, &pi_revealed,     sizeof(bool));
  byte_buffer_get(buf, &pi_toggle,       sizeof(bool));
  byte_buffer_get(buf, &red_seen,        sizeof(bool));
  byte_buffer_get(buf, &blue_seen,       sizeof(bool));
  byte_buffer_get(buf, &thermostat,      sizeof(int));
  byte_buffer_get(buf, &combination,     sizeof(int));
  byte_buffer_get(buf, &door_unlock,     sizeof(bool));

  reload_variables();
}

void game_init()
{
  int u, v;
  cur_x = mouse_state.x_pos;
  cur_y = mouse_state.y_pos;

  load_bmp("UI/HAND_D.BMP", &hand_down,       ICON);
  load_bmp("UI/TURN_L.BMP", &hand_turn_left,  ICON);
  load_bmp("UI/TURN_R.BMP", &hand_turn_right, ICON);

  load_bmp("SCENES/IMG/ARMOIRE/LOCK/0.BMP", &num_0, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/1.BMP", &num_1, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/2.BMP", &num_2, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/3.BMP", &num_3, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/4.BMP", &num_4, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/5.BMP", &num_5, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/6.BMP", &num_6, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/7.BMP", &num_7, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/8.BMP", &num_8, MAP);
  load_bmp("SCENES/IMG/ARMOIRE/LOCK/9.BMP", &num_9, MAP);

  add_scene( 0, "DIR/NORTH",            "NORTH",   UP,      4, LEFT,      3, RIGHT,      1, NORMAL,  5, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene( 1, "DIR/EAST",             "EAST",    UP,      4, LEFT,      0, RIGHT,      2, NORMAL,  8, NORMAL, 17, NORMAL, 14, NORMAL, -2);
  add_scene( 2, "DIR/SOUTH",            "SOUTH",   UP,      4, LEFT,      1, RIGHT,      3, NORMAL, 13, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene( 3, "DIR/WEST",             "WEST",    UP,      4, LEFT,      2, RIGHT,      0, NORMAL,  7, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene( 4, "DIR/UP",               "ALL",     DOWN,    2, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene( 5, "use computer",         "COMP",    DOWN,    0, NORMAL,   -2, NORMAL,    -2, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene( 6, "zoom computer",        "ALL",     DOWN,    5, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene( 7, "COMP/CHART",           "ALL",     DOWN,    3, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene( 8, "ARMOIRE/ARM",          "ARMOIRE", DOWN,    1, NORMAL,   -2, NORMAL,    10, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene( 9, "ARMOIRE/LOCK/LOCKBOX", "LOCKBOX", DOWN,    8, NORMAL,   11, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene(10, "ARMOIRE/CARD",         "ALL",     DOWN,    8, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene(11, "ARMOIRE/LOCK/ZOOM",    "ZOOM",    DOWN,    9, NORMAL,    1, NORMAL,     2, NORMAL,  3, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene(12, "ARMOIRE/LOCK/VR",      "VR",      DOWN,    8, NORMAL,   12, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene(13, "DOOR/DOOR",            "DOOR",    DOWN,    2, LOCKED,   -2, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene(14, "TENT/PLANTS",          "PLANTS",  DOWN,    1, NORMAL,   15, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);
  add_scene(15, "TENT/NOTE",            "ALL",     DOWN,   14, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene(16, "raspberry pi",         "PI",      DOWN,    1, NORMAL,   16, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene(17, "thermostat",           "THERMO",  DOWN,    1, TURN_LEFT, 2, TURN_RIGHT, 3, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  add_scene(18, "DOOR/OUTSIDE",         "ALL",     NORMAL, -1, NORMAL,   -1, NORMAL,    -1, NORMAL, -1, NORMAL, -1, NORMAL, -1, NORMAL, -1);

  game_reset();
}

void load_scene()
{
  bool use_color_img;
  char buf[64];

  use_color_img = false;
  if(thermostat > 0 && pi_toggle)
  {
    sprintf(buf, "SCENES/IMG/%s_%c.BMP", scenes[scene_index].image_file, (thermostat == 1 ? 'R' : 'B'));
    if(access(buf, 0) == 0)
    {
      use_color_img = true;
    }
  }

  if(!use_color_img)
  {
    sprintf(buf, "SCENES/IMG/%s.BMP", scenes[scene_index].image_file);
  }

  load_bmp(buf, &scene, PHOTO);

  if(scene_index == 11)
  {
    stamp_bitmap(&scene, nums[(combination / 100) % 10], 109, 60);
    stamp_bitmap(&scene, nums[(combination / 10) % 10], 149, 60);
    stamp_bitmap(&scene, nums[combination % 10], 190, 60);
  }

  if(!use_color_img && thermostat > 0 && pi_toggle && scene_index != 7 && scene_index != 14 && scene_index != 15 && scene_index != 18)
  {
    if(thermostat == 1)
    {
      tint_pallet(5, 3, 1, 4, 1, 4);
    }
    else
    {
      tint_pallet(1, 4, 1, 4, 5, 3);
    }
  }

  if(scene_index == 18)
  {
    draw_string(&scene, "Congratulations", (DISPLAY_WIDTH - get_string_width("Congratulations")) / 2 + 1, 5 + 1, COLOR_BLACK);
    draw_string(&scene, "Congratulations", (DISPLAY_WIDTH - get_string_width("Congratulations")) / 2,     5,     COLOR_WHITE);
    draw_string(&scene, "You Have Escaped The Room", (DISPLAY_WIDTH - get_string_width("You Have Escaped The Room")) / 2 + 1, 19 + 1, COLOR_BLACK);
    draw_string(&scene, "You Have Escaped The Room", (DISPLAY_WIDTH - get_string_width("You Have Escaped The Room")) / 2,     19,     COLOR_WHITE);

    draw_string(&scene, "Thank You For Playing", (DISPLAY_WIDTH - get_string_width("Thank You For Playing")) / 2 + 1, 132, COLOR_BLACK);
    draw_string(&scene, "Thank You For Playing", (DISPLAY_WIDTH - get_string_width("Thank You For Playing")) / 2 - 1, 132, COLOR_BLACK);
    draw_string(&scene, "Thank You For Playing", (DISPLAY_WIDTH - get_string_width("Thank You For Playing")) / 2, 132 + 1, COLOR_BLACK);
    draw_string(&scene, "Thank You For Playing", (DISPLAY_WIDTH - get_string_width("Thank You For Playing")) / 2, 132 - 1, COLOR_BLACK);
    draw_string(&scene, "Thank You For Playing", (DISPLAY_WIDTH - get_string_width("Thank You For Playing")) / 2,     132,     COLOR_WHITE);

    draw_string(&scene, "A Game For MiniLD Seventy One", (DISPLAY_WIDTH - get_string_width("A Game For MiniLD Seventy One")) / 2 + 1, 155 + 1, COLOR_BLACK);
    draw_string(&scene, "A Game For MiniLD Seventy One", (DISPLAY_WIDTH - get_string_width("A Game For MiniLD Seventy One")) / 2,     155,     COLOR_WHITE);
    draw_string(&scene, "The Retro Challenge", (DISPLAY_WIDTH - get_string_width("The Retro Challenge")) / 2 + 1, 169, COLOR_CHROMA_2);
    draw_string(&scene, "The Retro Challenge", (DISPLAY_WIDTH - get_string_width("The Retro Challenge")) / 2 - 1, 169, COLOR_CHROMA_2);
    draw_string(&scene, "The Retro Challenge", (DISPLAY_WIDTH - get_string_width("The Retro Challenge")) / 2, 169 + 1, COLOR_CHROMA_2);
    draw_string(&scene, "The Retro Challenge", (DISPLAY_WIDTH - get_string_width("The Retro Challenge")) / 2, 169 - 1, COLOR_CHROMA_2);
    draw_string(&scene, "The Retro Challenge", (DISPLAY_WIDTH - get_string_width("The Retro Challenge")) / 2,     169,     COLOR_CHROMA_1);

    draw_string(&scene, "By HitchHiker and Naali", (DISPLAY_WIDTH - get_string_width("By HitchHiker and Naali")) / 2 + 1, 183 + 1, COLOR_BLACK);
    draw_string(&scene, "By HitchHiker and Naali", (DISPLAY_WIDTH - get_string_width("By HitchHiker and Naali")) / 2,     183,     COLOR_WHITE);
  }

  sprintf(buf, "SCENES/DATA/%s.BMP", scenes[scene_index].map_file);
  load_bmp(buf, &click_map, MAP);
}

void game_activate()
{
  reload_variables();
  load_scene();
}

void game_update()
{
  int a, b, c;
  char buffer[255];

  if(keyboard_state.key_code != 0)
  {
    if(keyboard_state.key_code == 27) /* ESC */
    {
      if(scene_index == 18)
      {
        clear_buffer(&scene, COLOR_BLACK);
      }
      new_state = MENU;
    }
    else if(keyboard_state.key_code == 119 || /* w */
            keyboard_state.key_code ==  87 || /* W */
            keyboard_state.key_code ==  72)   /* UP */
    {
      cur_y -= 4;
      mouse_active = false;
    }
    else if(keyboard_state.key_code == 97 || /* a */
            keyboard_state.key_code == 65 || /* A */
            keyboard_state.key_code == 75)   /* LEFT */
    {
      cur_x -= 4;
      mouse_active = false;
    }
    else if(keyboard_state.key_code == 115 || /* s */
            keyboard_state.key_code ==  83 || /* S */
            keyboard_state.key_code ==  80)   /* DOWN */
    {
      cur_y += 4;
      mouse_active = false;
    }
    else if(keyboard_state.key_code == 100 || /* d */
            keyboard_state.key_code ==  68 || /* D */
            keyboard_state.key_code ==  77)   /* RIGHT */
    {
      cur_x += 4;
      mouse_active = false;
    }
  }
  if(mouse_active)
  {
    cur_x = mouse_state.x_pos;
    cur_y = mouse_state.y_pos;
  }

  hand_type = NORMAL;
  hover_link = get_link(cur_x, cur_y);
  if(hover_link >= 0)
  {
    hand_type = scenes[scene_index].link_types[hover_link];
    hover_link = scenes[scene_index].links[hover_link];
  }

  if(hover_link >= 0 && (mouse_state.clicked ||
                         keyboard_state.key_code == 32 || /* SPACE */
                         keyboard_state.key_code == 13))  /* ENTER */
  {
    click_link();
    scene_index = hover_link;
    load_scene();
  }
}

void game_draw()
{
  bitmap_blit(&scene, 0, 0);

  if(scene_index != 18)
  {
    draw_bordered_icon((hand_type == NORMAL ? &hand :      (hand_type == LEFT ? &hand_left :
                       (hand_type == RIGHT ? &hand_right : (hand_type == DOWN ? &hand_down :
                       (hand_type == UP ? &hand_up :       (hand_type == LOCKED ? &hand_lock :
                       (hand_type == TURN_LEFT ? &hand_turn_left : &hand_turn_right))))))),
          cur_x - 2, cur_y, (hover_link >= 0 && hand_type == NORMAL) ? COLOR_SELECT : COLOR_BLACK);
  }
}
