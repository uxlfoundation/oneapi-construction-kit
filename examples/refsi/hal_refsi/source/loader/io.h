// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
