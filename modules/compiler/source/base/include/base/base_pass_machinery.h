// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Base module pass machinery to be used for BaseModule's PassManager
/// state
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_PASS_MACHINERY_H_INCLUDED
#define BASE_PASS_MACHINERY_H_INCLUDED

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/device_info.h>
#include <compiler/utils/pass_machinery.h>
#include <mux/mux.h>

namespace llvm {
class Module;
class ModulePass;
}  // namespace llvm

namespace compiler {
/// @addtogroup compiler
/// @{

class BaseModulePassMachinery : public compiler::utils::PassMachinery {
 public:
  BaseModulePassMachinery(
      llvm::TargetMachine *TM, llvm::Optional<compiler::utils::DeviceInfo> Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses)
      : compiler::utils::PassMachinery(TM, verifyEach, debugLogging),
        TimePasses(timePasses),
        Info(Info),
        BICallback(BICallback) {}

  virtual void registerPasses() override;
  virtual void addClassToPassNames() override;
  virtual void registerPassCallbacks() override;
  virtual void printPassNames(llvm::raw_ostream &OS) override;

 private:
  llvm::TimePassesHandler TimePasses;

  /// @brief Device-specific information about the ComputeMux target being
  /// compiled for.
  llvm::Optional<compiler::utils::DeviceInfo> Info;

  /// @brief An optional callback function that provides target-specific
  /// BuiltinInfo information to supply to the BuiltinInfoAnalysis analysis
  /// pass.
  std::function<compiler::utils::BuiltinInfo(const llvm::Module &)> BICallback;
};

/// @brief A function which transfers 'mux' device properties to 'compiler'
/// ones.
///
/// FIXME: Ideally we wouldn't have any mux in the compiler library. See
/// CA-4236.
static inline compiler::utils::DeviceInfo initDeviceInfoFromMux(
    mux_device_info_t device_info) {
  if (!device_info) {
    return compiler::utils::DeviceInfo{};
  }

  return compiler::utils::DeviceInfo(
      device_info->half_capabilities, device_info->float_capabilities,
      device_info->double_capabilities, device_info->max_work_width);
}

/// @}
}  // namespace compiler
#endif  // BASE_PASS_MACHINERY_H_INCLUDED
