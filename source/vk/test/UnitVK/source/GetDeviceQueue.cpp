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

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetDeviceQueue

class GetDeviceQueue : public uvk::DeviceTest {
 public:
  GetDeviceQueue() : queue(VK_NULL_HANDLE) {}
  VkQueue queue;
};

TEST_F(GetDeviceQueue, Default) {
  // since the device create info structs are initialized with = {}
  // in DeviceTest the index values are 0 by default
  vkGetDeviceQueue(device, 0, 0, &queue);

  ASSERT_NE(nullptr, queue);
}
