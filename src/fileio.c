#include "fileio.h"

void fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
   {
      fgetc(fp);
   }
}
