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

/// @file
///
/// @brief Device Hardware Abstraction Layer runtime implementation loader.

#ifndef HAL_LIBRARY_H_INCLUDED
#define HAL_LIBRARY_H_INCLUDED

#include <string>

namespace hal {
/// @addtogroup hal
/// @{

struct hal_t;

using hal_library_t = void *;

// Returns the file name of the HAL library for the given device.
// Returns empty string on error.
std::string get_hal_library_path(const char *device_name);

// Try to load a HAL given a path to a library file. The path can be relative.
// If the expected API version is nonzero, loading a HAL library with a
// different API version results in an error.
// Returns a pointer to a HAL object and a library handle on success.
// Returns a nullptr on error.
hal_t *load_hal_library(const char *library_path, uint32_t expected_api_version,
                        hal_library_t &handle);

// Try to load a HAL based on the environment and default device name. If the
// expected API version is nonzero, loading a HAL library with a different API
// version results in an error.
// Returns a pointer to a HAL object and a library handle on success.
// Returns a nullptr on error.
hal_t *load_hal(const char *default_device, uint32_t expected_api_version,
                hal_library_t &handle);

// Unload a HAL library using the specified handle. HAL devices must have been
// deleted prior to calling this function.
void unload_hal(hal_library_t handle);

/// @}
}  // namespace hal

#endif  // HAL_LIBRARY_H_INCLUDED
