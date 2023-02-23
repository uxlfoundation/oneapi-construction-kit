// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _HAL_REFSI_LOADER_IO_H
#define _HAL_REFSI_LOADER_IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

struct exec_state;

void shutdown(intptr_t code) __attribute__((noreturn));
void barrier(struct exec_state *state);
int printm(const char* s, ...);
int vprintm(const char *s, va_list args);
int vsnprint(char* out, size_t n, const char* s, va_list vl);
void putstring(const char *s);
void* memcpy(void* dest, const void* src, size_t len);
void* memset(void* dest, int byte, size_t len);

#endif
