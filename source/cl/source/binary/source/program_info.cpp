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

#include <cl/binary/program_info.h>

namespace cl {
namespace binary {

cargo::optional<compiler::ProgramInfo> kernelDeclsToProgramInfo(
    const cargo::small_vector<std::string, 8> &decls, bool store_arg_metadata) {
  compiler::ProgramInfo program_info;
  if (const size_t num_kernels = decls.size()) {
    if (!program_info.resizeFromNumKernels(num_kernels)) {
      return cargo::nullopt;
    }
    for (uint32_t i = 0; i < num_kernels; ++i) {
      if (!kernelDeclStrToKernelInfo(*program_info.getKernel(i),
                                     cargo::string_view(decls[i]),
                                     store_arg_metadata)) {
        return cargo::nullopt;
      }
    }
  }
  return {std::move(program_info)};
}

}  // namespace binary
}  // namespace cl
