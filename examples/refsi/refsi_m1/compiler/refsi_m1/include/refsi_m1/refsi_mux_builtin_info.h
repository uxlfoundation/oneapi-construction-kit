// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_MUX_BUILTIN_INFO_H_INCLUDED
#define REFSI_MUX_BUILTIN_INFO_H_INCLUDED

#include <compiler/utils/builtin_info.h>

namespace refsi_m1 {

class RefSiM1BIMuxInfo : public compiler::utils::BIMuxInfoConcept {
 public:
  llvm::Function *defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                   llvm::Module &M) override;
};

}  // namespace refsi_m1

#endif  // REFSI_MUX_BUILTIN_INFO_H_INCLUDED
