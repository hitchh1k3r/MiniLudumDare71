#ifndef UTIL_H_
#define UTIL_H_

#include "types.h"

typedef struct
{
  byte data[256];
  byte cursor;
} byte_buffer;

dword adler32(void* buf, int buflength);
void byte_buffer_add(byte_buffer* buf, void* src, int byte_count);

#endif
