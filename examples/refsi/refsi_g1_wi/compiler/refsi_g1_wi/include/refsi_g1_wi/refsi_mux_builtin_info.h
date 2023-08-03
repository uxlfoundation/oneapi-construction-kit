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

#ifndef RISCV_MUX_BUILTIN_INFO_H_INCLUDED
#define RISCV_MUX_BUILTIN_INFO_H_INCLUDED

#include <compiler/utils/builtin_info.h>

namespace refsi_g1_wi {

namespace ExecStateStruct {
enum Type {
  wg = 0,
  num_groups_per_call,
  hal_extra,
  local_id,
  kernel_entry,
  packed_args,
  magic,
  state_size,
  flags,
  next_xfer_id,
  thread_id,
  total
};
}

class RefSiG1BIMuxInfo : public compiler::utils::BIMuxInfoConcept {
 public:
  static llvm::StructType *getExecStateStruct(llvm::Module &M);

  llvm::Function *getOrDeclareMuxBuiltin(
      compiler::utils::BuiltinID ID, llvm::Module &M,
      llvm::ArrayRef<llvm::Type *> OverloadInfo = {}) override;

  llvm::Function *defineMuxBuiltin(
      compiler::utils::BuiltinID ID, llvm::Module &M,
      llvm::ArrayRef<llvm::Type *> OverloadInfo = {}) override;
};

}  // namespace refsi_g1_wi

#endif  // RISCV_MUX_BUILTIN_INFO_H_INCLUDED
