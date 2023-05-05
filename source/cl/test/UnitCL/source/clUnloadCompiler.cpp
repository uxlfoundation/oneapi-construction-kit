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

#include "Common.h"

using clUnloadCompilerTest = testing::Test;

// This test is disabled because the function is deprecated in OpenCL 1.2, and
// is fundamentally incompatible with an ICD (due to not taking a platform
// parameter).  See clUnloadPlatformCompiler instead.
TEST_F(clUnloadCompilerTest, DISABLED_Default) {
  ASSERT_SUCCESS(clUnloadCompiler());
}
