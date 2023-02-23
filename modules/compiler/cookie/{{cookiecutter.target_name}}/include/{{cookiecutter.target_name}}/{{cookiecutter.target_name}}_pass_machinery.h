// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED
#define {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED

#include <base/base_pass_machinery.h>

namespace {{cookiecutter.target_name}} {

/// @brief Version of PassMachinery used in {{cookiecutter.target_name}}
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class {{cookiecutter.target_name.capitalize()}}PassMachinery : public compiler::BaseModulePassMachinery {
 public:
  {{cookiecutter.target_name.capitalize()}}PassMachinery(llvm::TargetMachine * TM,
                const compiler::utils::DeviceInfo &Info,
                compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
                bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
                bool timePasses);

  void addClassToPassNames() override;

  void registerPasses() override;

  void registerPassCallbacks() override;

  void printPassNames(llvm::raw_ostream &OS) override;
};

}  // namespace {{cookiecutter.target_name}}

#endif  // {{cookiecutter.target_name_capitals}}_PASS_MACHINERY_H_INCLUDED
