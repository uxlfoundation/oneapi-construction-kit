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

#include <cargo/string_view.h>

#include "Common.h"
#include "Device.h"

// This is a test simply to test the integeration of external files to UnitCL.
using ExampleUnitCLTest_Host = ucl::DeviceTest;

TEST_F(ExampleUnitCLTest_Host, Default) {
  // Since these are host specific test we want to skip it if we aren't
  // running on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP();
  }
  ASSERT_EQ(true, sizeof(size_t) == sizeof(size_t));
}
