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

#include "kts/arguments_shared.h"

/// @brief Default global size for 1D kernels.
const size_t kts::N = 256;

/// @brief Default local size for 1D kernels that make use of work-groups.
size_t kts::localN = 16;

kts::BufferDesc::BufferDesc()
    : size_(0), streamer_(nullptr), streamer2_(nullptr) {}

kts::BufferDesc::BufferDesc(size_t size,
                            std::shared_ptr<BufferStreamer> streamer)
    : size_(size), streamer_(streamer) {}

kts::BufferDesc::BufferDesc(size_t size,
                            std::shared_ptr<BufferStreamer> streamer,
                            std::shared_ptr<BufferStreamer> streamer2)
    : size_(size), streamer_(streamer), streamer2_(streamer2) {}
