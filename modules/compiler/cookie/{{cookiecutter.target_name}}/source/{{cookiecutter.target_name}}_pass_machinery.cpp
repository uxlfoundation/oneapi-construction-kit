/// Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}


#include <base/pass_pipelines.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <multi_llvm/optional_helper.h>
#include <{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h>
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
#include <{{cookiecutter.target_name}}/refsi_wrapper_pass.h>
{% endif -%}
#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <vecz/pass.h>

using namespace llvm;

namespace {{cookiecutter.target_name}} {
  {{cookiecutter.target_name.capitalize()}}PassMachinery::{{cookiecutter.target_name.capitalize()}}PassMachinery(
      llvm::TargetMachine * TM, const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
      bool timePasses)
      : compiler::BaseModulePassMachinery(TM, Info, BICallback, verifyEach,
                                          debugLogLevel, timePasses) {}

// Process vecz flags based off build options and environment variables
// return true if we want to vectorize
llvm::Optional<vecz::VeczPassOptions> processVeczFlags() {
  vecz::VeczPassOptions vecz_options;
  // The minimum number of elements to vectorize for. For a fixed-length VF,
  // this is the exact number of elements to vectorize by. For scalable VFs,
  // the actual number of elements is a multiple (vscale) of these, unknown at
  // compile time. Default is scalar, can be updated here.
  vecz_options.factor = compiler::utils::VectorizationFactor::getScalar();

  vecz_options.choices.enable(
      vecz::VectorizationChoices::eDivisionExceptions);

  // This is of the form of a comma separated set of fields
  // S     - use scalable vectorization
  // V     - vectorize only, otherwise produce both scalar and vector kernels
  // A     - let vecz automatically choose the vectorization factor
  // 1-64  - vectorization factor multiplier: the fixed amount itself, or the
  //         value that multiplies the scalable amount
  // VP    - produce a vector-predicated kernel
  if (const auto *vecz_vf_flags_env =
          std::getenv("CA_{{cookiecutter.target_name_capitals}}_VF")) {
    // Set scalable to off and let users add it explicitly with 'S'.
    vecz_options.factor.setIsScalable(false);
    llvm::SmallVector<llvm::StringRef, 4> flags;
    llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "A" || r == "a") {
        vecz_options.vecz_auto = true;
      } else if (r == "V" || r == "v") {
        // 'V is no longer a vectorization choice but is legally found as part
        // of this variable. See hasForceNoTail, below.
      } else if (r == "S" || r == "s") {
        vecz_options.factor.setIsScalable(true);
      } else if (isdigit(r[0])) {
        vecz_options.factor.setKnownMin(std::stoi(r.str()));
      } else if (r == "VP" || r == "vp") {
        vecz_options.choices.enable(
            vecz::VectorizationChoices::eVectorPredication);
      } else {
        return llvm::None;
      }
    }
  }

  // Choices override the cost model
  const char *ptr = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (ptr) {
    bool success = vecz_options.choices.parseChoicesString(ptr);
    if (!success) {
      llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    }
  }

  return vecz_options;
}

bool {{cookiecutter.target_name.capitalize()}}VeczPassOpts(
         llvm::Function &F, llvm::ModuleAnalysisManager &,
         llvm::SmallVectorImpl<vecz::VeczPassOptions> &PassOpts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (!compiler::utils::isKernelEntryPt(F) ||
      F.hasFnAttribute(llvm::Attribute::OptimizeNone) ||
      vecz_mode == compiler::VectorizationMode::NEVER) {
    return false;
  }
  llvm::Optional<vecz::VeczPassOptions> vecz_options = processVeczFlags();
  if (!vecz_options.hasValue()) {
    return false;
  }
  vecz_options->vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;
  vecz_options->vec_dim_idx = 0;
  PassOpts.push_back(*vecz_options);
  return true;
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_PASS_NO_PARSE(NAME, CLASS) PIC.addClassToPassName(CLASS, NAME);
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define CGSCC_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#include "{{cookiecutter.target_name}}_pass_registry.def"
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "{{cookiecutter.target_name}}_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
        (void) Name;
        (void) PM;
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }

#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  if (utils::checkParametrizedPassName(Name, NAME)) {                     \
    auto Params = utils::parsePassParameters(PARSER, Name, NAME);         \
    if (!Params) {                                                        \
      errs() << toString(Params.takeError()) << "\n";                     \
      return false;                                                       \
    }                                                                     \
    PM.addPass(CREATE_PASS(Params.get()));                                \
    return true;                                                          \
  }

#define MODULE_ANALYSIS(NAME, CREATE_PASS)                             \
  if (Name == "require<" NAME ">") {                                   \
    PM.addPass(RequireAnalysisPass<                                    \
               std::remove_reference<decltype(CREATE_PASS)>::type,     \
               llvm::Module>());                                       \
    return true;                                                       \
  }                                                                    \
  if (Name == "invalidate<" NAME ">") {                                \
    PM.addPass(InvalidateAnalysisPass<                                 \
               std::remove_reference<decltype(CREATE_PASS)>::type>()); \
    return true;                                                       \
  }

#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                \
  if (Name == "require<" NAME ">") {                                        \
    PM.addPass(createModuleToFunctionPassAdaptor(                           \
        RequireAnalysisPass<std::remove_reference_t<decltype(CREATE_PASS)>, \
                            Function>()));                                  \
    return true;                                                            \
  }                                                                         \
  if (Name == "invalidate<" NAME ">") {                                     \
    PM.addPass(createModuleToFunctionPassAdaptor(                           \
        InvalidateAnalysisPass<                                             \
            std::remove_reference_t<decltype(CREATE_PASS)>>()));            \
    return true;                                                            \
  }

#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS)   \
  if (utils::checkParametrizedPassName(Name, NAME)) {                         \
    auto Params = utils::parsePassParameters(PARSER, Name, NAME);             \
    if (!Params) {                                                            \
      errs() << toString(Params.takeError()) << "\n";                         \
      return false;                                                           \
    }                                                                         \
    PM.addPass(createModuleToFunctionPassAdaptor(CREATE_PASS(Params.get()))); \
    return true;                                                              \
  }

#define FUNCTION_PASS(NAME, CREATE_PASS)                        \
  if (Name == NAME) {                                           \
    PM.addPass(createModuleToFunctionPassAdaptor(CREATE_PASS)); \
    return true;                                                \
  }

#define CGSCC_PASS(NAME, CREATE_PASS)                                 \
  if (Name == NAME) {                                                 \
    PM.addPass(createModuleToPostOrderCGSCCPassAdaptor(CREATE_PASS)); \
    return true;                                                      \
  }
#include "{{cookiecutter.target_name}}_pass_registry.def"
        return false;
      });
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);

  OS << "\n{{cookiecutter.target_name}} specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"
  OS << "Module passes with params:\n";
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  utils::printPassName(NAME, PARAMS, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"
  OS << "Function analyses:\n";
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Function passes:\n";
#define FUNCTION_PASS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Function passes with params:\n";
#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  utils::printPassName(NAME, PARAMS, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "CGSCC passes:\n";
#define CGSCC_PASS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"
}
}
