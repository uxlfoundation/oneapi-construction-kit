// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// Host's LLVM pass machinery interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_PASSES_MACHINERY_H_INCLUDED
#define HOST_PASSES_MACHINERY_H_INCLUDED

#include <base/base_pass_machinery.h>

namespace llvm {
class TargetMachine;
}

namespace host {

class HostPassMachinery final : public compiler::BaseModulePassMachinery {
 public:
  HostPassMachinery(llvm::TargetMachine *TM,
                    const compiler::utils::DeviceInfo &Info,
                    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
                    bool verifyEach, compiler::utils::DebugLogging debugLogging,
                    bool timePasses)
      : compiler::BaseModulePassMachinery(TM, Info, BICallback, verifyEach,
                                          debugLogging, timePasses) {}
  ~HostPassMachinery() = default;
  void addClassToPassNames() override;

  void registerPassCallbacks() override;

  void registerPasses() override;

  void printPassNames(llvm::raw_ostream &) override;
};

}  // namespace host

#endif
