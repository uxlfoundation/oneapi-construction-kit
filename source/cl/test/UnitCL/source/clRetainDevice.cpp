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

using clRetainDeviceTest = ucl::DeviceTest;

TEST_F(clRetainDeviceTest, Default) {
  ASSERT_SUCCESS(clRetainDevice(device));
  ASSERT_SUCCESS(clReleaseDevice(device));
}

TEST_F(clRetainDeviceTest, BadDevice) {
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, clRetainDevice(nullptr));
}
