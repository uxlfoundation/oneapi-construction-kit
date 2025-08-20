// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _MUX_UTIL_H
#define _MUX_UTIL_H

#include <core/core.h>
#include <stddef.h>
#include <stdint.h>

// Create a Mux device with the specified type. If multiple devices exist with
// this type, use the first one returned by coreGetDeviceInfos.
core_device_t createDevice(core_device_type_e type,
                           core_allocator_info_t allocator);

// Allocate memory for core_allocator_info_t with the given size and minimum
// alignment.
void *ExampleAlloc(void *user_data, size_t size, size_t alignment);

// Free memory allocated by ExampleAlloc through core_allocator_info_t.
void ExampleFree(void *user_data, void *pointer);

// Find the ID of the first supported heap in the heaps bitmap.
uint32_t findFirstSupportedHeap(uint32_t heaps);

#endif  // _MUX_UTIL_H
