#include <stdlib.h>
#include "util.h"

dword adler32(void* buf, int buflength)
{
  int i;
  byte* buffer = (byte*)buf;

  dword s1 = 1;
  dword s2 = 0;

  for(i = 0; i < buflength; ++i)
  {
    s1 = (s1 + buffer[i]) % 65521;
    s2 = (s2 + s1) % 65521;
  }
  return (s2 << 16) | s1;
}

void byte_buffer_add(byte_buffer* buf, void* src, int byte_count)
{
  memcpy(buf->data+buf->cursor, src, byte_count);
  buf->cursor += byte_count;
}

void byte_buffer_get(byte_buffer* buf, void* dst, int byte_count)
{
  memcpy(dst, buf->data+buf->cursor, byte_count);
  buf->cursor += byte_count;
}
