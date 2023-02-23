// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// Host's LLVM pass machinery interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/pass_pipelines.h>
#include <compiler/module.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pipeline_parse_helpers.h>
#include <host/add_entry_hook_pass.h>
#include <host/add_floating_point_control_pass.h>
#include <host/disable_neon_attribute_pass.h>
#include <host/host_pass_machinery.h>
#include <host/remove_byval_attributes_pass.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/optional_helper.h>
#include <vecz/pass.h>

namespace host {

bool hostVeczPassOpts(llvm::Function &F, llvm::ModuleAnalysisManager &MAM,
                      llvm::SmallVectorImpl<vecz::VeczPassOptions> &Opts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (vecz_mode != compiler::VectorizationMode::ALWAYS &&
      vecz_mode != compiler::VectorizationMode::AUTO) {
    return false;
  }
  // We only vectorize kernels
  if (!compiler::utils::isKernelEntryPt(F)) {
    return false;
  }
  const auto &DI =
      MAM.getResult<compiler::utils::DeviceInfoAnalysis>(*F.getParent());
  auto max_work_width = DI.max_work_width;

  vecz::VeczPassOptions vecz_options;
  vecz_options.choices.enable(vecz::VectorizationChoices::eDivisionExceptions);

  const char *choices_string = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (choices_string &&
      !vecz_options.choices.parseChoicesString(choices_string)) {
    llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    return false;
  }

  auto local_sizes = compiler::utils::getLocalSizeMetadata(F);

  const uint32_t local_vec_dim = 0;
  const uint32_t local_size = local_sizes ? (*local_sizes)[local_vec_dim] : 0;

  vecz_options.vec_dim_idx = local_vec_dim;
  vecz_options.vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;

  vecz_options.local_size = local_size;

  // Although we can vectorize to much wider than 16, it is often
  // not beneficial to do so. Thus when the vectorization mode is
  // ALWAYS, we cap it at 16 as a compromise to prevent execution
  // times from becoming too long in UnitCL. When the mode is AUTO,
  // it may decide to vectorize narrower than the given width, but
  // never wider.
  const uint32_t work_width =
      (vecz_mode == compiler::VectorizationMode::ALWAYS) ? 16u : max_work_width;

  // The final vector width will be the kernel's dynamic work width,
  // and dynamic work width must not exceed the device's maximum
  // work width, so cap it before we even attempt vectorization.
  // Only try to vectorize to widths of powers of two.
  const uint32_t SIMDWidth = llvm::PowerOf2Floor(
      local_size != 0 ? std::min(local_size, work_width) : work_width);

  vecz_options.factor =
      compiler::utils::VectorizationFactor::getFixedWidth(SIMDWidth);

  Opts.push_back(vecz_options);
  return true;
}

void HostPassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();

#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);

#include "host_pass_registry.def"
}

/// @brief Helper functions for parsing options
/// @note These functions are small but keep the def file simpler and is in line
/// with PassBuilder.cpp
llvm::Expected<bool> parseFloatPointControlPassOptions(llvm::StringRef Params) {
  return compiler::utils::parseSinglePassOption(Params, "ftz",
                                                "FloatPointControlPass");
}

void HostPassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "host_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

void HostPassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS)   \
  if (compiler::utils::checkParametrizedPassName(Name, NAME)) {             \
    auto Params = compiler::utils::parsePassParameters(PARSER, Name, NAME); \
    if (!Params) {                                                          \
      llvm::errs() << toString(Params.takeError()) << "\n";                 \
      return false;                                                         \
    }                                                                       \
    PM.addPass(CREATE_PASS(Params.get()));                                  \
    return true;                                                            \
  }

#include "host_pass_registry.def"
        return false;
      });
}

void HostPassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);
  OS << "\nHost passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "host_pass_registry.def"

  OS << "Module passes with params:\n";
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  compiler::utils::printPassName(NAME, PARAMS, OS);
#include "host_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "host_pass_registry.def"
}

}  // namespace host
