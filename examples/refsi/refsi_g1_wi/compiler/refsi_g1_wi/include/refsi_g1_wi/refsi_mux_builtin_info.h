// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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

  llvm::Function *getOrDeclareMuxBuiltin(compiler::utils::BuiltinID ID,
                                         llvm::Module &M) override;

  llvm::Function *defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                   llvm::Module &M) override;
};

}  // namespace refsi_g1_wi

#endif  // RISCV_MUX_BUILTIN_INFO_H_INCLUDED
