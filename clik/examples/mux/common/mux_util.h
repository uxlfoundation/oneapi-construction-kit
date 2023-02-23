// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
