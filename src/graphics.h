#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include "types.h"

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 200

#define TEXT_MODE      0x03
#define VGA_MODE       0x13

#define PALLET_READ    0x03C7
#define PALLET_WRITE   0x03C8
#define PALLET_DATA    0x03C9
#define INPUT_STATUS   0x03DA
#define VRETRACE       0x08

#define NUM_LOAD_IMG   80

#define COLOR_CYAN     0xF8
#define COLOR_WHITE    0xF9
#define COLOR_GRAY     0xFA
#define COLOR_DISABLED 0xFB
#define COLOR_BLACK    0xFC
#define COLOR_SELECT   0xFD
#define COLOR_TRANSP   0xFD
#define COLOR_CHROMA_1 0xFE
#define COLOR_CHROMA_2 0xFF

typedef struct
{
  word  width;
  word  height;
  byte* data;
} bitmap;

typedef enum { ICON, PHOTO, MAP, OVERLAY } image_type;

extern bitmap composer;
extern byte* double_buffer;
extern bitmap hand, hand_left, hand_right, hand_up, hand_lock;

/*

  THIS IS A SUPER MESSY GRAPHIC'S LIBRARY... I ADDED NEW METHODS AS NEEDED, AND ONLY OCCASIONALLY
  REMOVED ONES.... IDEALLY WE WOULD ONLY HAVE ONE DRAW METHOD THAT TAKES A DRAW TYPE, AND THE SCREEN
  BUFFER WOULD BE A BITMAP THAT COULD BE COPPIED TO (SO ALL GRAPHICS OPERATIONS, EXCEPT SWAP BUFFERS)
  WOULD COPY FROM ONE BITMAP TO ANOTHER.

  BELIEVE IT OR NOT THIS GOT SOME CLEANUP... IF I HAD MORE TIME TO FINISH THIS GAME I'D CLEAN IT UP
  A LOT MORE.... HOW EMBARISING ^_^

*/

void screen(byte mode);
#define bit_blit(X, Y, C) double_buffer[X + (Y << 8) + (Y << 6)] = C;
void load_bmp(char* file, bitmap* b, image_type type);
void load_default_font(char* file);
void make_buffer(bitmap* bmp, int width, int height, byte color);
void clear_buffer(bitmap* bmp, byte color);
void free_buffer(bitmap* bmp);
void bitmap_blit(bitmap* bmp, int x, int y);
void stamp_bitmap(bitmap* dst, bitmap* src, int x, int y);
void draw_icon(bitmap* bmp, int x, int y);
void stamp_icon(bitmap* dst, bitmap* src, int x, int y);
void stamp_scale_icon(bitmap* dst, bitmap* src, int x, int y, int scale);
void draw_icon_transp(bitmap* bmp, int x, int y, byte transp_color);
void stamp_icon_transp(bitmap* dst, bitmap* src, int x, int y, byte transp_color);
void draw_bordered_icon(bitmap* bmp, int x, int y, byte color);
void draw_string(bitmap* bmp, char* str, int x, int y, byte color);
void draw_scaled_string(bitmap* bmp, char* str, int x, int y, int scale, byte color);
int  get_string_width(char* str);
void swap_buffers();
void init_graphics();
void cleanup_graphics();
void pallet_set(int index, byte r, byte g, byte b);
void attenuate_pallet(int numer, int denom);
void tint_pallet(int r_num, int r_den, int g_num, int g_den, int b_num, int b_den);
void color_update();
void stamp_overlay(bitmap* dst, bitmap* src, int x, int y, bool flip);
void bitmap_underlay(bitmap* dst, bitmap* src, int x, int y);

#endif
