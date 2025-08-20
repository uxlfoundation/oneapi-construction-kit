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
/// @brief Implementation of khr_extended_async_copies extension.

#ifndef EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED
#define EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_khr_extended_async_copies extension.
class khr_extended_async_copies final : public extension {
 public:
  /// @brief Default constructor.
  khr_extended_async_copies();
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED
