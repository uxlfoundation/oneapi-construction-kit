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
#include <base/macros.h>
#include <base/mem_to_reg_pass.h>
#include <base/module.h>
#include <base/pass_pipelines.h>
#include <base/printf_replacement_pass.h>
#include <base/program_metadata.h>
#include <base/set_convergent_attr_pass.h>
#include <base/software_division_pass.h>
#include <builtins/bakery.h>
#include <cargo/argument_parser.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Serialization/ASTRecordReader.h>
#include <compiler/limits.h>
#include <compiler/utils/encode_builtin_range_metadata_pass.h>
#include <compiler/utils/llvm_global_mutex.h>
#include <compiler/utils/lower_to_mux_builtins_pass.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_machinery.h>
#include <compiler/utils/replace_atomic_funcs_pass.h>
#include <compiler/utils/replace_c11_atomic_funcs_pass.h>
#include <compiler/utils/replace_target_ext_tys_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/verify_reqd_sub_group_size_pass.h>
#include <llvm-c/BitWriter.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/ForceFunctionAttrs.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/BDCE.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/EntryExitInstrumenter.h>
#include <llvm/Transforms/Vectorize/LoopVectorize.h>
#include <llvm/Transforms/Vectorize/SLPVectorizer.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/triple.h>
#include <mux/mux.hpp>
#include <spirv-ll/module.h>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_set>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

namespace {

// LLVM's pipeline hooks are broken until version 12. There's no way to
// access early pipeline hooks in O0 mode. Thus, we build our pipelines
// manually in older versions. For the newer versions, a few parameters got
// changed around; not a big deal, just a few ugly ifdefs
inline llvm::ModulePassManager buildPerModuleDefaultPipeline(
    llvm::PassBuilder &PB, llvm::OptimizationLevel OL,
    std::optional<llvm::ModulePassManager> EP) {
  assert(OL != llvm::OptimizationLevel::O0);
  if (EP.has_value()) {
    PB.registerPipelineStartEPCallback(
        [&EP](llvm::ModulePassManager &MPM, llvm::OptimizationLevel) {
          MPM.addPass(std::move(EP.value()));
        });
  }
  PB.registerPipelineStartEPCallback(
      [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel) {
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(
            llvm::EntryExitInstrumenterPass(/*PostInlining=*/false)));
      });
  return PB.buildPerModuleDefaultPipeline(OL);
}

inline llvm::ModulePassManager buildO0DefaultPipeline(
    llvm::PassBuilder &PB, std::optional<llvm::ModulePassManager> EP) {
  if (EP.has_value()) {
    PB.registerPipelineStartEPCallback(
        [&EP](llvm::ModulePassManager &MPM, llvm::OptimizationLevel) {
          MPM.addPass(std::move(EP.value()));
        });
  }
  return PB.buildO0DefaultPipeline(llvm::OptimizationLevel::O0);
}

inline llvm::ModulePassManager buildPipeline(
    llvm::PassBuilder &PB, clang::CodeGenOptions Opts,
    std::optional<llvm::ModulePassManager> EP) {
  return Opts.OptimizationLevel == 0
             ? buildO0DefaultPipeline(PB, std::move(EP))
             : buildPerModuleDefaultPipeline(PB, llvm::OptimizationLevel::O3,
                                             std::move(EP));
}

// Helper function for running the llvm passes we skip during codegen.
//
// We skip these passes at first because they optimize out integer divide by
// zero operations as if they were undefined behaviour, whereas for OpenCL C
// these cases should result in an unspecified value. The way these passes are
// run by clang varies between llvm versions, hence this generic helper. On
// versions less than 12 we use the old legacy pass manager and on versions
// greater than 12 we use the new(ish) optimization pipelines. These two paths
// map to the `EmitAssembly` and `EmitAssemblyWithNewPassManager` functions in
// `clang/lib/CodeGen/BackendUtil.cpp` respectively.
void runFrontendPipeline(
    compiler::BaseModule &base_module, llvm::Module &module,
    const clang::CodeGenOptions &CGO,
    std::optional<llvm::ModulePassManager> EP = std::nullopt,
    std::optional<llvm::ModulePassManager> LP = std::nullopt) {
  llvm::PipelineTuningOptions PTO;
  PTO.LoopUnrolling = CGO.UnrollLoops;
  auto PassMach = base_module.createPassMachinery();

  base_module.initializePassMachineryForFrontend(*PassMach, CGO);

  llvm::ModulePassManager MPM =
      buildPipeline(PassMach->getPB(), CGO, std::move(EP));

  if (LP.has_value()) {
    MPM.addPass(std::move(LP.value()));
  }
  MPM.run(module, PassMach->getMAM());
}

class DeserializeMemoryBuffer final : public llvm::MemoryBuffer {
 public:
  DeserializeMemoryBuffer(llvm::StringRef buffer) {
    init(buffer.begin(), buffer.end(), false);
  }

  virtual llvm::MemoryBuffer::BufferKind getBufferKind() const {
    return llvm::MemoryBuffer::MemoryBuffer_Malloc;
  }
};

class BakedMemoryBuffer : public llvm::MemoryBuffer {
 public:
  BakedMemoryBuffer(const void *const buffer, const uint32_t size) {
    init(static_cast<const char *>(buffer),
         static_cast<const char *>(buffer) + size, true);
  }

  virtual llvm::MemoryBuffer::BufferKind getBufferKind() const {
    return llvm::MemoryBuffer::MemoryBuffer_Malloc;
  }

  virtual ~BakedMemoryBuffer() {}
};

bool isDeviceProfileFull(cargo::string_view profile_string) {
  return profile_string.compare("FULL_PROFILE");
}

/// @brief Load the kernel builtins header as a virtual Clang file.
///
/// PCH files are not independent from the header source they were created
/// from, and as a result we need to load our embedded builtins.h into Clang
/// so that we can compile kernels successfully without having this source on
/// disk.
///
/// Relocatable PCH files do also exist, but still require the header source,
/// only at a location discoverable at runtime. However this complicates
/// development and shipping of our library so instead we just embed the
/// builtins.h which our PCH was derived from.
///
/// @param[in]    compiler - Compiler to use to load the file.
/// @param[in]    moduleFile - Module created from the PCH file.
///
/// @return Return true if the kernel builtins header was loaded, false
/// otherwise.
static bool loadKernelAPIHeader(clang::CompilerInstance &compiler,
                                clang::serialization::ModuleFile *moduleFile) {
  if (!moduleFile || moduleFile->InputFilesLoaded.size() != 1) {
    return false;
  }

  // Jump to the place where the information about the builtins header is
  // stored inside the PCH file.
  llvm::BitstreamCursor &Cursor = moduleFile->InputFilesCursor;
  const clang::SavedStreamPosition SavedPosition(Cursor);
#if LLVM_VERSION_GREATER_EQUAL(18, 0)
  // LLVM 18 introduces a new offset that should be included
  const uint64_t Base = moduleFile->InputFilesOffsetBase;
#else
  const uint64_t Base = 0;
#endif
  if (Cursor.JumpToBit(Base + moduleFile->InputFileOffsets[0])) {
    return false;
  }

  // Read the file input record for that header. We need to know its size and
  // last modified time in order to pass the PCH validation checks.  LLVM's
  // BitstreamReader.h triggers an undefined shift warning in clang-tidy, we
  // don't have control of this header and unfortunately setting NOLINT neither
  // here nor on the include line can convince clang-tidy-8 to suppress the
  // warning.  However, this assert is guaranteed to be correct due to the
  // above `Cursor.JumpToBit` call, and provides enough information for
  // clang-tidy to understand that the undefined shift is impossible.
  assert((Cursor.GetCurrentBitNo() == Base + moduleFile->InputFileOffsets[0]) &&
         "Clang bitstream reader is in invalid state.");
  clang::ASTReader::RecordData Record;
  llvm::StringRef Filename;
  auto ExpectCode = Cursor.ReadCode();
  if (ExpectCode.takeError()) {
    return false;
  }
  const unsigned Code = *ExpectCode;
  auto ExpectResult = Cursor.readRecord(Code, Record, &Filename);
  if (ExpectResult.takeError()) {
    return false;
  }
  const unsigned Result = *ExpectResult;
  if (static_cast<clang::serialization::InputFileRecordTypes>(Result) !=
      clang::serialization::INPUT_FILE) {
    return false;
  }
  const off_t StoredSize = static_cast<off_t>(Record[1]);
  const time_t StoredTime = static_cast<time_t>(Record[2]);

  // Retrieve the builtins header and checks that the size matches.
  auto header = builtins::get_api_src_file();
  if (StoredSize != static_cast<off_t>(header.size())) {
    return false;
  }

  // Create a virtual 'in-memory' file for the header, with the hardcoded path.
  clang::FileManager &fileManager = compiler.getFileManager();
  const clang::FileEntryRef entry =
      fileManager.getVirtualFileRef(Filename, StoredSize, StoredTime);
  if (!entry) {
    return false;
  }

  // Create a buffer that will hold the content of that file.
  std::unique_ptr<llvm::MemoryBuffer> header_buffer{
      new BakedMemoryBuffer(header.data(), header.size())};
  clang::SourceManager &sourceManager = compiler.getSourceManager();
  sourceManager.overrideFileContents(entry, std::move(header_buffer));

  // Finally, let Clang know we have loaded this file already.
  moduleFile->InputFilesLoaded[0] =
      clang::serialization::InputFile(entry, false, false);
  return true;
}

bool hasRecursiveKernels(llvm::Module *module) {
  auto callgraph = llvm::CallGraph(*module);
  std::unordered_map<const llvm::Function *,
                     std::unordered_set<const llvm::Function *>>
      callDep;
  for (auto callerIt = callgraph.begin(); callerIt != callgraph.end();
       ++callerIt) {
    if (callerIt->first) {
      const llvm::Function *caller = callerIt->first;
      for (auto calleeIt = callerIt->second->begin();
           calleeIt != callerIt->second->end(); ++calleeIt) {
        llvm::Function *callee = calleeIt->second->getFunction();
        // We have recursion if we call the same function we are executing, or
        // if the function we call already calls us.
        if ((callee == caller) || callDep[callee].count(caller)) {
          return true;
        }
        callDep[caller].insert(callee);
      }
    }
  }
  return false;
}

// LLVM 12 replaced the dedicated OpenCLOptions class with an
// llvm::StringMap<bool>
inline void supportOpenCLOpt(clang::CompilerInstance &instance,
                             const std::string opt) {
  instance.getTarget().getSupportedOpenCLOpts().insert({opt, true});
}
}  // namespace

namespace compiler {
BaseModule::BaseModule(compiler::BaseTarget &target,
                       compiler::BaseContext &context, uint32_t &num_errors,
                       std::string &log)
    : target(target),
      context(context),
      state(ModuleState::NONE),
      num_errors(num_errors),
      log(log) {}

BaseModule::~BaseModule() {}

void BaseModule::clear() {
  llvm_module.reset();
  kernel_map.clear();

  state = ModuleState::NONE;
}

Options &BaseModule::getOptions() { return options; }

const Options &BaseModule::getOptions() const { return options; }

Result BaseModule::parseOptions(cargo::string_view input_options,
                                compiler::Options::Mode mode) {
  const compiler::Info *compiler_info = target.getCompilerInfo();

  // Select the appropriate error code.
  const Result invalid_options = [](const compiler::Options::Mode mode) {
    switch (mode) {
      case compiler::Options::Mode::BUILD:
        return Result::INVALID_BUILD_OPTIONS;
      case compiler::Options::Mode::COMPILE:
        return Result::INVALID_COMPILER_OPTIONS;
      case compiler::Options::Mode::LINK:
        return Result::INVALID_LINKER_OPTIONS;
    }
    return Result::SUCCESS;
  }(mode);
  cargo::argument_parser<32> parser;

  // TODO: If/when we have cargo::vector_view, we can set the include
  // directories and definitions directly in options.
  cargo::small_vector<cargo::string_view, 4> includes;
  cargo::small_vector<cargo::string_view, 4> definitions;
  cargo::small_vector<std::pair<cargo::string_view, cargo::string_view>, 4>
      device_custom_options;

  bool create_library = false;
  bool enable_link_options = false;

  // -cl-strict-aliasing is deprecated in OpenCL 1.1, so accept the argument,
  // but do nothing with the result (i.e. do not record it in options).
  bool cl_strict_aliasing = false;
  const cargo::string_view spir_std;
  const cargo::string_view x;

  cargo::string_view cl_std;
  cargo::string_view cl_vec;
  cargo::string_view cl_wfv;
  const cargo::string_view cl_dma;
  cargo::string_view source;

  if (parser.add_argument({"-create-library", create_library})) {
    return Result::OUT_OF_MEMORY;
  }
  if (parser.add_argument({"-enable-link-options", enable_link_options})) {
    return Result::OUT_OF_MEMORY;
  }

  if (parser.add_argument(
          {"-cl-denorms-are-zero", options.denorms_may_be_zero})) {
    return Result::OUT_OF_MEMORY;
  }
  if (parser.add_argument({"-cl-no-signed-zeros", options.no_signed_zeros})) {
    return Result::OUT_OF_MEMORY;
  }
  if (parser.add_argument({"-cl-unsafe-math-optimizations",
                           options.unsafe_math_optimizations})) {
    return Result::OUT_OF_MEMORY;
  }
  if (parser.add_argument({"-cl-finite-math-only", options.finite_math_only})) {
    return Result::OUT_OF_MEMORY;
  }
  if (parser.add_argument({"-cl-fast-relaxed-math", options.fast_math})) {
    return Result::OUT_OF_MEMORY;
  }

  cargo::result parse_result;

  if (compiler::Options::Mode::LINK == mode) {
    // Parse the link options.
    parse_result = parser.parse_args(input_options);
  } else {
    // Add additional compile/build options.
    if (parser.add_argument({"-I", includes})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-D", definitions})) {
      return Result::OUT_OF_MEMORY;
    }

    if (parser.add_argument({"-Werror", options.warn_error})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-w", options.warn_ignore})) {
      return Result::OUT_OF_MEMORY;
    }

    if (parser.add_argument({"-cl-fp32-correctly-rounded-divide-sqrt",
                             options.fp32_correctly_rounded_divide_sqrt})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-cl-kernel-arg-info", options.kernel_arg_info})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-cl-mad-enable", options.mad_enable})) {
      return Result::OUT_OF_MEMORY;
    }

    if (parser.add_argument({"-cl-opt-disable", options.opt_disable,
                             cargo::argument::STORE_TRUE})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-cl-opt-enable", options.opt_disable,
                             cargo::argument::STORE_FALSE})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-cl-single-precision-constant",
                             options.single_precision_constant})) {
      return Result::OUT_OF_MEMORY;
    }
    std::array<cargo::string_view, 3> cl_std_choices = {
        {"CL1.1", "CL1.2", "CL3.0"}};
    if (parser.add_argument({"-cl-std=", cl_std_choices, cl_std})) {
      return Result::OUT_OF_MEMORY;
    }
    if (parser.add_argument({"-cl-strict-aliasing", cl_strict_aliasing})) {
      return Result::OUT_OF_MEMORY;
    }

    // OCL_EXTENSION_cl_codeplay_soft_math
    // TODO: This should be called -cl-codeplay-soft-math
    if (parser.add_argument({"-codeplay-soft-math", options.soft_math})) {
      return Result::OUT_OF_MEMORY;
    }

    // Enables the cl_codeplay_kernel_debug extension
    if (target.getCompilerInfo()->kernel_debug) {
      if (parser.add_argument({"-g", options.debug_info})) {
        return Result::OUT_OF_MEMORY;
      }
      if (parser.add_argument({"-S", source})) {
        return Result::OUT_OF_MEMORY;
      }
    }

    if (parser.add_argument({"-cl-llvm-stats", options.llvm_stats})) {
      return Result::OUT_OF_MEMORY;
    }

    std::array<cargo::string_view, 4> cl_vec_choices = {
        {"none", "loop", "slp", "all"}};
    if (parser.add_argument({"-cl-vec=", cl_vec_choices, cl_vec})) {
      return Result::OUT_OF_MEMORY;
    }

    std::array<cargo::string_view, 3> cl_wfv_choices = {
        {"never", "always", "auto"}};
    if (parser.add_argument({"-cl-wfv=", cl_wfv_choices, cl_wfv})) {
      return Result::OUT_OF_MEMORY;
    }

    const auto precache_wgs_choices_parser = [this](
                                                 cargo::string_view choices) {
      const auto size_strings = cargo::split(choices, ":");
      for (const auto &size_string : size_strings) {
        std::array<size_t, 3> wgs = {1, 1, 1};
        const auto wgs_dims = cargo::split(size_string, ",");
        if (wgs_dims.size() > 3) {
          return cargo::argument::parse::INVALID;
        }
        for (size_t i = 0; i < wgs_dims.size(); i++) {
          std::string wgs_dim_string(wgs_dims[i].begin(), wgs_dims[i].end());
          char *endptr;
          const int64_t local_size =
              std::strtol(wgs_dim_string.data(), &endptr, 10);
          // Fail if we got a dodgy value or a non-number character.
          if (endptr != wgs_dim_string.data() + wgs_dim_string.size() ||
              local_size < 1) {
            return cargo::argument::parse::INVALID;
          }
          wgs[i] = local_size;
        }
        this->options.precache_local_sizes.push_back(wgs);
      }
      return cargo::argument::parse::COMPLETE;
    };

    if (parser.add_argument({"-cl-precache-local-sizes=",
                             [](cargo::string_view) {
                               return cargo::argument::parse::INCOMPLETE;
                             },
                             precache_wgs_choices_parser})) {
      return Result::OUT_OF_MEMORY;
    }

    // Device argument name handler
    const auto name_parser = [&device_custom_options](
                                 cargo::string_view argument,
                                 bool value_expected,
                                 cargo::string_view arg_name) {
      if (!value_expected && (argument != arg_name)) {
        // Flags must match exactly rather than being a substring
        return cargo::argument::parse::INVALID;
      }

      if (device_custom_options.emplace_back(std::make_pair(arg_name, ""))) {
        return cargo::argument::parse::INVALID;
      }
      return value_expected ? cargo::argument::parse::INCOMPLETE
                            : cargo::argument::parse::COMPLETE;
    };

    // Value handler when device option doesn't take a value
    const auto empty_value_parser = [](cargo::string_view) {
      return cargo::argument::parse::NOT_FOUND;
    };

    // Value handler when device option does take a value
    const auto set_value_parser =
        [&device_custom_options](cargo::string_view arg_value) {
          auto &pair = device_custom_options.back();
          if (arg_value.starts_with("-")) {
            // Value for option shouldn't start with '-', suggests we've
            // started parsing the next argument.
            return cargo::argument::parse::INVALID;
          }
          pair.second = arg_value;
          return cargo::argument::parse::COMPLETE;
        };

    auto split_options = cargo::split(compiler_info->compilation_options, ";");
    for (auto option : split_options) {
      const auto tuple = cargo::split_all(option, ",");

      // Sanity check options reported by device are valid
      assert(tuple.size() == 3 &&
             "Device compilation options does not conform to core spec");
      assert(cargo::string_view::npos == tuple[2].find_first_of("\t\n\v\f\r") &&
             "Device compilation options does not conform to core spec");
      assert((tuple[1] == "1" || tuple[1] == "0") &&
             "Device compilation options does not conform to core spec");
      assert(cargo::string_view::npos ==
                 tuple[0].find_first_of(" \t\n\v\f\r") &&
             "Device compilation options does not conform to core spec");

      const cargo::string_view name = tuple[0];
      const bool takes_value = (tuple[1][0] == '1');

      cargo::argument::custom_handler_function bound_name_parser =
          std::bind(name_parser, std::placeholders::_1, takes_value, name);
      cargo::argument::custom_handler_function value_parser =
          takes_value
              ? cargo::argument::custom_handler_function(set_value_parser)
              : cargo::argument::custom_handler_function(empty_value_parser);

      if (parser.add_argument(
              {name, std::move(bound_name_parser), std::move(value_parser)})) {
        return Result::OUT_OF_MEMORY;
      }
    }

    // Parsing the compile/build options must be called within this scope
    // because the lifetime of object storage for arguments with choices does
    // not extend into the outer scope.
    parse_result = parser.parse_args(input_options);
  }

  switch (parse_result) {
    case cargo::success:
      break;
    case cargo::bad_argument:
      return invalid_options;
    default:
      return Result::OUT_OF_MEMORY;
  }

  // -enable-link-options is only valid with -create-library.
  if (enable_link_options && !create_library) {
    return invalid_options;
  }

  // individual options are not set when creating a library.
  if (create_library &&
      (options.denorms_may_be_zero || options.no_signed_zeros ||
       options.unsafe_math_optimizations || options.finite_math_only ||
       options.fast_math)) {
    return invalid_options;
  }

  // TODO: If/when we have cargo::vector_view, we can set the include
  // directories and definitions directly in options.
  for (auto include : includes) {
    options.include_dirs.emplace_back(include.data(), include.size());
  }
  for (auto definition : definitions) {
    options.definitions.emplace_back(definition.data(), definition.size());
  }

  for (auto itr = device_custom_options.begin(),
            end = device_custom_options.end();
       itr != end; itr++) {
    const cargo::string_view name = itr->first;
    const cargo::string_view value = itr->second;

    options.device_args.append(name.data(), name.size());
    options.device_args.push_back(',');
    options.device_args.append(value.data(), value.size());

    // Don't add trailing ';' to last element
    if (itr != (end - 1)) {
      options.device_args.push_back(';');
    }
  }

  if ("none" == cl_vec) {
    options.prevec_mode = compiler::PreVectorizationMode::NONE;
  } else if ("loop" == cl_vec) {
    options.prevec_mode = compiler::PreVectorizationMode::LOOP;
  } else if ("slp" == cl_vec) {
    options.prevec_mode = compiler::PreVectorizationMode::SLP;
  } else if ("all" == cl_vec) {
    options.prevec_mode = compiler::PreVectorizationMode::ALL;
  }

  if ("always" == cl_wfv) {
    if (!compiler_info->vectorizable) {
      addDiagnostic(
          "Ignoring -cl-wfv=always option: Device does not support "
          "vectorization.");
    } else {
      options.vectorization_mode = compiler::VectorizationMode::ALWAYS;
    }
  } else if ("auto" == cl_wfv) {
    if (compiler_info->vectorizable) {
      options.vectorization_mode = compiler::VectorizationMode::AUTO;
    }
  } else if ("never" == cl_wfv) {
    options.vectorization_mode = compiler::VectorizationMode::NEVER;
  }

  if (!source.empty()) {
    options.source_file.assign(source.data(), source.size());
  }

  options.standard =
      llvm::StringSwitch<Standard>(cargo::as<llvm::StringRef>(cl_std))
          .Case("CL1.1", Standard::OpenCLC11)
          .Case("CL1.2", Standard::OpenCLC12)
          .Case("CL3.0", Standard::OpenCLC30)
          .Default(options.standard);

  if (options.fast_math) {
    // -cl-fast-relaxed-math implicitly sets -cl-finite-math-only &
    // -cl-unsafe-math-optimizations
    options.finite_math_only = true;
    options.unsafe_math_optimizations = true;
  }

  if (options.unsafe_math_optimizations) {
    // -cl-unsafe-math-optimizations implicitly sets cl-no-signed-zeros and
    // -cl-mad-enable (perhaps via -cl-fast-relaxed-math).
    options.no_signed_zeros = true;
    options.mad_enable = true;
  }

  // TODO: CA-669 Change when we add support for this flag
  if (options.fp32_correctly_rounded_divide_sqrt) {
    addBuildError(
        "Error compiling -cl-fp32-correctly-rounded-divide-sqrt not supported "
        "on device.");
    return invalid_options;
  }
  return Result::SUCCESS;
}

class StripFastMathAttrs final
    : public llvm::PassInfoMixin<StripFastMathAttrs> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {
    auto version = compiler::utils::getOpenCLVersion(*F.getParent());
    // This is only required for compatibility with OpenCL 3.0 semantics.
    if (version < compiler::utils::OpenCLC30) {
      return llvm::PreservedAnalyses::all();
    }
    bool Changed = false;
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (llvm::isa<llvm::FPMathOperator>(&I) &&
            I.getOpcode() == llvm::Instruction::FDiv) {
          I.setFast(false);
          Changed = true;
        }
      }
    }
    return Changed ? llvm::PreservedAnalyses::none()
                   : llvm::PreservedAnalyses::all();
  }

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

llvm::ModulePassManager BaseModule::getEarlyOpenCLCPasses() {
  // Run the software division pass required for OpenCL C.
  llvm::ModulePassManager pm;
  pm.addPass(llvm::createModuleToFunctionPassAdaptor(SoftwareDivisionPass()));
  pm.addPass(llvm::createModuleToFunctionPassAdaptor(StripFastMathAttrs()));
  pm.addPass(compiler::SetConvergentAttrPass());
  return pm;
}

llvm::ModulePassManager BaseModule::getEarlySPIRVPasses() {
  // Run the various fixup passes needed to make sure the IR we've got is spec
  // conformant.
  llvm::ModulePassManager pm;
  // Set the opencl.ocl.version metadata if not already set. In SPIR-V this is
  // not set (by spirv-ll) and conveys the best-matching version of OpenCL C
  // for which we translate SPIR-V binaries. This covers not just how we
  // translate ops from the OpenCL Extended Instruction Set, but also for core
  // concepts like the generic address space and sub-group ops.
  pm.addPass(compiler::utils::SimpleCallbackPass([](llvm::Module &m) {
    if (!m.getNamedMetadata("opencl.ocl.version")) {
      auto *const ocl_ver = m.getOrInsertNamedMetadata("opencl.ocl.version");
      const unsigned major = 3;
      const unsigned minor = 0;
      llvm::Metadata *values[2] = {
          llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(m.getContext()), major)),
          llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(m.getContext()), minor))};

      ocl_ver->addOperand(llvm::MDTuple::get(m.getContext(), values));
    }
  }));
  {
    // The BitShiftFixupPass and SoftwareDivisionPass manually fix cases
    // which in C would be UB but which the CL spec has different rules for.
    llvm::FunctionPassManager fpm;
    fpm.addPass(compiler::BitShiftFixupPass());
    fpm.addPass(compiler::SoftwareDivisionPass());
    pm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));
  }
  // The SetConvergentAttrPass sets the convergent attribute on all barrier and
  // other functions to ensure that LLVM optimizers do not illegally change
  // their use.
  pm.addPass(compiler::SetConvergentAttrPass());
  return pm;
}

cargo::expected<spirv::ModuleInfo, Result> BaseModule::compileSPIRV(
    cargo::array_view<const std::uint32_t> buffer,
    const spirv::DeviceInfo &spirv_device_info,
    cargo::optional<const spirv::SpecializationInfo &> spirv_spec_info) {
  const std::lock_guard<compiler::BaseContext> lock(context);

  spirv::ModuleInfo module_info;

  {
    spirv_ll::Context spvContext(&target.getLLVMContext());

    // Convert SPIR-V inputs to SPIRV-LL data structures.
    spirv_ll::DeviceInfo spirv_ll_device_info;
    std::copy(spirv_device_info.capabilities.begin(),
              spirv_device_info.capabilities.end(),
              std::back_inserter(spirv_ll_device_info.capabilities));
    std::copy(spirv_device_info.extensions.begin(),
              spirv_device_info.extensions.end(),
              std::back_inserter(spirv_ll_device_info.extensions));
    std::copy(spirv_device_info.ext_inst_imports.begin(),
              spirv_device_info.ext_inst_imports.end(),
              std::back_inserter(spirv_ll_device_info.extInstImports));
    spirv_ll_device_info.addressingModel = spirv_device_info.addressing_model;
    spirv_ll_device_info.addressBits = spirv_device_info.address_bits;

    spirv_ll::SpecializationInfo spirv_ll_spec_info;
    cargo::optional<const spirv_ll::SpecializationInfo &>
        spirv_ll_spec_info_optional;
    if (spirv_spec_info) {
      spirv_ll_spec_info.data = spirv_spec_info->data;
      for (const auto &entry : spirv_spec_info->entries) {
        spirv_ll_spec_info.entries[entry.first] =
            spirv_ll::SpecializationInfo::Entry{entry.second.offset,
                                                entry.second.size};
      }
      spirv_ll_spec_info_optional = spirv_ll_spec_info;
    }

    // Translate the SPIR-V binary into an llvm::Module.
    auto spvModule =
        spvContext.translate({buffer.data(), buffer.size()},
                             spirv_ll_device_info, spirv_ll_spec_info_optional);
    if (!spvModule) {
      // Add error message to the build log.
      log.append(spvModule.error().message + "\n");
      num_errors = 1;
      return cargo::make_unexpected(Result::COMPILE_PROGRAM_FAILURE);
    }

    // Fill the SPIR-V module info data structure.
    for (const auto &db : spvModule->getUsedDescriptorBindings()) {
      module_info.used_descriptor_bindings.push_back({db.set, db.binding});
    }
    module_info.workgroup_size = spvModule->getWGS();

    // Transfer ownership of the llvm::Module.
    llvm_module = std::move(spvModule.value().llvmModule);
  }

  createOpenCLKernelsMetadata(*llvm_module);

  // Now run a generic optimization pipeline based on the one clang normally
  // runs during codegen.
  // We also run some of the fixup passes on IR generated from SPIR-V and it's
  // unclear if that's actually necessary: see DDK-278.
  clang::CodeGenOptions codeGenOpts;
  populateCodeGenOpts(codeGenOpts);
  runOpenCLFrontendPipeline(codeGenOpts, getEarlySPIRVPasses());

  state = ModuleState::COMPILED_OBJECT;

  return {std::move(module_info)};
}

void BaseModule::populateCodeGenOpts(clang::CodeGenOptions &codeGenOpts) const {
  codeGenOpts.OptimizationLevel = options.opt_disable ? 0 : 3;
  codeGenOpts.StackRealignment = true;
  codeGenOpts.SimplifyLibCalls = false;
  // Clang sets this by default when compiling OpenCL C.
  codeGenOpts.EnableNoundefAttrs = true;

  codeGenOpts.VectorizeSLP =
      options.prevec_mode == compiler::PreVectorizationMode::SLP ||
      options.prevec_mode == compiler::PreVectorizationMode::ALL;
  codeGenOpts.VectorizeLoop =
      options.prevec_mode == compiler::PreVectorizationMode::LOOP ||
      options.prevec_mode == compiler::PreVectorizationMode::ALL;

  codeGenOpts.LessPreciseFPMAD = options.mad_enable;
  if (options.denorms_may_be_zero) {
    // LLVM 11 changes controls of denormal modes handling in the attempt to
    // unify backend behaviours. Denormal mode should now be set by the Driver.
    // Some backend rely on a separate option for 32-bit FP only.
    // Here we set it for consistency. Not sure if we really use it later on.
    codeGenOpts.FP32DenormalMode = llvm::DenormalMode::getPositiveZero();
    codeGenOpts.FPDenormalMode = llvm::DenormalMode::getPositiveZero();
  }
  // Currently this will always be true as we don't report support for the
  // flag, and have not implemented the required sqrt builtin.  See CA-669.
  codeGenOpts.OpenCLCorrectlyRoundedDivSqrt =
      options.fp32_correctly_rounded_divide_sqrt;

  codeGenOpts.EmitOpenCLArgMetadata = options.kernel_arg_info;
  if (options.debug_info) {
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
    codeGenOpts.setDebugInfo(llvm::codegenoptions::FullDebugInfo);
#else
    codeGenOpts.setDebugInfo(clang::codegenoptions::FullDebugInfo);
#endif
  }
#if LLVM_VERSION_LESS(17, 0)
  codeGenOpts.OpaquePointers = true;
#endif
}

void BaseModule::addDefaultOpenCLPreprocessorOpts(
    cargo::string_view device_profile, MacroDefVec &macro_defs,
    OpenCLOptVec &opencl_opts) const {
  mux_device_info_t device_info = target.getCompilerInfo()->device_info;

  // Clang seems to define this by default.
  if (!device_info->double_capabilities) {
    addMacroUndef("__opencl_c_fp64", macro_defs);
  }
  if (device_info->image_support) {
    addMacroDef("__opencl_c_images=1", macro_defs);
  }
  if (options.fast_math) {
    addMacroDef("__FAST_RELAXED_MATH__=1", macro_defs);
  }
  if (device_info->endianness == mux_endianness_little) {
    addMacroDef("__ENDIAN_LITTLE__=1", macro_defs);
  }

  // If supported add cles extensions which aren't covered by clang.
  const bool deviceHasInt64Support =
      device_info->integer_capabilities & mux_integer_capabilities_64bit;
  if (!isDeviceProfileFull(device_profile)) {
    if (device_info->image2d_array_writes) {
      addMacroDef("cles_khr_2d_image_array_writes=1", macro_defs);
    }

    if (deviceHasInt64Support) {
      addMacroDef("cles_khr_int64=1", macro_defs);
    }
  }

  if (deviceHasInt64Support) {
    addMacroDef("__opencl_c_int64=1", macro_defs);
  }

  for (const auto &definition : options.definitions) {
    addMacroDef(definition, macro_defs);
  }

  // Although most option macros are not defined in the builtins library and
  // are dynamically defined here, it is the other way around for the image
  // support because all the builtin functions etc are already covered by
  // defines set by the build system.
  if (!device_info->image_support) {
    addMacroUndef("__IMAGE_SUPPORT__", macro_defs);
    addMacroUndef("__opencl_c_images", macro_defs);
    addMacroUndef("__opencl_c_3d_image_writes", macro_defs);
  }

  // Clang blindly sets the highest CL_VERSION that it supports (2.0), so
  // remove that macro.
  addMacroUndef("CL_VERSION_2_0", macro_defs);

  // Add defines for runtime extensions
  for (const auto &extension : options.runtime_extensions) {
    addMacroDef(extension, macro_defs);
  }

  // Enable compiler extensions and add defines
  for (const auto &extension : options.compiler_extensions) {
    addOpenCLOpt(extension, opencl_opts);
    addMacroDef(extension, macro_defs);
  }

  // Disable half types unless supported by the device
  if (!device_info->half_capabilities) {
    addOpenCLOpt("-cl_khr_fp16", opencl_opts);
    addMacroUndef("cl_khr_fp16", macro_defs);
  }

  // Disable `cl_khr_int64_base_atomics` and `cl_khr_int64_extended_atomics`
  // unless supported by the device.
  if (!(device_info->atomic_capabilities & mux_atomic_capabilities_64bit)) {
    addOpenCLOpt("-cl_khr_int64_base_atomics", opencl_opts);
    addMacroUndef("cl_khr_int64_base_atomics", macro_defs);
    addOpenCLOpt("-cl_khr_int64_extended_atomics", opencl_opts);
    addMacroUndef("cl_khr_int64_extended_atomics", macro_defs);
  }

  if (options.standard == Standard::OpenCLC30) {
    // work-group collective functions are an optional feature in OpenCL 3.0.
    if (device_info->supports_work_group_collectives) {
      addMacroDef("__opencl_c_work_group_collective_functions", macro_defs);
    }
    if (device_info->max_sub_group_count) {
      addMacroDef("__opencl_c_subgroups", macro_defs);
    }
  }

  // Clang appears to unconditionally define the following macros, even though
  // we might not support the features.

  // generic address space functions are an optional feature in OpenCL 3.0
  if (options.standard != Standard::OpenCLC30 ||
      !device_info->supports_generic_address_space) {
    addMacroUndef("__opencl_c_generic_address_space", macro_defs);
  }

  addMacroUndef("__opencl_c_program_scope_global_variables", macro_defs);
  addMacroUndef("__opencl_c_atomic_order_acq_rel", macro_defs);
  addMacroUndef("__opencl_c_atomic_order_seq_cst", macro_defs);
  addMacroUndef("__opencl_c_device_enqueue", macro_defs);
  addMacroUndef("__opencl_c_pipes", macro_defs);
  addMacroUndef("__opencl_c_read_write_images", macro_defs);
}

clang::LangStandard::Kind BaseModule::setClangOpenCLStandard(
    clang::LangOptions &lang_opts) const {
  switch (options.standard) {
    case Standard::OpenCLC11:
      lang_opts.OpenCLVersion = 110;
      return clang::LangStandard::lang_opencl11;
    case Standard::OpenCLC12:
      lang_opts.OpenCLVersion = 120;
      return clang::LangStandard::lang_opencl12;
    case Standard::OpenCLC30:
      lang_opts.OpenCLVersion = 300;
      return clang::LangStandard::lang_opencl30;
    default:
      llvm_unreachable("clang language standard not initialised");
  }
}

void BaseModule::setDefaultOpenCLLangOpts(clang::LangOptions &lang_opts) const {
  mux_device_info_t device_info = target.getCompilerInfo()->device_info;

  // Set Clang Language Options.
  lang_opts.RTTI = false;
  lang_opts.RTTIData = false;
  lang_opts.MathErrno = false;
  lang_opts.Optimize = !options.opt_disable;
  lang_opts.NoBuiltin = true;
  lang_opts.ModulesSearchAll = false;
  // Before llvm10, clang assumes OpenCL functions are always convergent. After
  // e531750c6cf9a it's a compiler option, which defaults to true.
  // Assuming that all function are convergent is unnecessarily conservative
  // and we already fixup those that *should be* convergent in our
  // implementation in `compiler::SetConvergentAttrPass`, so we can disable
  // this pessimization
  lang_opts.ConvergentFunctions = false;

  if (device_info->double_capabilities == 0) {
    lang_opts.SinglePrecisionConstants = true;
  } else {
    lang_opts.SinglePrecisionConstants = options.single_precision_constant;
  }
  if (options.fast_math) {
    lang_opts.FastRelaxedMath = options.fast_math;
  }

  // The default fast-math flags for the IR builder are now derived from
  // properties in LangOptions, which in prior to LLVM 11 versions were
  // declared inside CodeGenOptions, therefore we need to define Clang's
  // LangOptions for FP optmization.
  lang_opts.NoHonorInfs = options.finite_math_only;
  lang_opts.NoHonorNaNs = options.finite_math_only;
  lang_opts.NoSignedZero = options.no_signed_zeros;
  lang_opts.UnsafeFPMath = options.unsafe_math_optimizations;
  lang_opts.AllowFPReassoc =
      options.unsafe_math_optimizations;  // Spec does not mandate this.
  lang_opts.AllowRecip =
      options.unsafe_math_optimizations;  // Spec does not mandate this.

  // Override the C99 inline semantics to accommodate for more OpenCL C
  // programs in the wild.
  lang_opts.GNUInline = true;

  lang_opts.OpenCLGenericAddressSpace =
      (options.standard == Standard::OpenCLC30) &&
      device_info->supports_generic_address_space;
}

std::string BaseModule::debugDumpKernelSource(
    llvm::StringRef source, llvm::ArrayRef<std::string> definitions) {
  std::string dbg_filename;

#ifndef CA_ENABLE_DEBUG_SUPPORT
  (void)source;
  (void)definitions;
#else
  // Print the kernels' source code into a unique file
  // When calculating the file name, store the unique ID to avoid having to
  // iterate over too many files at each execution.
  static int dbg_filename_id = 0;
  const auto *env = std::getenv("CA_OCL_DEBUG_PRINT_KERNELS");
  if (env && std::isdigit(env[0]) && std::atoi(env)) {
    const std::string dbg_filename_prefix = "cl_program_";
    const std::string dbg_filename_suffix = ".cl";

    // Try to find the next available filename. This isn't the most efficient
    // way, but it's for debugging only so it doesn't really matter.
    do {
      std::ostringstream dbg_filename_s;

      dbg_filename_s << dbg_filename_prefix << std::setw(7) << std::setfill('0')
                     << dbg_filename_id << dbg_filename_suffix;

      dbg_filename = dbg_filename_s.str();
      dbg_filename_id += 1;
    } while (llvm::sys::fs::exists(dbg_filename));

    std::ofstream dbg_fout(dbg_filename);
    // Print the macro definitions passed to the compiler (-D etc.)
    // This will print all the macros in between comments, to make them easy to
    // separate from the rest of the code. Each macro will have ifdef guards and
    // there will also be a big ifdef guard for all of them.
    if (!definitions.empty()) {
      dbg_fout << "// BEGIN MANUALLY SET MACROS\n";
      dbg_fout << "#ifndef CA_DISABLE_EMITTED_MACROS\n";
      for (const auto &definition : definitions) {
        // Definitions are in the form of "macro" or "macro=value"
        auto pos = definition.find_first_of('=');
        if (pos != definition.npos) {
          std::string macro = definition.substr(0, pos);
          std::string value = definition.substr(pos + 1);
          dbg_fout << "#ifndef " << macro << "\n#define " << macro << " "
                   << value << "\n"
                   << "#endif // " << macro << "\n";
        } else {
          dbg_fout << "#ifndef " << definition << "\n#define " << definition
                   << "\n"
                   << "#endif // " << definition << "\n";
        }
      }
      dbg_fout << "#endif // CA_DISABLE_EMITTED_MACROS\n";
      dbg_fout << "// END MANUALLY SET MACROS\n";
    }
    // Print the source
    dbg_fout << source.data();
  }
#endif  // CA_ENABLE_DEBUG_SUPPORT

  return dbg_filename;
}

std::string BaseModule::printKernelSource(llvm::StringRef source,
                                          llvm::StringRef path,
                                          clang::CodeGenOptions codeGenOpts) {
  llvm::StringRef kernel_file_name = "kernel.opencl";

  llvm::SmallString<128> absPath(path);
  if (!absPath.empty()) {
    // Make file path absolute
    llvm::sys::fs::make_absolute(absPath);

    // Split path into directory and filename.
    const size_t delimiter = absPath.find_last_of(PATH_SEPARATOR);
    if (llvm::StringRef::npos != delimiter) {
      kernel_file_name = absPath.substr(delimiter + 1);
      codeGenOpts.DebugCompilationDir = absPath.substr(0, delimiter).str();
    }

    // Write kernel source to disk if the file doesn't already exist.
    if (!llvm::sys::fs::exists(absPath)) {
      int fd;
      if (!llvm::sys::fs::createUniqueFile(absPath, fd, absPath)) {
        llvm::raw_fd_ostream Out(fd, /*shouldClose=*/true);
        Out << source;
        Out.close();
      }
    }
  }

  codeGenOpts.MainFileName = kernel_file_name.str();

  // It makes sure to report the errors in the input file, if provided. If the
  // source code didn't come from a file, the kernel file name reported in
  // errors will be (in increasing order of priority) the default of
  // "kernel.opencl", the uniquely-numbered "cl_program_XXXXXXXX.cl" debug file
  // which was written out, or the file written out as specified by the "-S"
  // command line option.
  if (!options.source_file_in.empty()) {
    kernel_file_name = options.source_file_in;
  }

  return kernel_file_name.str();
}

Result BaseModule::setOpenCLInstanceDefaults(
    clang::CompilerInstance &instance) {
  mux_device_info_t device_info = target.getCompilerInfo()->device_info;
  auto &codeGenOpts = instance.getCodeGenOpts();

  // Disable the llvm optimization passes that clang would normally run during
  // codegen because they aggressively optimize out integer divide by zero
  // operations which the CL spec requires us to preserve. These passes are run
  // manually later in the runCodegenLLVMPasses helper function.
  // !NOTE! this does mean we skip over a bunch of passes relating to profiling,
  // sanitizers and some other codegen related stuff. At present we don't use
  // these features but should we want to enable them codegen_optimizations.h in
  // the multi_llvm module will need to be modified in addition to enabling the
  // option here.
  codeGenOpts.DisableLLVMPasses = true;

  for (const auto &include_dir : options.include_dirs) {
    instance.getHeaderSearchOpts().AddPath(
        include_dir, clang::frontend::CSystem, false, false);
  }
  instance.getDiagnosticOpts().IgnoreWarnings = options.warn_ignore;
  if (options.warn_error) {
    instance.getDiagnosticOpts().Warnings.push_back("error");
  }

  std::string spir_triple;
  if (device_info->address_capabilities & mux_address_capabilities_bits64) {
    spir_triple = "spir64-unknown-unknown";
  } else if (device_info->address_capabilities &
             mux_address_capabilities_bits32) {
    spir_triple = "spir-unknown-unknown";
  } else {
    addBuildError(
        "The target device does not support 32 or 64-bit addressing.");
    return Result::COMPILE_PROGRAM_FAILURE;
  }

  instance.getTargetOpts().Triple = spir_triple;

  const auto OpenCLInputKind = clang::Language::OpenCL;
  auto &lang_opts = instance.getLangOpts();
  auto &pp_opts = instance.getPreprocessorOpts();

  const clang::LangStandard::Kind standard = setClangOpenCLStandard(lang_opts);

  const llvm::Triple triple(spir_triple);
  clang::LangOptions::setLangDefaults(lang_opts, OpenCLInputKind, triple,
                                      pp_opts.Includes, standard);
  setDefaultOpenCLLangOpts(lang_opts);

  return Result::SUCCESS;
}

clang::FrontendInputFile BaseModule::prepareOpenCLInputFile(
    clang::CompilerInstance &instance, llvm::StringRef source,
    std::string kernel_file_name, const OpenCLOptVec &opencl_opts,
    cargo::array_view<compiler::InputHeader> input_headers) {
  mux_device_info_t device_info = target.getCompilerInfo()->device_info;
  auto &pp_opts = instance.getPreprocessorOpts();

  const auto OpenCLInputKind = clang::Language::OpenCL;

  std::unique_ptr<llvm::MemoryBuffer> buffer(
      llvm::MemoryBuffer::getMemBuffer(source));
  clang::FrontendInputFile kernelFile(kernel_file_name, OpenCLInputKind);
  instance.getFrontendOpts().Inputs.push_back(kernelFile);
  pp_opts.addRemappedFile(kernel_file_name, buffer.release());
  pp_opts.DisablePCHOrModuleValidation =
      clang::DisableValidationForModuleKind::All;
  pp_opts.AllowPCHWithCompilerErrors = true;

  instance.setTarget(clang::TargetInfo::CreateTargetInfo(
      instance.getDiagnostics(),
      std::make_shared<clang::TargetOptions>(instance.getTargetOpts())));

  // We add the supported OpenCL opts now as we need an existing target before
  // we can do so.
  populateOpenCLOpts(instance, opencl_opts);

  instance.createFileManager();
  instance.createSourceManager(instance.getFileManager());

  auto addIncludeFile = [&](const std::string &name, const void *data,
                            const size_t size) {
    const clang::FileEntryRef entry =
        instance.getFileManager().getVirtualFileRef(
            "include" PATH_SEPARATOR + name, size, 0);
    std::unique_ptr<llvm::MemoryBuffer> buffer{
        new BakedMemoryBuffer(data, size)};
    instance.getSourceManager().overrideFileContents(entry, std::move(buffer));
  };

  if (options.standard >= Standard::OpenCLC30) {
    const auto &source = builtins::get_api_30_src_file();
    const std::string name = "builtins-3.0.h";
    addIncludeFile(name, source.data(), source.size());
    // Add the forced header to the list of includes
    pp_opts.Includes.push_back(name);
  }

  // Load optional force-include header
  auto device_header =
      builtins::get_api_force_file_device(device_info->device_name);
  if (device_header.size() > 1) {
    const std::string name = "device.h";
    addIncludeFile(name, device_header.data(), device_header.size());
    // Add the forced header to the list of includes
    pp_opts.Includes.push_back(name);
  }

  if (!input_headers.empty()) {
    for (const auto &input_header : input_headers) {
      addIncludeFile(cargo::as<std::string>(input_header.name),
                     input_header.source.data(), input_header.source.size());
    }
  }

  return kernelFile;
}

void BaseModule::loadBuiltinsPCH(clang::CompilerInstance &instance) {
  clang::ASTContext *astContext = &(instance.getASTContext());

  auto reader = std::make_unique<clang::ASTReader>(
      instance.getPreprocessor(), instance.getModuleCache(), astContext,
      instance.getPCHContainerReader(),
      instance.getFrontendOpts().ModuleFileExtensions, "",
      clang::DisableValidationForModuleKind::All, false, true, false, false);

  instance.setASTReader(reader.get());
  llvm::StringRef builtinsName("builtins.opencl");

  // deduce whether device meets all the requirements for doubles

  auto caps = target.getCompilerInfo()->getBuiltinCapabilities();

  auto kernelAPI = builtins::get_pch_file(caps);

  std::unique_ptr<llvm::MemoryBuffer> builtins_buffer{
      new BakedMemoryBuffer(kernelAPI.data(), kernelAPI.size())};

  reader->addInMemoryBuffer(builtinsName, std::move(builtins_buffer));

  const clang::ASTReader::ASTReadResult astReaderResult =
      reader->ReadAST(builtinsName, clang::serialization::MK_PCH,
                      clang::SourceLocation(), clang::ASTReader::ARR_None);
  if (clang::ASTReader::Success != astReaderResult) {
    CPL_ABORT(
        "BaseModule::loadBuiltinsPCH. Error compiling program: unable "
        "to load precompiled header.");
  }

  const ScopedDiagnosticHandler handler(*this);

  // Load the builtins header as a virtual file. This is required by Clang which
  // needs to access the contents of the header even when using PCH files.
  clang::serialization::ModuleFile *moduleFile =
      reader->getModuleManager().lookupByFileName(builtinsName);
  const bool builtinsLoaded = loadKernelAPIHeader(instance, moduleFile);
  if (!builtinsLoaded) {
    CPL_ABORT(
        "BaseModule::loadBuiltinsPCH. Error compiling program: unable "
        "to load builtins header.");
  }

  const llvm::IntrusiveRefCntPtr<clang::ExternalASTSource> pchAST(
      reader.release());
  instance.getASTContext().setExternalSource(pchAST);
}

void BaseModule::runOpenCLFrontendPipeline(
    const clang::CodeGenOptions &codeGenOpts,
    std::optional<llvm::ModulePassManager> early_passes,
    std::optional<llvm::ModulePassManager> late_passes) {
  if (options.fast_math) {
    if (late_passes.has_value()) {
      late_passes->addPass(FastMathPass());
    } else {
      late_passes = llvm::ModulePassManager();
      late_passes->addPass(FastMathPass());
    }
  }

  runFrontendPipeline(*this, *llvm_module, codeGenOpts, std::move(early_passes),
                      std::move(late_passes));
}

void BaseModule::FrontendDiagnosticPrinter::HandleDiagnostic(
    clang::DiagnosticsEngine::Level Level, const clang::Diagnostic &Info) {
  // Flush whatever we've built up already
  TempStr.clear();
  // Emit the diagnostic to TempOS (and thus TempStr)
  TextDiagnosticPrinter::HandleDiagnostic(Level, Info);
  // Ensure we've finished writing
  TempOS.flush();
  // Emit the diagnostic into the build log
  base_module.addDiagnostic(TempStr);
  // Forward the diagnostic onto the callback function, if set
  if (auto callback = base_module.target.getNotifyCallbackFn()) {
    callback(TempStr.c_str(), /*data*/ nullptr, /*data_size*/ 0);
  }
}

Result BaseModule::compileOpenCLC(
    cargo::string_view device_profile, cargo::string_view source_sv,
    cargo::array_view<compiler::InputHeader> input_headers) {
  clang::CompilerInstance instance;

  llvm_module = compileOpenCLCToIR(instance, device_profile, source_sv,
                                   input_headers, &num_errors, &state);

  if (!llvm_module) {
    return compiler::Result::COMPILE_PROGRAM_FAILURE;
  }

  // Now run the passes we skipped by enabling the DisableLLVMPasses option
  // earlier.
  const std::lock_guard<compiler::BaseContext> guard(context);
  runOpenCLFrontendPipeline(instance.getCodeGenOpts(), getEarlyOpenCLCPasses());

  return compiler::Result::SUCCESS;
}

std::unique_ptr<llvm::Module> BaseModule::compileOpenCLCToIR(
    clang::CompilerInstance &instance, cargo::string_view device_profile,
    cargo::string_view source_sv,
    cargo::array_view<compiler::InputHeader> input_headers,
    uint32_t *num_errors, ModuleState *new_state) {
  const llvm::StringRef source{source_sv.data(), source_sv.size()};

  MacroDefVec macro_defs;
  OpenCLOptVec opencl_opts;

  addDefaultOpenCLPreprocessorOpts(device_profile, macro_defs, opencl_opts);
  populatePPOpts(instance, macro_defs);

  // Populate our codegen options based on the compiler options we've got.
  auto &codeGenOpts = instance.getCodeGenOpts();
  populateCodeGenOpts(codeGenOpts);

  auto result = setOpenCLInstanceDefaults(instance);
  if (result != Result::SUCCESS) {
    return nullptr;
  }

  // TODO(CA-608): Allow developers to inject LLVM options for debugging at
  // this point, formerly called OCL_LLVM_DEBUG was remove due to lack of use.

#ifdef CA_ENABLE_DEBUG_SUPPORT
  const std::string dbg_filename =
      debugDumpKernelSource(source, options.definitions);
#else
  const std::string dbg_filename;
#endif  // CA_ENABLE_DEBUG_SUPPORT

  instance.createDiagnostics(
      new FrontendDiagnosticPrinter(*this, &instance.getDiagnosticOpts()));

  // Write a copy of the kernel source out to disk and update the debug info
  // to point to the location as the kernel source file.
  auto kernel_file_name = printKernelSource(
      source, options.source_file.length() ? options.source_file : dbg_filename,
      codeGenOpts);

  auto kernelFile = prepareOpenCLInputFile(instance, source, kernel_file_name,
                                           opencl_opts, input_headers);

  // Now we're actually going to start doing work, so need to lock LLVMContext.
  const std::lock_guard<compiler::BaseContext> guard(context);

  clang::EmitLLVMOnlyAction action(&target.getLLVMContext());

  // Prepare the action for processing kernelFile
  {
    // BeginSourceFile accesses LLVM global variables: LLVMTimePassesEnabled
    // and LLVMTimePassesPerRun.
    const std::lock_guard<std::mutex> globalLock(
        compiler::utils::getLLVMGlobalMutex());
    if (!action.BeginSourceFile(instance, kernelFile)) {
      return nullptr;
    }
  }

  loadBuiltinsPCH(instance);

  {
    // At this point we have already locked the LLVMContext mutex for the
    // current context we are operating on.  If, however, an OpenCL programmer
    // uses multiple cl_context in parallel they can invoke multiple compiler
    // instances in parallel.  This is generally safe, as each context is
    // independent.  Unfortunately, Clang has some global option handling code
    // that does not affect us, but is still run and causes multiple threads to
    // write to a large global object at once (GlobalParser in LLVM).  On x86
    // this did not seem to matter, on AArch64 it caused crashes due to double
    // free's within a std::string's destructor.  So, we lock globally before
    // asking Clang to process this source file.
    const std::lock_guard<std::mutex> guard(
        compiler::utils::getLLVMGlobalMutex());
    if (action.Execute()) {
      return nullptr;
    }
    action.EndSourceFile();
  }

  clang::DiagnosticConsumer *const consumer =
      instance.getDiagnostics().getClient();
  consumer->finish();

  const uint32_t errs = consumer->getNumErrors();
  if (num_errors) {
    *num_errors = errs;
  }
  if (errs > 0) {
    return nullptr;
  }

  if (new_state) {
    *new_state = ModuleState::COMPILED_OBJECT;
  }
  auto mod = action.takeModule();

  if (!mod) {
    return mod;
  }

  if (hasRecursiveKernels(mod.get())) {
    addBuildError("Recursive OpenCL kernels are not supported.");
    return nullptr;
  }

  createOpenCLKernelsMetadata(*mod);

  return mod;
}

Result BaseModule::link(cargo::array_view<Module *> input_modules) {
  std::unique_ptr<llvm::Module> module;

  // We'll need to lock the LLVMContext for the whole function.
  const std::lock_guard<compiler::BaseContext> guard(context);

  auto filter_func = [](const llvm::DiagnosticInfo &DI) {
    switch (DI.getSeverity()) {
      default:
        return false;
      case llvm::DiagnosticSeverity::DS_Warning:
      case llvm::DiagnosticSeverity::DS_Error:
        return true;
    }
  };
  const ScopedDiagnosticHandler handler(*this, filter_func);

  if (ModuleState::COMPILED_OBJECT == state) {
    module = llvm::CloneModule(*this->llvm_module);
  } else {
    module = std::unique_ptr<llvm::Module>(
        new llvm::Module("::ca_module_id", target.getLLVMContext()));
  }

  for (auto input_module_interface : input_modules) {
    auto input_module =
        static_cast<compiler::BaseModule *>(input_module_interface);
    // We need to clone the LLVM module for the input program as LLVM does not
    // preserve the source module during linking, and a program can be linked
    // multiple times.
    const llvm::Module *m = input_module->llvm_module.get();
    if (&target.getLLVMContext() != &m->getContext()) {
      CPL_ABORT(
          "BaseModule::link. Error linking program: Cannot clone "
          "with incompatible contexts.");
    }
    auto clone = llvm::CloneModule(*m);

    // if any of the input programs had argument metadata, we need to ensure it
    // will be preserved
    if (input_module->options.kernel_arg_info) {
      this->options.kernel_arg_info = true;
    }

    if (llvm::Linker::linkModules(*module.get(), std::move(clone))) {
      return Result::LINK_PROGRAM_FAILURE;
    }
  }

  switch (state) {
    case ModuleState::COMPILED_OBJECT:
      this->llvm_module.reset();
      break;
    case ModuleState::NONE:
      break;
    default:
      CPL_ABORT(
          "BaseModule::link. Error linking program: Program in invalid "
          "state.");
  }

  // Always creates a library. clBuildProgram and clLinkProgram call this
  // function to generate an executable (finalize the program) if
  // necessary, e.g., when the -create-library option is passed to
  // clLinkProgram.
  this->llvm_module = std::move(module);
  state = ModuleState::LIBRARY;

  return Result::SUCCESS;
}

bool BaseModule::DiagnosticHandler::handleDiagnostics(
    const llvm::DiagnosticInfo &DI) {
  if (filter_fn) {
    if (!filter_fn(DI)) {
      return true;
    }
  } else if (auto *Remark =
                 llvm::dyn_cast<llvm::DiagnosticInfoOptimizationBase>(&DI)) {
    // Optimization remarks are selective. They need to check whether the regexp
    // pattern, passed via one of the -pass-remarks* flags, matches the name of
    // the pass that is emitting the diagnostic. If there is no match, ignore
    // the diagnostic and return.
    //
    // Also noisy remarks are only enabled if we have hotness information to
    // sort them.
    if (!Remark->isEnabled() ||
        (Remark->isVerbose() && !Remark->getHotness())) {
      return true;
    }
  }

  std::string diagnostic;
  llvm::raw_string_ostream stream(diagnostic);
  llvm::DiagnosticPrinterRawOStream DPROS(stream);

  DPROS << llvm::LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity())
        << ": ";
  DI.print(DPROS);
  DPROS << "\n";
  stream.flush();

  if (DI.getSeverity() == llvm::DiagnosticSeverity::DS_Error) {
    base_module.addBuildError(diagnostic);
  } else {
    base_module.addDiagnostic(diagnostic);
  }

  if (base_module.target.getNotifyCallbackFn()) {
    base_module.target.getNotifyCallbackFn()(diagnostic.c_str(), nullptr, 0);
  }

  return true;
}

Result BaseModule::finalize(
    ProgramInfo *program_info,
    std::vector<builtins::printf::descriptor> &printf_calls) {
  // Lock the context, this is necessary due to analysis/pass managers being
  // owned by the LLVMContext and we are making heavy use of both below.
  const std::lock_guard<compiler::BaseContext> contextLock(context);
  // Numerous things below touch LLVM's global state, in particular
  // retriggering command-line option parsing at various points. Ensure we
  // avoid data races by locking the LLVM global mutex.
  const std::lock_guard<std::mutex> globalLock(
      compiler::utils::getLLVMGlobalMutex());

  if (!llvm_module) {
    CPL_ABORT(
        "BaseModule::finalize. Error finalizing "
        "program: Module is not initialised.");
  }

  mux_device_info_t device_info = target.getCompilerInfo()->device_info;

  // Further on we will be cloning the module, this will not work with
  // mismatching contexts.
  const llvm::Module *m = llvm_module.get();
  if (&target.getLLVMContext() != &m->getContext()) {
    CPL_ABORT(
        "BaseModule::finalize. Error finalizing program: Cannot "
        "clone with incompatible contexts.");
  }

  auto pass_mach = createPassMachinery();
  initializePassMachineryForFinalize(*pass_mach);

  // Forward on any compiler options required.
  static_cast<compiler::BaseModulePassMachinery &>(*pass_mach)
      .setCompilerOptions(options);

  llvm::ModulePassManager pm;

  // Compute the immutable DeviceInfoAnalysis so that cached retrievals work.
  pm.addPass(llvm::RequireAnalysisPass<compiler::utils::DeviceInfoAnalysis,
                                       llvm::Module>());

  if (auto *target_machine = pass_mach->getTM()) {
    const std::string triple = target_machine->getTargetTriple().normalize();
    auto DL = target_machine->createDataLayout();
    pm.addPass(
        compiler::utils::SimpleCallbackPass([triple, DL](llvm::Module &m) {
          m.setDataLayout(DL);
          m.setTargetTriple(triple);
        }));
  }

  pm.addPass(compiler::utils::VerifyReqdSubGroupSizeLegalPass());

#if LLVM_VERSION_GREATER_EQUAL(17, 0)
  const compiler::utils::ReplaceTargetExtTysOptions RTETOpts;
  pm.addPass(compiler::utils::ReplaceTargetExtTysPass(RTETOpts));
#endif

  // Lower all language-level builtins with corresponding mux builtins
  pm.addPass(compiler::utils::LowerToMuxBuiltinsPass());

  pm.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::SoftwareDivisionPass()));
  pm.addPass(compiler::ImageArgumentSubstitutionPass());
  pm.addPass(compiler::utils::ReplaceAtomicFuncsPass());

  compiler::utils::EncodeBuiltinRangeMetadataOptions Opts;
  // FIXME: We don't have a way to grab the maximum *global* work-group sizes
  // as being distinct from the local ones. See CA-4714.
  Opts.MaxLocalSizes[0] = device_info->max_work_group_size_x;
  Opts.MaxLocalSizes[1] = device_info->max_work_group_size_y;
  Opts.MaxLocalSizes[2] = device_info->max_work_group_size_z;
  pm.addPass(compiler::utils::EncodeBuiltinRangeMetadataPass(Opts));

  pm.addPass(compiler::utils::SimpleCallbackPass(
      [vecz_mode = options.vectorization_mode](llvm::Module &m) {
        for (auto &f : m) {
          compiler::encodeVectorizationMode(f, vecz_mode);
        }
      }));

  pm.addPass(compiler::utils::ReplaceC11AtomicFuncsPass());

  if (options.prevec_mode != compiler::PreVectorizationMode::NONE) {
    llvm::FunctionPassManager fpm;
    if (options.prevec_mode == compiler::PreVectorizationMode::ALL ||
        options.prevec_mode == compiler::PreVectorizationMode::SLP) {
      fpm.addPass(llvm::SLPVectorizerPass());
    }

    if (options.prevec_mode == compiler::PreVectorizationMode::ALL ||
        options.prevec_mode == compiler::PreVectorizationMode::LOOP) {
      // Loop vectorization apparently only works on loops with a single basic
      // block. Sometimes, Loop Rotation may be able to help us here.
      fpm.addPass(llvm::createFunctionToLoopPassAdaptor(
          llvm::LoopRotatePass(/*EnableHeaderDuplication*/ false)));
      fpm.addPass(llvm::LoopVectorizePass());

      // Loop vectorization also emits a scalar version of the loop, in case it
      // wasn't a multiple of the vector size, even when the loop count is a
      // compile-time constant that is a known multiple of the vector size.
      // In that case we get a redundant compare and branch to clean up.
      fpm.addPass(llvm::InstCombinePass());
      fpm.addPass(llvm::SimplifyCFGPass());
    }

    // SLP vectorization can leave a lot of unused GEPs lying around..
    fpm.addPass(llvm::DCEPass());

    pm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));
  }

  if (!options.opt_disable) {
    {
      llvm::FunctionPassManager fpm;
      fpm.addPass(llvm::InstCombinePass());
      fpm.addPass(llvm::ReassociatePass());
      fpm.addPass(compiler::MemToRegPass());
      fpm.addPass(llvm::BDCEPass());
      fpm.addPass(llvm::ADCEPass());
      fpm.addPass(llvm::SimplifyCFGPass());
      pm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));
    }
    pm.addPass(compiler::BuiltinSimplificationPass());
    {
      llvm::FunctionPassManager fpm;
      fpm.addPass(llvm::InstCombinePass());
      fpm.addPass(llvm::ReassociatePass());
      fpm.addPass(llvm::BDCEPass());
      fpm.addPass(llvm::ADCEPass());
      fpm.addPass(llvm::SimplifyCFGPass());
      pm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));
    }
  }

  if (!options.opt_disable) {
    pm.addPass(llvm::GlobalDCEPass());
    pm.addPass(pass_mach->getPB().buildInlinerPipeline(
        llvm::OptimizationLevel::O3, llvm::ThinOrFullLTOPhase::None));
  }

  pm.addPass(
      compiler::PrintfReplacementPass(&printf_calls, PRINTF_BUFFER_SIZE));

  {
    llvm::FunctionPassManager fpm;
    fpm.addPass(compiler::CombineFPExtFPTruncPass());
    fpm.addPass(compiler::CheckForUnsupportedTypesPass());
    pm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));
  }

  const ScopedDiagnosticHandler handler(*this);
  /// Set up an error handler to redirect fatal errors to the build log.
  const llvm::ScopedFatalErrorHandler error_handler(
      BaseModule::llvmFatalErrorHandler, this);

  // We need to clone the LLVM module as LLVM does not preserve the source
  // module during linking and the module can be used multiple times.
  auto clone = std::unique_ptr<llvm::Module>(llvm::CloneModule(*m));

  // Generate program info.
  if (program_info) {
    auto program_info_result = moduleToProgramInfo(*program_info, clone.get(),
                                                   options.kernel_arg_info);
    if (program_info_result != Result::SUCCESS) {
      return program_info_result;
    }
  }

  // Finally, check if there are any external functions that we don't have a
  // definition for, and error out if so
  pm.addPass(compiler::CheckForExtFuncsPass());

  // Add any target-specific passes
  pm.addPass(getLateTargetPasses(*pass_mach));

  llvm::CrashRecoveryContext CRC;
  llvm::CrashRecoveryContext::Enable();
  const bool crashed =
      !CRC.RunSafely([&] { pm.run(*clone, pass_mach->getMAM()); });
  llvm::CrashRecoveryContext::Disable();

  // Check if we've accumulated any errors
  if (crashed || num_errors) {
    return Result::FINALIZE_PROGRAM_FAILURE;
  }

  // Save the finalized LLVM module.
  finalized_llvm_module = std::move(clone);

  state = ModuleState::EXECUTABLE;
  return compiler::Result::SUCCESS;
}

Kernel *BaseModule::getKernel(const std::string &name) {
  if (!finalized_llvm_module) {
    return nullptr;
  }

  // Lookup or create kernel.
  const std::lock_guard<std::mutex> guard(kernel_mutex);

  if (kernel_map.count(name)) {
    return kernel_map[name].get();
  }

  auto *kernel = createKernel(name);
  if (kernel) {
    kernel_map[name] = std::unique_ptr<Kernel>(kernel);
  }
  return kernel;
}

std::size_t BaseModule::size() {
  std::size_t size = 0;

  // If this module contains nothing, then there's no LLVM module to serialize.
  if (state == ModuleState::NONE) {
    return size;
  }

  // Write the module state.
  size += sizeof(compiler::ModuleState);

  // Serialize the LLVM module.
  struct ostream final : public llvm::raw_ostream {
    std::size_t size;  // the total number of bytes required for the stream

    explicit ostream() : size(0) {}

    virtual void write_impl(const char *, std::size_t size) override {
      this->size += size;
    }

    virtual uint64_t current_pos() const override {
      return static_cast<uint64_t>(this->size);
    }
  } stream;

  {
    const std::lock_guard<compiler::BaseContext> guard(context);
    llvm::WriteBitcodeToFile(*llvm_module, stream);
  }
  stream.flush();

  size += stream.size;

  return size;
}

std::size_t BaseModule::serialize(std::uint8_t *output_buffer) {
  std::size_t total_written = 0;

  // If this module contains nothing, then there's no LLVM module to serialize.
  if (state == ModuleState::NONE) {
    return total_written;
  }

  // Write the module state.
  std::memcpy(output_buffer, &state, sizeof(state));
  output_buffer += sizeof(state);
  total_written += sizeof(state);

  // Serialize the LLVM module.
  struct ostream final : public llvm::raw_ostream {
    char *binary;
    std::size_t size;  // the total number of bytes required for the stream

    explicit ostream(char *binary) : binary(binary), size(0) {}

    virtual void write_impl(const char *ptr, std::size_t size) override {
      std::memcpy(binary + this->size, ptr, size);
      this->size += size;
    }

    virtual uint64_t current_pos() const override {
      return static_cast<uint64_t>(this->size);
    }
  } stream(reinterpret_cast<char *>(output_buffer));

  {
    const std::lock_guard<compiler::BaseContext> guard(context);
    llvm::WriteBitcodeToFile(*llvm_module, stream);
  }
  stream.flush();

  total_written += stream.size;

  return total_written;
}

bool BaseModule::deserialize(cargo::array_view<const std::uint8_t> buffer) {
  const std::lock_guard<compiler::BaseContext> guard(context);
  const ScopedDiagnosticHandler handler(*this);

  // If there's nothing to deserialize, that implies that the module is empty.
  if (buffer.empty()) {
    return true;
  }

  const std::uint8_t *buffer_read_ptr = buffer.data();

  // Get the module state.
  std::memcpy(&state, buffer_read_ptr, sizeof(state));
  buffer_read_ptr += sizeof(state);

  // Deserialize the LLVM module.
  const std::ptrdiff_t header_size = buffer_read_ptr - buffer.data();
  const DeserializeMemoryBuffer memoryBuffer(
      llvm::StringRef(reinterpret_cast<const char *>(buffer_read_ptr),
                      buffer.size() - header_size));
  auto errorOrModule(llvm::parseBitcodeFile(memoryBuffer.getMemBufferRef(),
                                            target.getLLVMContext()));

  if (errorOrModule) {
    llvm_module = std::move(errorOrModule.get());
    return true;
  } else {
    addBuildError(std::string("Failed to deserialize module: ") +
                  toString(errorOrModule.takeError()));
    return false;
  }
}

void BaseModule::addDiagnostic(cargo::string_view message) {
  log.append(message.data(), message.size());
  log.append("\n");
}

void BaseModule::addBuildError(cargo::string_view message) {
  num_errors++;
  addDiagnostic(message);
}

void BaseModule::llvmFatalErrorHandler(void *user_data, const char *reason,
                                       bool gen_crash_diag) {
  // Deliberately ignore gen_crash_diag - if this handler returns, LLVM's
  // report_fatal_error handling will either abort() if gen_crash_diag is true
  // or exit(1) if it's false.
  (void)gen_crash_diag;
  auto &base_module = *static_cast<BaseModule *>(user_data);
  llvm::SmallVector<char, 64> Buffer;
  llvm::raw_svector_ostream OS(Buffer);
  // Prepend 'LLVM ERROR' to make it look like the fatal errors other LLVM
  // tools produce. This is what report_fatal_error does without a handler such
  // as this.
  OS << "LLVM ERROR: " << reason;
  base_module.addBuildError(OS.str());
}

void BaseModule::createOpenCLKernelsMetadata(llvm::Module &mod) {
  const char *name = "opencl.kernels";

  // If the module is null, or the metadata we are looking for already exists,
  // bail out!
  if (mod.getNamedMetadata(name)) {
    return;
  }

  // LLVM doesn't fill out the opencl.kernels metadata anymore, so we need to
  auto md = mod.getOrInsertNamedMetadata(name);
  llvm::LLVMContext &ctx = mod.getContext();

  for (auto &function : mod.functions()) {
    // If the function is a kernel (as denoted by the calling convention), and
    // only if the kernel is a definition (and thus has all the correct metadata
    // we can copy).
    if (llvm::CallingConv::SPIR_KERNEL == function.getCallingConv() &&
        !function.isDeclaration()) {
      const char *names[] = {
          "kernel_arg_addr_space", "kernel_arg_access_qual", "kernel_arg_type",
          "kernel_arg_base_type",  "kernel_arg_type_qual",   "kernel_arg_name",
          "reqd_work_group_size",  "work_group_size_hint",   "vec_type_hint",
      };

      llvm::SmallVector<llvm::Metadata *, 8> nodes;

      // the first thing in our metadata is the kernel function
      nodes.push_back(llvm::ValueAsMetadata::get(&function));

      for (auto name : names) {
        // and the function metadata goes after the name
        if (auto mdf = function.getMetadata(name)) {
          llvm::SmallVector<llvm::Metadata *, 8> mds;

          // the name is the first operand of our resulting node
          mds.push_back(llvm::MDString::get(ctx, name));

          // the operands are the remaining
          for (const llvm::MDOperand &op : mdf->operands()) {
            mds.push_back(op);
          }

          nodes.push_back(llvm::MDTuple::get(ctx, mds));
        }

        // And erase the metadata from the function.
        function.setMetadata(name, nullptr);
      }

      md->addOperand(llvm::MDNode::get(ctx, nodes));
    }
  }
}

void BaseModule::populatePPOpts(clang::CompilerInstance &instance,
                                const MacroDefVec &macro_defs) const {
  auto &pp_opts = instance.getPreprocessorOpts();
  for (const auto &def : macro_defs) {
    switch (def.first) {
      case MacroDefType::Def:
        pp_opts.addMacroDef(def.second);
        break;
      case MacroDefType::Undef:
        pp_opts.addMacroUndef(def.second);
        break;
    }
  }
}

void BaseModule::populateOpenCLOpts(clang::CompilerInstance &instance,
                                    const OpenCLOptVec &opencl_opts) {
  for (const auto &opt : opencl_opts) {
    supportOpenCLOpt(instance, opt);
  }
}

std::unique_ptr<compiler::utils::PassMachinery>
BaseModule::createPassMachinery() {
  return std::make_unique<BaseModulePassMachinery>(
      llvm_module->getContext(), /*TM*/ nullptr, /*Info*/ std::nullopt,
      /*BICallback*/ nullptr, target.getContext().isLLVMVerifyEachEnabled(),
      target.getContext().getLLVMDebugLoggingLevel(),
      target.getContext().isLLVMTimePassesEnabled());
}

void BaseModule::initializePassMachineryForFrontend(
    compiler::utils::PassMachinery &pass_mach,
    const clang::CodeGenOptions &CGO) const {
  // For historical reasons, loop interleaving is set to mirror setting for loop
  // unrolling. - comment from clang source
  llvm::PipelineTuningOptions PTO;
  PTO.LoopInterleaving = CGO.UnrollLoops;
  PTO.LoopVectorization = CGO.VectorizeLoop;
  PTO.SLPVectorization = CGO.VectorizeSLP;

  pass_mach.initializeStart(PTO);

  // Register the target library analysis directly and give it a customized
  // preset TLI.
  const llvm::Triple TT = llvm::Triple(llvm_module->getTargetTriple());
  auto TLII = llvm::TargetLibraryInfoImpl(TT);

  // getVecLib()'s return type changed in LLVM 18.
  auto VecLib = CGO.getVecLib();
  using VecLibT = decltype(VecLib);
  switch (VecLib) {
    case VecLibT::Accelerate:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::Accelerate, TT);
      break;
    case VecLibT::SVML:
      TLII.addVectorizableFunctionsFromVecLib(llvm::TargetLibraryInfoImpl::SVML,
                                              TT);
      break;
    case VecLibT::MASSV:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::MASSV, TT);
      break;
    case VecLibT::LIBMVEC:
      switch (TT.getArch()) {
        default:
          break;
        case llvm::Triple::x86_64:
          TLII.addVectorizableFunctionsFromVecLib(
              llvm::TargetLibraryInfoImpl::LIBMVEC_X86, TT);
          break;
      }
      break;
    default:
      break;
  }

  pass_mach.getFAM().registerPass(
      [&TLII] { return llvm::TargetLibraryAnalysis(TLII); });

  pass_mach.initializeFinish();
}

void BaseModule::initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &pass_mach) const {
  pass_mach.initializeStart();
  pass_mach.initializeFinish();
}

}  // namespace compiler
