#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

typedef unsigned char  byte;
typedef unsigned short word;

void fskip(FILE *fp, int num_bytes)
{
   int i;
   for(i = 0; i < num_bytes; i++)
   {
      fgetc(fp);
   }
}

int main()
{
  byte* data;
  FILE* ifp;
  FILE* ofp;

  word num_colors;
  word width, rW;
  word height;
  long index;
  int x, y, i, r;
  byte write_byte;
  word write_word;

  char* in_file = "font.bmp";
  char* out_file = "..\\res\\font.fnt";

  union REGS regs;

  regs.h.ah = 0x00;
  regs.h.al = 0x13;
  int86(0x10, &regs, &regs);

  if((ifp = fopen(in_file, "rb")) == NULL)
  {
    printf("Cannot open file %s\n", in_file);
    exit(1);
  }

  if((ofp = fopen(out_file, "wb")) == NULL)
  {
    fclose(ifp);
    printf("Cannot open file %s\n", out_file);
    exit(1);
  }

  if(fgetc(ifp)!='B' || fgetc(ifp)!='M')
  {
    fclose(ifp);
    fclose(ofp);
    printf("%s is not a bitmap file\n", in_file);
    exit(1);
  }

  fskip(ifp,16);
  fread(&width,      sizeof(word), 1, ifp);
  fskip(ifp,2);
  fread(&height,     sizeof(word), 1, ifp);
  fskip(ifp,22);
  fread(&num_colors, sizeof(word), 1, ifp);
  fskip(ifp,6);

  if(num_colors == 0)
  {
    num_colors = 256;
  }

  fskip(ifp, num_colors*4);

  if((data = (byte*) malloc((word)(width * height))) == NULL)
  {
    fclose(ifp);
    fclose(ofp);
    printf("Not enough memory to process %s\n", in_file);
    exit(1);
  }

  rW = (4 - (width % 4)) % 4;
  for(index = (height-1)*width; index >= 0; index -= width)
  {
    for(x = 0; x < width; x++)
    {
      data[(word)index+x] = (byte) fgetc(ifp);
    }
    for(x = 0; x < rW; x++)
    {
      fgetc(ifp);
    }
  }

  write_word = (word) width;
  fwrite(&write_word, sizeof(word), 1, ofp);
  write_word = (word) height;
  fwrite(&write_word, sizeof(word), 1, ofp);
  write_word = (word) (height + 7) / 8;
  fwrite(&write_word, sizeof(word), 1, ofp);

  for(i = 0; i < 26; i++)
  {
    printf("Offset for \"%c\":", 'A'+i);
    while((r = scanf("%d", &x)) == 0) {}
    write_word = (word) x;
    fwrite(&write_word, sizeof(word), 1, ofp);
  }

  for(x = 0; x < width; x++)
  {
    for(y = 0; y < height; y += 8)
    {
      write_byte = 0;
      for(i = 0; i < 8 && (y + i) < height; i++)
      {
        if(data[x + ((y + i) * width)])
        {
          write_byte |= 1 << i;
        }
      }
      fwrite(&write_byte, sizeof(byte), 1, ofp);
    }
  }

  fclose(ifp);
  fclose(ofp);

  return 0;
}

/*

  ~~ FONT FILE DATA ~~

  word: width
  word: height
  word: bytes_per_row
  word: a_x_offset
  word: b_x_offset
  word: c_x_offset
  word: d_x_offset
  ...
  word: z_x_offset
  bit: first pixel (indexed top to bottom, then left to right)
  bit: second pixel
  ...
  ... (pixels starting a new column are byte aligned, hence our bytes_per_row)
  ...
  bit: last pixel

*/
