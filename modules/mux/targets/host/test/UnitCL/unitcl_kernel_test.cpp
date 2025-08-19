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

#include <cargo/string_view.h>
// In-house headers
#include "Common.h"
#include "Device.h"
#include "kts/vecz_tasks_common.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Host_01_Example) {
  // Since these are host specific test we want to skip it if we aren't
  // running on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP();
  }

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}
