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

#ifndef HOST_MUX_BUILTIN_INFO_H_INCLUDED
#define HOST_MUX_BUILTIN_INFO_H_INCLUDED

#include <compiler/utils/builtin_info.h>

namespace host {

namespace MiniWGInfoStruct {
enum Type { group_id = 0, num_groups, total };
}

namespace ScheduleInfoStruct {
enum Type {
  global_size = 0,
  global_offset,
  local_size,
  slice,
  total_slices,
  work_dim,
  total
};
}

class HostBIMuxInfo : public compiler::utils::BIMuxInfoConcept {
 public:
  static llvm::StructType *getMiniWGInfoStruct(llvm::Module &M);

  static llvm::StructType *getScheduleInfoStruct(llvm::Module &M);

  llvm::SmallVector<compiler::utils::BuiltinInfo::SchedParamInfo, 4>
  getMuxSchedulingParameters(llvm::Module &M) override;

  llvm::Function *defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                   llvm::Module &M) override;

  llvm::Value *initializeSchedulingParamForWrappedKernel(
      const compiler::utils::BuiltinInfo::SchedParamInfo &Info,
      llvm::IRBuilder<> &B, llvm::Function &IntoF, llvm::Function &) override;
};

}  // namespace host

#endif  // HOST_MUX_BUILTIN_INFO_H_INCLUDED
