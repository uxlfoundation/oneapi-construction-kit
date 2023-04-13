// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/pass_pipelines.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <multi_llvm/optional_helper.h>
#include <refsi_m1/refsi_pass_machinery.h>
#include <refsi_m1/refsi_wrapper_pass.h>
#include <vecz/pass.h>

refsi_m1::RefSiM1PassMachinery::RefSiM1PassMachinery(
    llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
    const compiler::utils::DeviceInfo &Info,
    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
    bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
    bool timePasses)
    : riscv::RiscvPassMachinery(Ctx, TM, Info, BICallback, verifyEach,
                                debugLogLevel, timePasses) {}

void refsi_m1::RefSiM1PassMachinery::addClassToPassNames() {
  RiscvPassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#include "refsi_pass_registry.def"
}

void refsi_m1::RefSiM1PassMachinery::registerPassCallbacks() {
  RiscvPassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#include "refsi_pass_registry.def"
        return false;
      });
}

void refsi_m1::RefSiM1PassMachinery::printPassNames(llvm::raw_ostream &OS) {
  riscv::RiscvPassMachinery::printPassNames(OS);

  OS << "\nRisc-v specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "refsi_pass_registry.def"
}
