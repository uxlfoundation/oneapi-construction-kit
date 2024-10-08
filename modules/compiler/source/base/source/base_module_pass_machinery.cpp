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

#include <base/base_module_pass_machinery.h>
#include <base/bit_shift_fixup_pass.h>
#include <base/builtin_simplification_pass.h>
#include <base/check_for_ext_funcs_pass.h>
#include <base/check_for_unsupported_types_pass.h>
#include <base/combine_fpext_fptrunc_pass.h>
#include <base/fast_math_pass.h>
#include <base/image_argument_substitution_pass.h>
#include <base/mem_to_reg_pass.h>
#include <base/pass_pipelines.h>
#include <base/printf_replacement_pass.h>
#include <base/set_convergent_attr_pass.h>
#include <base/software_division_pass.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/compute_local_memory_usage_pass.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/define_mux_dma_pass.h>
#include <compiler/utils/degenerate_sub_group_pass.h>
#include <compiler/utils/encode_builtin_range_metadata_pass.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/fixup_calling_convention_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/lower_to_mux_builtins_pass.h>
#include <compiler/utils/make_function_name_unique_pass.h>
#include <compiler/utils/manual_type_legalization_pass.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/optimal_builtin_replacement_pass.h>
#include <compiler/utils/pipeline_parse_helpers.h>
#include <compiler/utils/prepare_barriers_pass.h>
#include <compiler/utils/reduce_to_function_pass.h>
#include <compiler/utils/remove_exceptions_pass.h>
#include <compiler/utils/remove_lifetime_intrinsics_pass.h>
#include <compiler/utils/rename_builtins_pass.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_atomic_funcs_pass.h>
#include <compiler/utils/replace_c11_atomic_funcs_pass.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <compiler/utils/replace_mux_math_decls_pass.h>
#include <compiler/utils/replace_target_ext_tys_pass.h>
#include <compiler/utils/replace_wgc_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/sub_group_analysis.h>
#include <compiler/utils/unique_opaque_structs_pass.h>
#include <compiler/utils/verify_reqd_sub_group_size_pass.h>
#include <compiler/utils/work_item_loops_pass.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FormatVariadic.h>
#include <metadata/handler/generic_metadata.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <vecz/pass.h>
#include <vecz/vecz_target_info.h>

#include <cstdint>
#include <unordered_map>

using namespace llvm;

namespace compiler {
/// @addtogroup compiler
/// @{

void BaseModulePassMachinery::setCompilerOptions(
    const compiler::Options &opts) {
  options = opts;
}

void BaseModulePassMachinery::addClassToPassNames() {
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

#include "base_module_pass_registry.def"
}

static Expected<compiler::utils::AddKernelWrapperPassOptions>
parseAddKernelWrapperPassOptions(StringRef Params) {
  compiler::utils::AddKernelWrapperPassOptions Opts;

  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName.consume_front("packed")) {
      Opts.IsPackedStruct = true;
    } else if (ParamName.consume_front("unpacked")) {
      Opts.IsPackedStruct = false;
    } else if (ParamName.consume_front("local-buffers-by-size")) {
      Opts.PassLocalBuffersBySize = true;
    } else if (ParamName.consume_front("local-buffers-by-ptr")) {
      Opts.PassLocalBuffersBySize = false;
    } else {
      return make_error<StringError>(
          formatv("invalid AddKernelWrapperPass pass parameter '{0}' ",
                  ParamName)
              .str(),
          inconvertibleErrorCode());
    }
  }

  return Opts;
}

static Expected<StringRef> parseMakeFunctionNameUniquePassOptions(
    StringRef Params) {
  return compiler::utils::parseSinglePassStringRef(Params);
}

template <size_t N>
static ErrorOr<std::array<std::optional<uint64_t>, N>> parseIntList(
    StringRef OptionVal, bool AllowNegative = false) {
  std::array<std::optional<uint64_t>, N> Arr;
  for (unsigned i = 0; i < N; i++) {
    int64_t Res;
    StringRef Val;
    std::tie(Val, OptionVal) = OptionVal.split(':');
    // Allow the skipping of everything but the first value
    if (i > 0 && Val.empty()) {
      break;
    }
    // Else, parse it as an integer.
    if (Val.getAsInteger(0, Res)) {
      return std::errc::invalid_argument;
    }
    if (Res >= 0 || AllowNegative) {
      Arr[i] = Res;
    }
  }
  // Check the user isn't passing extra values we're ignoring.
  if (!OptionVal.empty()) {
    return std::errc::argument_list_too_long;
  }
  return std::array<std::optional<uint64_t>, 3U>(Arr);
}

constexpr const char LocalSizesOptName[] = "max-local-sizes=";
constexpr const char GlobalSizesOptName[] = "max-global-sizes=";

static Expected<compiler::utils::EncodeBuiltinRangeMetadataOptions>
parseEncodeBuiltinRangeMetadataPassOptions(StringRef Params) {
  compiler::utils::EncodeBuiltinRangeMetadataOptions Opts;
  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    StringRef OptName;
    std::array<std::optional<uint64_t>, 3> *SizesPtr = nullptr;
    if (ParamName.consume_front(LocalSizesOptName)) {
      OptName = LocalSizesOptName;
      SizesPtr = &Opts.MaxLocalSizes;
    } else if (ParamName.consume_front(GlobalSizesOptName)) {
      OptName = GlobalSizesOptName;
      SizesPtr = &Opts.MaxGlobalSizes;
    } else {
      return make_error<StringError>(
          formatv(
              "invalid EncodeBuiltinRangeMetadataPass pass parameter '{0}' ",
              ParamName)
              .str(),
          inconvertibleErrorCode());
    }

    if (auto Array = parseIntList<3>(ParamName)) {
      *SizesPtr = *Array;
    } else {
      return make_error<StringError>(
          formatv("invalid max work-group size parameter to "
                  "EncodeBuiltinRangeMetadataPass pass "
                  "{0} parameter: '{1}' ",
                  OptName.drop_back(1), ParamName)
              .str(),
          inconvertibleErrorCode());
    }
  }

  return Opts;
}

static Expected<compiler::utils::EncodeKernelMetadataPassOptions>
parseEncodeKernelMetadataPassOptions(StringRef Params) {
  compiler::utils::EncodeKernelMetadataPassOptions Opts;
  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName.consume_front("name=")) {
      Opts.KernelName = ParamName.str();
    } else if (ParamName.consume_front("local-sizes=")) {
      // Fill the optional value in with default values - we require each to be
      // overwritten by the user here anyway.
      Opts.LocalSizes = {1, 1, 1};
      if (auto Array = parseIntList<3>(ParamName)) {
        for (unsigned i = 0; i < 3; i++) {
          if (auto v = (*Array)[i]) {
            (*Opts.LocalSizes)[i] = *v;
          } else {
            return make_error<StringError>(
                formatv(
                    "invalid local-sizes parameter to EncodeKernelMetadataPass "
                    "- all 3 dimensions must be provided: '{0}'",
                    ParamName)
                    .str(),
                inconvertibleErrorCode());
          }
        }
      } else {
        return make_error<StringError>(
            formatv("invalid local-sizes parameter to EncodeKernelMetadataPass "
                    ": '{0}'",
                    ParamName)
                .str(),
            inconvertibleErrorCode());
      }
    } else {
      return make_error<StringError>(
          formatv("invalid EncodeKernelMetadataPass parameter '{0}' ",
                  ParamName)
              .str(),
          inconvertibleErrorCode());
    }
  }

  if (Opts.KernelName.empty()) {
    return make_error<StringError>(
        "EncodeKernelMetadataPass must be provided a 'name'",
        inconvertibleErrorCode());
  }

  return Opts;
}

static Expected<bool> parseReplaceMuxMathDeclsPassOptions(StringRef Params) {
  return compiler::utils::parseSinglePassOption(Params, "fast",
                                                "ReplaceMuxMathDeclsPass");
}

// Lookup table for calling convention enums
static std::unordered_map<std::string, CallingConv::ID> CallConvMap = {
    {"C", CallingConv::C},
    {"Fast", CallingConv::Fast},
    {"Cold", CallingConv::Cold},
    {"GHC", CallingConv::GHC},
    {"HiPE", CallingConv::HiPE},
#if LLVM_VERSION_LESS(18, 0)
    {"WebKit_JS", CallingConv::WebKit_JS},
#endif
    {"AnyReg", CallingConv::AnyReg},
    {"PreserveMost", CallingConv::PreserveMost},
    {"PreserveAll", CallingConv::PreserveAll},
    {"Swift", CallingConv::Swift},
    {"CXX_FAST_TLS", CallingConv::CXX_FAST_TLS},
    {"FirstTargetCC", CallingConv::FirstTargetCC},
    {"X86_StdCall", CallingConv::X86_StdCall},
    {"X86_FastCall", CallingConv::X86_FastCall},
    {"ARM_APCS", CallingConv::ARM_APCS},
    {"ARM_AAPCS", CallingConv::ARM_AAPCS},
    {"ARM_AAPCS_VFP", CallingConv::ARM_AAPCS_VFP},
    {"MSP430_INTR", CallingConv::MSP430_INTR},
    {"X86_ThisCall", CallingConv::X86_ThisCall},
    {"PTX_Kernel", CallingConv::PTX_Kernel},
    {"PTX_Device", CallingConv::PTX_Device},
    {"SPIR_FUNC", CallingConv::SPIR_FUNC},
    {"SPIR_KERNEL", CallingConv::SPIR_KERNEL},
    {"Intel_OCL_BI", CallingConv::Intel_OCL_BI},
    {"X86_64_SysV", CallingConv::X86_64_SysV},
    {"Win64", CallingConv::Win64},
    {"X86_VectorCall", CallingConv::X86_VectorCall},
    {"X86_INTR", CallingConv::X86_INTR},
    {"AVR_INTR", CallingConv::AVR_INTR},
    {"AVR_SIGNAL", CallingConv::AVR_SIGNAL},
    {"AVR_BUILTIN", CallingConv::AVR_BUILTIN},
    {"AMDGPU_VS", CallingConv::AMDGPU_VS},
    {"AMDGPU_GS", CallingConv::AMDGPU_GS},
    {"AMDGPU_PS", CallingConv::AMDGPU_PS},
    {"AMDGPU_CS", CallingConv::AMDGPU_CS},
    {"AMDGPU_KERNEL", CallingConv::AMDGPU_KERNEL},
    {"X86_RegCall", CallingConv::X86_RegCall},
    {"AMDGPU_HS", CallingConv::AMDGPU_HS},
    {"MSP430_BUILTIN", CallingConv::MSP430_BUILTIN},
    {"AMDGPU_LS", CallingConv::AMDGPU_LS},
    {"AMDGPU_ES", CallingConv::AMDGPU_ES},
    {"AArch64_VectorCall", CallingConv::AArch64_VectorCall},
    {"AArch64_SVE_VectorCall", CallingConv::AArch64_SVE_VectorCall},
    {"WASM_EmscriptenInvoke", CallingConv::WASM_EmscriptenInvoke},
    {"AMDGPU_Gfx", CallingConv::AMDGPU_Gfx},
    {"M68k_INTR", CallingConv::M68k_INTR},
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
    {"AArch64_SME_ABI_Support_Routines_PreserveMost_From_X0",
     CallingConv::AArch64_SME_ABI_Support_Routines_PreserveMost_From_X0},
    {"AArch64_SME_ABI_Support_Routines_PreserveMost_From_X2",
     CallingConv::AArch64_SME_ABI_Support_Routines_PreserveMost_From_X2},
#endif
};

// For parseFixupCallingConventionPassOptions we check the param against
// an entry in a map. We should consider making this generic if we get similar
// enum map style parsing needs
static Expected<CallingConv::ID> parseFixupCallingConventionPassOptions(
    StringRef Params) {
  CallingConv::ID Result = CallingConv::C;
  while (!Params.empty()) {
    StringRef CCName;
    std::tie(CCName, Params) = Params.split(';');

    auto Itr = CallConvMap.find(CCName.str());
    if (Itr != CallConvMap.end()) {
      Result = Itr->second;
    } else {
      return make_error<StringError>(
          formatv("invalid FixupCallingConventionPass parameter '{0}' ", CCName)
              .str(),
          inconvertibleErrorCode());
    }
  }
  return Result;
}

static Expected<compiler::utils::WorkItemLoopsPassOptions>
parseWorkItemLoopsPassOptions(StringRef Params) {
  compiler::utils::WorkItemLoopsPassOptions Opts;

  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName == "debug") {
      Opts.IsDebug = true;
    } else if (ParamName == "no-tail") {
      Opts.ForceNoTail = true;
    }
  }
  return Opts;
}

static Expected<std::vector<llvm::StringRef>> parseReduceToFunctionPassOptions(
    StringRef Params) {
  std::vector<llvm::StringRef> Names;

  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName.consume_front("names=")) {
      while (!ParamName.empty()) {
        StringRef Val;
        std::tie(Val, ParamName) = ParamName.split(':');
        Names.push_back(Val);
      }
      if (Names.empty()) {
        return make_error<StringError>(
            "ReduceToFunctionPass parameter 'names' must not be empty",
            inconvertibleErrorCode());
      }
    } else {
      return make_error<StringError>(
          formatv("invalid ReduceToFunctionPass parameter '{0}' ", ParamName)
              .str(),
          inconvertibleErrorCode());
    }
  }

  return Names;
}

static Expected<compiler::utils::ReplaceTargetExtTysOptions>
parseReplaceTargetExtTysPassOptions(StringRef Params) {
  compiler::utils::ReplaceTargetExtTysOptions Opts;

  while (!Params.empty()) {
    StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName == "no-images") {
      Opts.ReplaceImages = false;
    } else if (ParamName == "no-samplers") {
      Opts.ReplaceSamplers = false;
    } else if (ParamName == "no-events") {
      Opts.ReplaceEvents = false;
    }
  }

  return Opts;
}

void BaseModulePassMachinery::registerPasses() {
  PassMachinery::registerPasses();
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) \
  getFAM().registerPass([&] { return CREATE_PASS; });

#include "base_module_pass_registry.def"
}

void BaseModulePassMachinery::registerPassCallbacks() {
  PB.registerPipelineParsingCallback(
      [this](StringRef Name, ModulePassManager &PM,
             ArrayRef<PassBuilder::PipelineElement>) {
        // Add a custom callback for pre-defined pipeline components.
        if (Name.consume_front("mux-base")) {
          if (!Name.consume_front("<") || !Name.consume_back(">")) {
            errs() << "'mux-base' must be parameterized\n";
            return false;
          }
          // Construct some default compiler options and a pipeline tuner, since
          // we've not been told otherwise.
          const compiler::Options options;
          const compiler::BasePassPipelineTuner tuner(options);
          if (Name.consume_front("pre-vecz")) {
            compiler::addPreVeczPasses(PM, tuner);
          } else if (Name.consume_front("late-builtins")) {
            compiler::addLateBuiltinsPasses(PM, tuner);
          } else if (Name.consume_front("prepare-wg-sched")) {
            compiler::addPrepareWorkGroupSchedulingPasses(PM);
          } else if (Name.consume_front("wg-sched")) {
            compiler::addPrepareWorkGroupSchedulingPasses(PM);
            const compiler::utils::AddKernelWrapperPassOptions opts;
            PM.addPass(compiler::utils::AddKernelWrapperPass(opts));
          } else {
            errs() << "Unknown mux-base pipeline component '" << Name << "'\n";
            return false;
          }

          // Check there's no trailing stuff we've misidentified
          if (!Name.empty()) {
            errs() << "Unknown mux-base pipeline component '" << Name << "'\n";
            return false;
          }
          return true;
        }

        // Check if any derived classes can handle this pipeline element.
        if (handlePipelineElement(Name, PM)) {
          return true;
        }

#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#define MODULE_PASS_FULL_CREATE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                                   \
    PM.addPass(CREATE_PASS());                          \
    return true;                                        \
  }

#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS)   \
  if (compiler::utils::checkParametrizedPassName(Name, NAME)) {             \
    auto Params = compiler::utils::parsePassParameters(PARSER, Name, NAME); \
    if (!Params) {                                                          \
      errs() << toString(Params.takeError()) << "\n";                       \
      return false;                                                         \
    }                                                                       \
    PM.addPass(CREATE_PASS(Params.get()));                                  \
    return true;                                                            \
  }

#define MODULE_ANALYSIS(NAME, CREATE_PASS)                                  \
  if (Name == "require<" NAME ">") {                                        \
    PM.addPass(                                                             \
        RequireAnalysisPass<std::remove_reference_t<decltype(CREATE_PASS)>, \
                            llvm::Module>());                               \
    return true;                                                            \
  }                                                                         \
  if (Name == "invalidate<" NAME ">") {                                     \
    PM.addPass(InvalidateAnalysisPass<                                      \
               std::remove_reference_t<decltype(CREATE_PASS)>>());          \
    return true;                                                            \
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
  if (compiler::utils::checkParametrizedPassName(Name, NAME)) {               \
    auto Params = compiler::utils::parsePassParameters(PARSER, Name, NAME);   \
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

#include "base_module_pass_registry.def"
        return false;
      });
  TimePasses.registerCallbacks(PIC);
}

void BaseModulePassMachinery::printPassNames(raw_ostream &OS) {
  OS << "Utility passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "base_module_pass_registry.def"

  OS << "Module passes with params:\n";
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  compiler::utils::printPassName(NAME, PARAMS, OS);
#include "base_module_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "base_module_pass_registry.def"

  OS << "Function analyses:\n";
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "base_module_pass_registry.def"

  OS << "Function passes:\n";
#define FUNCTION_PASS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "base_module_pass_registry.def"

  OS << "Function passes with params:\n";
#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  compiler::utils::printPassName(NAME, PARAMS, OS);
#include "base_module_pass_registry.def"

  OS << "CGSCC passes:\n";
#define CGSCC_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "base_module_pass_registry.def"
}

compiler::utils::DeviceInfo initDeviceInfoFromMux(
    mux_device_info_t device_info) {
  if (!device_info) {
    return compiler::utils::DeviceInfo{};
  }

  auto info = compiler::utils::DeviceInfo(
      device_info->half_capabilities, device_info->float_capabilities,
      device_info->double_capabilities, device_info->max_work_width);

  for (size_t i = 0, e = device_info->num_sub_group_sizes; i != e; i++) {
    info.reqd_sub_group_sizes.push_back(device_info->sub_group_sizes[i]);
  }

  return info;
}

}  // namespace compiler
