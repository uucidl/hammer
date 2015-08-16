/* local implementation for win32 whose std lib doesn't have asprintf */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int asprintf(char**strp, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int non_null_char_count = _vscprintf(fmt, ap);
  va_end(ap);
  
  if (non_null_char_count < 0) {
    return -1;
  }

  size_t buffer_size = 1 + non_null_char_count;
  char* buffer = malloc(buffer_size);
  
  va_start(ap, fmt);
  int ret = vsnprintf_s(buffer, buffer_size, non_null_char_count, fmt, ap);
  if (ret < 0) {
    free(buffer);
    return ret;
  }

  buffer[non_null_char_count] = 0;
  va_end(ap);

  *strp = buffer;
  
  return ret;
}
