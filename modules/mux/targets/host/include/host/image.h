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
/// Host's image interface.

#ifndef HOST_IMAGE_H_INCLUDED
#define HOST_IMAGE_H_INCLUDED

#include <mux/mux.h>

#ifdef HOST_IMAGE_SUPPORT
#include <libimg/host.h>
#endif

namespace host {
/// @addtogroup host
/// @{

struct image_s final : public mux_image_s {
  /// @brief Host image constructor.
  image_s(mux_memory_requirements_s memory_requirements, mux_image_type_e type,
          mux_image_format_e format, uint32_t pixel_size, uint32_t width,
          uint32_t height, uint32_t depth, uint32_t array_layers,
          uint64_t row_size, uint64_t slice_size);

#ifdef HOST_IMAGE_SUPPORT
  libimg::HostImage image;
#endif
};

/// @}
}  // namespace host

#endif  // HOST_IMAGE_H_INCLUDED
