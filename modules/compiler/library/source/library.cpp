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

#include "compiler/library.h"

#include "base/context.h"
#include "compiler/info.h"
#include "mux/utils/id.h"

namespace compiler {
const char *llvmVersion() { return CA_COMPILER_LLVM_VERSION; }

const compiler::Info *getCompilerForDevice(mux_device_info_t device_info) {
  if (mux::objectIsInvalid(device_info)) {
    return nullptr;
  }
  // Ensure that device IDs are initialized.
  uint64_t device_infos_length;
  if (mux_success != muxGetDeviceInfos(mux_device_type_all, 0, nullptr,
                                       &device_infos_length)) {
    return nullptr;
  }
  for (const compiler::Info *info : compilers()) {
    if (info->device_info->id == device_info->id) {
      return info;
    }
  }
  return nullptr;
}

std::unique_ptr<Context> createContext() {
  return std::unique_ptr<Context>{new BaseContext};
}
}  // namespace compiler
