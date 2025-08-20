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

/// @file
///
/// @brief Host builtin kernel support.

#ifndef HOST_BUILTIN_KERNEL_H_INCLUDED
#define HOST_BUILTIN_KERNEL_H_INCLUDED

#include <host/kernel.h>
#include <mux/mux.h>

#include <map>
#include <string>

namespace host {
/// @addtogroup host
/// @{

using builtin_kernel_map =
    std::map<std::string, ::host::kernel_variant_s::entry_hook_t>;

/// @brief Get the map of supported builtin kernels.
///
/// @param[in] device_info Pointer to device information.
///
/// @return Returns the builtin kernel map if present.
builtin_kernel_map getBuiltinKernels(mux_device_info_t device_info);

/// @}
}  // namespace host

#endif  // HOST_BUILTIN_KERNEL_H_INCLUDED
