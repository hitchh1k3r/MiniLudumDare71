#include <alloc.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "errors.h"
#include "fileio.h"
#include "graphics.h"
#include "types.h"

#define C_F8 0,52,58
#define C_F9 63,63,63
#define C_FA 40,40,40
#define C_FB 18,18,18
#define C_FC 0,0,0

#define C_FD_1  43,43,43
#define C_FD_2  47,47,47
#define C_FD_3  53,53,53
#define C_FD_4  58,58,58
#define C_FD_5  63,63,63
#define C_FD_6  63,63,63
#define C_FD_7  63,63,63
#define C_FD_8  58,58,58
#define C_FD_9  53,53,53
#define C_FD_10 47,47,47
#define C_FD_11 43,43,43
#define C_FD_12 43,43,43

bitmap* loaded_bitmaps[NUM_LOAD_IMG];
byte* VGA = (byte*)    0xA0000000L;
byte* double_buffer;
byte pallet[3 * 248];
bool pallet_dirty = false;

int color_cycle;
bitmap hand, hand_left, hand_right, hand_up, hand_lock;
bitmap composer;

struct font
{
  word height;
  word offset[27];
  byte bytes_per_row;
  byte* data;
} font;

void init_graphics()
{
  int i;

  screen(VGA_MODE);

  make_buffer(&composer, DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_BLACK);

  double_buffer = (byte*) malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT);
  if(double_buffer == NULL)
  {
    crash("Cannot allocate double buffer.");
  }

  for(i = 0; i < NUM_LOAD_IMG; ++i)
  {
    loaded_bitmaps[i] = NULL;
  }

  load_default_font("UI/FONT.FNT");
  load_bmp("UI/HAND.BMP",   &hand,            ICON);
  load_bmp("UI/HAND_L.BMP", &hand_left,       ICON);
  load_bmp("UI/HAND_R.BMP", &hand_right,      ICON);
  load_bmp("UI/HAND_U.BMP", &hand_up,         ICON);
  load_bmp("UI/LOCKED.BMP", &hand_lock,       ICON);

  /* Fixed Color Pallet Indexes: */
  pallet_set(0xF8, C_F8);
  pallet_set(0xF9, C_F9);
  pallet_set(0xFA, C_FA);
  pallet_set(0xFB, C_FB);
  pallet_set(0xFC, C_FC);
}

void cleanup_graphics()
{
  int i;

  screen(TEXT_MODE);

  free(&double_buffer);
  free(&font.data);

  for(i = 0; i < NUM_LOAD_IMG; ++i)
  {
    if(loaded_bitmaps[i] == NULL)
    {
      free(&loaded_bitmaps[i]->data);
    }
  }
}

void swap_buffers()
{
  /* Wait for the current graphics frame to end: */
  while((inp(INPUT_STATUS) & VRETRACE));
  while(!(inp(INPUT_STATUS) & VRETRACE));

  /* Move the offscreen buffer onto the screen: */
  memcpy(VGA, double_buffer, DISPLAY_WIDTH * DISPLAY_HEIGHT);
}

void screen(byte mode)
{
  union REGS regs;

  regs.h.ah = 0x00;
  regs.h.al = mode;
  int86(0x10, &regs, &regs);
}

void bitmap_blit(bitmap* bmp, int x, int y)
{
  int i;

  word screen_offset = (y<<8) + (y<<6) + x;
  word bitmap_offset = 0;

  for(i=0; i < bmp->height; ++i)
  {
    memcpy(&double_buffer[screen_offset], &bmp->data[bitmap_offset], bmp->width);

    bitmap_offset += bmp->width;
    screen_offset += DISPLAY_WIDTH;
  }
}

void stamp_bitmap(bitmap* dst, bitmap* src, int x, int y)
{
  int i;

  word dst_offset = (y * dst->width) + x;
  word src_offset = 0;

  for(i=0; i < src->height; ++i)
  {
    memcpy(&dst->data[dst_offset], &src->data[src_offset], src->width);

    src_offset += src->width;
    dst_offset += dst->width;
  }
}

void draw_icon_transp(bitmap* bmp, int x, int y, byte transp_color)
{
  int i, j, aW = 0, bW = 0, w, h;
  word screen_offset;
  word bitmap_offset = 0;
  byte data;
  w = bmp->width;
  h = bmp->height;
  if(x < 0)
  {
    w += x;
    aW = -x;
    x = 0;
  }
  if(y < 0)
  {
    h += y;
    bitmap_offset = -y * bmp->width;
    y = 0;
  }
  if(x + w > DISPLAY_WIDTH)
  {
    w = DISPLAY_WIDTH - x;
    bW = bmp->width - w;
  }
  if(y + h > DISPLAY_HEIGHT)
  {
    h = DISPLAY_HEIGHT - y;
  }

  screen_offset = (y<<8)+(y<<6);
  for(j = 0; j < h; j++)
  {
    bitmap_offset += aW;
    for(i = 0; i < w; i++, bitmap_offset++)
    {
      data = bmp->data[bitmap_offset];
      if(data != transp_color)
      {
        double_buffer[screen_offset+x+i] = data;
      }
    }
    bitmap_offset += bW;
    screen_offset += DISPLAY_WIDTH;
  }
}

void draw_icon(bitmap* bmp, int x, int y)
{
  draw_icon_transp(bmp, x, y, COLOR_TRANSP);
}

void stamp_icon_transp(bitmap* dst, bitmap* src, int x, int y, byte transp_color)
{
  int i, j, aW = 0, bW = 0, w, h;
  word dst_offset;
  word bitmap_offset = 0;
  byte data;
  w = src->width;
  h = src->height;
  if(x < 0)
  {
    w += x;
    aW = -x;
    x = 0;
  }
  if(y < 0)
  {
    h += y;
    bitmap_offset = -y * src->width;
    y = 0;
  }
  if(x + w > dst->width)
  {
    w = dst->width - x;
    bW = src->width - w;
  }
  if(y + h > dst->height)
  {
    h = dst->height - y;
  }

  dst_offset = y * dst->width;
  for(j = 0; j < h; j++)
  {
    bitmap_offset += aW;
    for(i = 0; i < w; i++, bitmap_offset++)
    {
      data = src->data[bitmap_offset];
      if(data != transp_color)
      {
        dst->data[dst_offset+x+i] = data;
      }
    }
    bitmap_offset += bW;
    dst_offset += dst->width;
  }
}

void stamp_scale_icon(bitmap* dst, bitmap* src, int x, int y, int scale)
{
  int i, j, aW = 0, bW = 0, w, h, sU, sV;
  word dst_offset;
  word bitmap_offset = 0;
  byte data;
  w = src->width;
  h = src->height;
  if(x < 0)
  {
    w += x;
    aW = -x;
    x = 0;
  }
  if(y < 0)
  {
    h += y;
    bitmap_offset = -y * src->width;
    y = 0;
  }
  if(x + w > dst->width)
  {
    w = dst->width - x;
    bW = src->width - w;
  }
  if(y + h > dst->height)
  {
    h = dst->height - y;
  }

  dst_offset = y * dst->width;
  for(j = 0; j < h; j++)
  {
    bitmap_offset += aW;
    for(i = 0; i < w; i++, bitmap_offset++)
    {
      data = src->data[bitmap_offset];
      if(data != COLOR_TRANSP)
      {
        for(sU = 0; sU < scale; sU++)
        {
          for(sV = 0; sV < scale; sV++)
          {
            dst->data[dst_offset+x+(i*scale)+sU + sV*dst->width] = data;
          }
        }
      }
    }
    bitmap_offset += bW;
    dst_offset += dst->width * scale;
  }
}

void stamp_icon(bitmap* dst, bitmap* src, int x, int y)
{
  stamp_icon_transp(dst, src, x, y, COLOR_TRANSP);
}

void draw_bordered_icon(bitmap* bmp, int x, int y, byte color)
{
  int u, v, aW = 0, aH = 0, bW = 0, w, h;
  word screen_offset;
  word bitmap_offset = 0;
  byte data;
  w = bmp->width;
  h = bmp->height;
  if(x < 0)
  {
    w += x;
    aW = -x;
    x = 0;
  }
  if(y < 0)
  {
    h += y;
    aH = -y;
    y = 0;
  }
  if(x + w > DISPLAY_WIDTH)
  {
    w = DISPLAY_WIDTH - x;
    bW = bmp->width - w;
  }
  if(y + h > DISPLAY_HEIGHT)
  {
    h = DISPLAY_HEIGHT - y;
  }

  bitmap_offset = aH * bmp->width;
  screen_offset = (y<<8)+(y<<6);
  screen_offset -= DISPLAY_WIDTH;
  bitmap_offset -= bmp->width - 1;
  for(v = -1; v <= h; v++)
  {
    bitmap_offset += aW - 2;
    for(u = -1; u <= w; u++)
    {
      if(u >= 0 && v >= 0 && u < w && v < h)
      {
        data = bmp->data[bitmap_offset];
      }
      else
      {
        data = COLOR_TRANSP;
      }
      if(data != COLOR_TRANSP)
      {
        if(screen_offset >= 0)
        {
          double_buffer[screen_offset+x+u] = data;
        }
      }
      else
      {
        if(((v >= 0) && (v < h) && (u + aW) > 0               && (bmp->data[bitmap_offset - 1] != COLOR_TRANSP)) ||
           ((v >= 0) && (v < h) && (u + aW + 1) < bmp->width  && (bmp->data[bitmap_offset + 1] != COLOR_TRANSP)) ||
           ((u >= 0) && (u < w) && (v + aH) > 0               && (bmp->data[bitmap_offset - bmp->width] != COLOR_TRANSP)) ||
           ((u >= 0) && (u < w) && (v + aH + 1) < bmp->height && (bmp->data[bitmap_offset + bmp->width] != COLOR_TRANSP)))
        {
          if((x + u) >= 0 && (x + u) < DISPLAY_WIDTH && (y + v) >= 0 && (y + v) < DISPLAY_HEIGHT)
          {
            double_buffer[screen_offset+x+u] = color;
          }
        }
      }
      ++bitmap_offset;
    }
    bitmap_offset += bW;
    screen_offset += DISPLAY_WIDTH;
  }
}

void load_bmp(char* file, bitmap* b, image_type type)
{
  FILE* fp;
  word num_colors, rW, width, height;
  byte rc, gc, bc;
  long index;
  int i, x, q;
  char error_buffer[255];
  bool loadedPointer;

  /* Store a pointer for cleanup: */
  loadedPointer = false;
  for(i = 0; i < NUM_LOAD_IMG; ++i)
  {
    if(loaded_bitmaps[i] == b)
    {
      loadedPointer = true;
      i = NUM_LOAD_IMG;
    }
  }
  if(!loadedPointer)
  {
    for(i = 0; i < NUM_LOAD_IMG; ++i)
    {
      if(loaded_bitmaps[i] == NULL)
      {
        loaded_bitmaps[i] = b;
        loadedPointer = true;
        i = NUM_LOAD_IMG;
      }
    }
  }
  if(!loadedPointer)
  {
    sprintf(error_buffer, "Too many images are loaded, cannot open %s", file);
    crash(error_buffer);
  }

  /* Open the file: */
  if((fp = fopen(file, "rb")) == NULL)
  {
    sprintf(error_buffer, "Cannot open file %s", file);
    crash(error_buffer);
  }

  /* Read header data: */
  if(fgetc(fp) != 'B' || fgetc(fp) != 'M')
  {
    fclose(fp);
    sprintf(error_buffer, "%s is not a bitmap file", file);
    crash(error_buffer);
  }

  fskip(fp, 16);
  fread(&width,      sizeof(word), 1, fp);
  fskip(fp, 2);
  fread(&height,     sizeof(word), 1, fp);
  fskip(fp, 22);
  fread(&num_colors, sizeof(word), 1, fp);
  fskip(fp, 6);

  if(num_colors == 0)
  {
    num_colors = 256;
  }

  /* Allocate image data memory block: */
  if(b->data != NULL)
  {
    if(b->width != width && b->height != height)
    {
      free(b->data);
    }
  }

  b->width = width;
  b->height = height;

  if(b->data == NULL)
  {
    if((b->data = (byte*) malloc((word)(b->width*b->height) / (type == OVERLAY ? 4 : 1))) == NULL)
    {
      fclose(fp);
      sprintf(error_buffer, "Not enough memory to open %s", file);
      crash(error_buffer);
    }
  }

  /* Set the VGA pallet to this images pallet (if it is a PHOTO): */
  if(type == PHOTO && num_colors >= 248)
  {
    pallet_dirty = true;
    for(x = 0; x < 3 * 248; x += 3)
    {
      pallet[x + 2] = fgetc(fp) >> 2;
      pallet[x + 1] = fgetc(fp) >> 2;
      pallet[x] = fgetc(fp) >> 2;
      fgetc(fp);
    }
    x /= 3;
    for(; x < num_colors; ++x)
    {
      /* Discard colors past 236 index! */
      fgetc(fp);
      fgetc(fp);
      fgetc(fp);
      fgetc(fp);
    }
  }
  else
  {
    fskip(fp, num_colors * 4);
  }

  /* Read image data: */
  rW = (4 - (b->width % 4)) % 4;

  if(type == OVERLAY)
  {
    /*
      Overlays only have 4 colors... and there are a lot... so we will compress the memory a bit.
      (this costs CPU... but prevents out of memory errors... probably worth it ^_^)
      Also because I'm running out of time... I got a bit sloppy and don't flip the images Y here
      that means the original images have to be flipped vertically... ^_^
    */
    q = 0;
    i = 0;
    for(index = 0; index < b->height; ++index)
    {
      for(x = 0; x < b->width; ++x)
      {
        if(q == 0)
        {
          b->data[i] = 0;
        }
        b->data[i] += (byte)fgetc(fp) << (2*q);
        ++q;
        if(q >= 4)
        {
          q = 0;
          ++i;
        }
      }
      for(x = 0; x < rW; x++)
      {
        fgetc(fp);
      }
    }
  }
  else
  {
    for(index = (b->height - 1) * b->width; index >= 0; index -= b->width)
    {
      for(x = 0; x < b->width; x++)
      {
        b->data[(word)index+x] = (byte)fgetc(fp) + ((type == ICON) ? COLOR_CYAN : 0);
      }
      for(x = 0; x < rW; x++)
      {
        fgetc(fp);
      }
    }
  }

  fclose(fp);
}

void load_default_font(char* file)
{
  FILE* fp;
  int i;
  word width;
  char error_buffer[255];

  /* Open the file: */
  if((fp = fopen(file, "rb")) == NULL)
  {
    sprintf(error_buffer, "Cannot open file %s", file);
    crash(error_buffer);
  }

  /* Read header data: */
  fread(&width,         sizeof(word), 1, fp);
  fread(&font.height,        sizeof(word), 1, fp);
  fread(&font.bytes_per_row, sizeof(word), 1, fp);

  /* Allocate image data memory blocks: */
  if((font.data = (byte*) malloc((word)(width * font.bytes_per_row))) == NULL)
  {
    fclose(fp);
    sprintf(error_buffer, "Not enough memory to open %s", file);
    crash(error_buffer);
  }

  /* Read character offset data: */
  fread(font.offset, sizeof(word), 26, fp);
  font.offset[26] = width;

  /* Read image data: */
  for(i = 0; i < (width * font.bytes_per_row); ++i)
  {
    font.data[i] = (byte)fgetc(fp);
  }

  fclose(fp);
}

void draw_string(bitmap* bmp, char* str, int x, int y, byte color)
{
  int u, bU, v, bV, c, offset, width;
  byte blob;

  for(; *str != '\0'; str++)
  {
    if(*str == ' ')
    {
      x += 5;
    }
    else
    {
      c = *str - 'a';
      if(c < 0 || c >= 26)
      {
        c = *str - 'A';
      }
      if(c >= 0 && c < 26)
      {
        offset = font.offset[c];
        width = font.offset[c + 1] - offset;
        for(u = 0; u < width; u++)
        {
          bU = 0;
          for(v = 0; v < font.height; v += 8)
          {
            blob = font.data[((offset + u) * font.bytes_per_row) + bU];
            for(bV = 0; bV < 8 && (v + bV) < font.height; bV++)
            {
              if(blob & (1 << bV))
              {
                bmp->data[x + ((y + v + bV) * bmp->width)] = color;
              }
            }
            ++bU;
          }
          ++x;
        }
        x += 2;
      }
    }
  }
}

void draw_scaled_string(bitmap* bmp, char* str, int x, int y, int scale, byte color)
{
  int u, bU, v, bV, c, offset, width, sU, sV;
  byte blob;

  for(; *str != '\0'; str++)
  {
    if(*str == ' ')
    {
      x += 5;
    }
    else
    {
      c = *str - 'a';
      if(c < 0 || c >= 26)
      {
        c = *str - 'A';
      }
      if(c >= 0 && c < 26)
      {
        offset = font.offset[c];
        width = font.offset[c + 1] - offset;
        for(u = 0; u < width; u++)
        {
          bU = 0;
          for(v = 0; v < font.height; v += 8)
          {
            blob = font.data[((offset + u) * font.bytes_per_row) + bU];
            for(bV = 0; bV < 8 && (v + bV) < font.height; bV++)
            {
              if(blob & (1 << bV))
              {
                for(sU = 0; sU < scale; sU++)
                {
                  for(sV = 0; sV < scale; sV++)
                  {
                    bmp->data[x + sU + ((y + ((v + bV) * scale) + sV) * bmp->width)] = color;
                  }
                }
              }
            }
            ++bU;
          }
          x += scale;
        }
        x += scale + scale;
      }
    }
  }
}

void make_buffer(bitmap* bmp, int width, int height, byte color)
{
  int i;
  bool loadedPointer;

  /* Store a pointer for cleanup: */
  loadedPointer = false;
  for(i = 0; i < NUM_LOAD_IMG; ++i)
  {
    if(loaded_bitmaps[i] == bmp)
    {
      loadedPointer = true;
      i = NUM_LOAD_IMG;
    }
  }
  if(!loadedPointer)
  {
    for(i = 0; i < NUM_LOAD_IMG; ++i)
    {
      if(loaded_bitmaps[i] == NULL)
      {
        loaded_bitmaps[i] = bmp;
        loadedPointer = true;
        i = NUM_LOAD_IMG;
      }
    }
  }
  if(!loadedPointer)
  {
    crash("Too many images are loaded, cannot create image buffer");
  }

  /* Set bitmap data: */
  bmp->width = width;
  bmp->height = height;

  /* Allocate image data memory block: */
  if((bmp->data = (byte*) malloc((word)(bmp->width*bmp->height))) == NULL)
  {
    crash("Not enough memory to create image buffer");
  }

  /* Fill with a single color: */
  for(i = 0; i < bmp->width*bmp->height; ++i)
  {
    bmp->data[i] = color;
  }
}

void clear_buffer(bitmap* bmp, byte color)
{
  int i;
  for(i = 0; i < bmp->width*bmp->height; ++i)
  {
    bmp->data[i] = color;
  }
}

void free_buffer(bitmap* bmp)
{
  int i;

  for(i = 0; i < NUM_LOAD_IMG; ++i)
  {
    if(loaded_bitmaps[i] == bmp)
    {
      free(bmp->data);
      loaded_bitmaps[i] = NULL;
    }
  }
}

int get_string_width(char* str)
{
  int x = 0;
  int c;

  for(; *str != '\0'; str++)
  {
    if(*str == ' ')
    {
      x += 5;
    }
    else
    {
      c = *str - 'a';
      if(c < 0 || c >= 26)
      {
        c = *str - 'A';
      }
      if(c >= 0 && c < 26)
      {
        x += font.offset[c + 1] - font.offset[c] + 2;
      }
    }
  }

  return x;
}

void pallet_set(int index, byte r, byte g, byte b)
{
  outp(PALLET_WRITE, index);

  outp(PALLET_DATA, r);
  outp(PALLET_DATA, g);
  outp(PALLET_DATA, b);
}

void tint_pallet(int r_num, int r_den, int g_num, int g_den, int b_num, int b_den)
{
  int i, r, g, b;

  pallet_dirty = true;

  for(i = 0; i < 3 * 248; i += 3)
  {
    r = pallet[i]  * r_num / r_den;
    if(r > 63)
    {
      r = 63;
    }
    g = pallet[i + 1]  * g_num / g_den;
    if(g > 63)
    {
      g = 63;
    }
    b = pallet[i + 2]  * b_num / b_den;
    if(b > 63)
    {
      b = 63;
    }
    pallet[i] = r;
    pallet[i + 1] = g;
    pallet[i + 2] = b;
  }
}

void attenuate_pallet(int numer, int denom)
{
  int i;

  pallet_dirty = true;

  for(i = 0; i < 3 * 248; ++i)
  {
    pallet[i] = pallet[i]  * numer / denom;
  }
}

void color_update()
{
  int i, p;
  int R = 0, G = 0, B = 0;

  ++color_cycle;
  if(color_cycle > 215)
  {
    color_cycle = 0;
  }
  if(color_cycle < 72)
  {
    G = 64 * color_cycle / 72;
    R = 63 - G;
    B = 0;
  }
  else if(color_cycle < 144)
  {
    B = 64 * (color_cycle - 72) / 72;
    G = 63 - B;
    R = 0;
  }
  else
  {
    R = 64 * (color_cycle - 144) / 72;
    B = 63 - R;
    G = 0;
  }
  pallet_set(COLOR_CHROMA_1, R, G, B);
  pallet_set(COLOR_CHROMA_2, G / 2, B / 2, R / 2);
  switch(color_cycle % 24)
  {
    case 0:
      {
        pallet_set(COLOR_SELECT, C_FD_1);
      } break;
    case 2:
      {
        pallet_set(COLOR_SELECT, C_FD_2);
      } break;
    case 4:
      {
        pallet_set(COLOR_SELECT, C_FD_3);
      } break;
    case 6:
      {
        pallet_set(COLOR_SELECT, C_FD_4);
      } break;
    case 8:
      {
        pallet_set(COLOR_SELECT, C_FD_5);
      } break;
    case 10:
      {
        pallet_set(COLOR_SELECT, C_FD_6);
      } break;
    case 12:
      {
        pallet_set(COLOR_SELECT, C_FD_7);
      } break;
    case 14:
      {
        pallet_set(COLOR_SELECT, C_FD_8);
      } break;
    case 16:
      {
        pallet_set(COLOR_SELECT, C_FD_9);
      } break;
    case 18:
      {
        pallet_set(COLOR_SELECT, C_FD_10);
      } break;
    case 20:
      {
        pallet_set(COLOR_SELECT, C_FD_11);
      } break;
    case 22:
      {
        pallet_set(COLOR_SELECT, C_FD_12);
      } break;
  }

  if(pallet_dirty)
  {
    pallet_dirty = false;
    outp(PALLET_WRITE, 0);
    p = 0;
    for(i = 0; i < 248; ++i)
    {
      outp(PALLET_DATA, pallet[p]);
      ++p;
      outp(PALLET_DATA, pallet[p]);
      ++p;
      outp(PALLET_DATA, pallet[p]);
      ++p;
    }
  }
}

void stamp_overlay(bitmap* dst, bitmap* src, int x, int y, bool flip)
{
  int i, j, aW = 0, bW = 0, w, k = 0;
  word screen_offset;
  word bitmap_offset = 0;
  byte data;
  w = src->width;
  if(flip)
  {
    if(x > DISPLAY_WIDTH)
    {
      w += DISPLAY_WIDTH - x;
      aW = x - DISPLAY_WIDTH;
      x = DISPLAY_WIDTH;
    }
    if(x - w < 0)
    {
      w = x;
      bW = src->width - w;
    }
  }
  else
  {
    if(x < 0)
    {
      w += x;
      aW = -x;
      x = 0;
    }
    if(x + w > DISPLAY_WIDTH)
    {
      w = DISPLAY_WIDTH - x;
      bW = src->width - w;
    }
  }

  screen_offset = y * dst->width;
  for(j = 0; j < src->height; j++)
  {
    for(i = 0; i < aW; i++)
    {
      ++k;
      if(k >= 4)
      {
        ++bitmap_offset;
        k = 0;
      }
    }
    for(i = 0; i < w; i++)
    {
      data = ((src->data[bitmap_offset] >> (2*k)) & 3) + COLOR_BLACK;
      if(data != COLOR_TRANSP)
      {
        if(flip)
        {
          dst->data[screen_offset+x-i] = data;
        }
        else
        {
          dst->data[screen_offset+x+i] = data;
        }
      }
      ++k;
      if(k >= 4)
      {
        ++bitmap_offset;
        k = 0;
      }
    }
    for(i = 0; i < bW; i++)
    {
      ++k;
      if(k >= 4)
      {
        ++bitmap_offset;
        k = 0;
      }
    }
    screen_offset += dst->width;
  }
}

void bitmap_overlay(bitmap* bmp, int x, int y)
{
  int i, j;
  word screen_offset;
  byte data;
  byte* reader = bmp->data;

  screen_offset = (y<<8)+(y<<6);
  for(j = 0; j < bmp->height; j++)
  {
    for(i = 0; i < bmp->width; i++, ++reader)
    {
      data = *reader;
      if(data != COLOR_BLACK)
      {
        double_buffer[screen_offset+x+i] = data;
      }
    }
    screen_offset += DISPLAY_WIDTH;
  }
}

void bitmap_underlay(bitmap* dst, bitmap* src, int x, int y)
{
  int i, j;
  word screen_offset;
  byte data;
  byte* reader = src->data;

  screen_offset = y * dst->width;
  for(j = 0; j < src->height; j++)
  {
    for(i = 0; i < src->width; i++, ++reader)
    {
      data = *reader;
      if(dst->data[screen_offset+x+i] == COLOR_BLACK)
      {
        dst->data[screen_offset+x+i] = data;
      }
    }
    screen_offset += dst->width;
  }
}
