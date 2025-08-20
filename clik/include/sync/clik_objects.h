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

#ifndef _CLIK_RUNTIME_CLIK_OBJECTS_H
#define _CLIK_RUNTIME_CLIK_OBJECTS_H

#include <string>

#include "clik_sync_api.h"

namespace hal {
using hal_program_t = uint64_t;
using hal_library_t = void *;
struct hal_t;
struct hal_device_t;
}  // namespace hal

// Contains state required for a device object.
struct clik_device {
  // Low-level interface to the device.
  hal::hal_device_t *hal_device;
  // HAL object used to create and delete HAL devices.
  hal::hal_t *hal;
  // Handle to the HAL library.
  hal::hal_library_t library;
};

// Contains state required for a program object.
struct clik_program {
  // Refers to the device object this program was created for.
  clik_device *device;
  // ELF program read from a binary.
  hal::hal_program_t elf;
};

// Contains state required for a buffer object.
struct clik_buffer {
  // Refers to the device object this buffer was created for.
  clik_device *device;
  // Address of the memory allocated for the buffer, in device memory.
  uint64_t device_addr;
  // Size of the buffer, in bytes.
  uint64_t size;
};

#endif
