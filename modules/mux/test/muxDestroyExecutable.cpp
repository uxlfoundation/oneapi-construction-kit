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

#include "common.h"

struct muxDestroyExecutableTest : public DeviceCompilerTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyExecutableTest);

TEST_P(muxDestroyExecutableTest, Default) {
  // Kernel source.
  const char *nop_source = R"(void kernel nop() {})";

  mux_executable_t executable = nullptr;
  ASSERT_SUCCESS(createMuxExecutable(nop_source, &executable));
  muxDestroyExecutable(device, executable, allocator);
}
