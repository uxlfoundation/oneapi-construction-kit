// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <stdbool.h>
#include "io.h"
#include "device/device_if.h"
#include "device/memory_map.h"
#include "device/io_impl.h"

int snprint(char* out, size_t n, const char* s, ...)
{
  va_list vl;
  va_start(vl, s);
  int res = vsnprint(out, n, s, vl);
  va_end(vl);
  return res;
}

size_t strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2)
{
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

char* strcpy(char* dest, const char* src)
{
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

static int startswith(const char *s1, const char *s2)
{
  while (*s1 && *s2)
  {
    if (*s1 != *s2)
    {
       return 0;
    }
    s1++;
    s2++;
  }
  return *s2 == 0;
}

char* strstr(const char *haystack, const char *needle)
{
  while (*haystack)
  {
    if (startswith(haystack, needle) == 1)
    {
      return (char *)haystack;
    }
    haystack++;
  }
  return NULL;
}

long atol(const char* str)
{
  long res = 0;
  int sign = 0;

  while (*str == ' ')
    str++;

  if (*str == '-' || *str == '+') {
    sign = *str == '-';
    str++;
  }

  while (*str) {
    res *= 10;
    res += *str++ - '0';
  }

  return sign ? -res : res;
}
