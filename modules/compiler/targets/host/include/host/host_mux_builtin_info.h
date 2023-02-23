// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
