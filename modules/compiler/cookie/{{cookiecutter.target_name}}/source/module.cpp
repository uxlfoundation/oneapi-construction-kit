/// Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#include <base/macros.h>
#include <base/pass_pipelines.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/handle_barriers_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/lld_linker.h>
#include <compiler/utils/llvm_global_mutex.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Target/TargetMachine.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/optional_helper.h>
#include <{{cookiecutter.target_name}}/module.h>
#include <{{cookiecutter.target_name}}/target.h>
#include <vecz/pass.h>
#include <{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h>
{% if "replace_mem"  in cookiecutter.feature.split(";") -%}
#include <compiler/utils/replace_mem_intrinsics_pass.h>
{% endif -%}

{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
// Add an additional wrapper pass for refsi
#include <{{cookiecutter.target_name}}/refsi_wrapper_pass.h>
{% endif -%}

{% if "clmul"  in cookiecutter.feature.split(";") -%}
#include <compiler/utils/mangling.h>
#include <llvm/IR/IntrinsicsRISCV.h>

namespace {{cookiecutter.target_name}} {
  class CLTargetBuiltinInfo : public compiler::utils::CLBuiltinInfo {
   public:
    // Will be filled out by each example
    CLTargetBuiltinInfo(std::unique_ptr<compiler::utils::CLBuiltinLoader> L)
        : compiler::utils::CLBuiltinInfo(std::move(L)) {}
    compiler::utils::Builtin analyzeBuiltin(
        llvm::Function const &Builtin) const override {
      compiler::utils::NameMangler mangler(&Builtin.getParent()->getContext());
      llvm::StringRef BaseName = mangler.demangleName(Builtin.getName());

      if ((BaseName == "clmul") || (BaseName == "clmulh") ||
          (BaseName == "clmulr")) {
        unsigned Properties = compiler::utils::eBuiltinPropertyCanEmitInline |
                              compiler::utils::eBuiltinPropertyNoSideEffects;
        return compiler::utils::Builtin{
            Builtin, compiler::utils::eBuiltinUnknown,
            (compiler::utils::BuiltinProperties)Properties};
      }
      return compiler::utils::CLBuiltinInfo::analyzeBuiltin(Builtin);
    }
    llvm::Value *emitBuiltinInline(
        llvm::Function *Builtin, llvm::IRBuilder<> &B,
        llvm::ArrayRef<llvm::Value *> Args) override {
      if (Builtin) {       
        compiler::utils::NameMangler mangler(&Builtin->getParent()->getContext());
        llvm::StringRef BaseName = mangler.demangleName(Builtin->getName());           
        if (BaseName == "clmul") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmul,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        } else if (BaseName == "clmulh") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulh,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        } else if (BaseName == "clmulr") {
          return B.CreateIntrinsic(llvm::Intrinsic::riscv_clmulr,
                                  Builtin->getReturnType(), {Args[0], Args[1]});
        }
      }
      return compiler::utils::CLBuiltinInfo::emitBuiltinInline(Builtin, B, Args);
    }
  };
}
{% endif -%}

// Note: This is a legacy toggle for forcing vectorization with no scalar tail
// based on the "VF" environment variable. Ideally we'd be setting it on a
// per-function basis, and we'd also be setting the vectorization options
// themselves on a per-function basis. Until we've designed a new method, keep
// the legacy behaviour by re-parsing the "VF" environment variable and look
// for a "v/V" toggle.
static bool hasForceNoTail(const std::string &env_debug_prefix) {
  if (env_debug_prefix.empty()) {
    return false;
  }
  std::string env_name = env_debug_prefix + "_VF";
  if (auto *vecz_vf_flags_env = std::getenv(env_name.c_str())) {
    llvm::SmallVector<llvm::StringRef, 4> flags;
    llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "V" || r == "v") {
        return true;
      }
    }
  }
  return false;
}

static bool isEarlyBuiltinLinkingEnabled(const std::string &env_debug_prefix) {
  if (env_debug_prefix.empty()) {
    return false;
  }

  // Allow any decisions made on early link builtins to be overridden
  // with an env variable
  std::string env_name = env_debug_prefix + "_EARLY_LINK_BUILTINS";
  if (const char *early_link_builtins_env = getenv(env_name.c_str())) {
    return atoi(early_link_builtins_env);
  }

  // Else, check whether we're scalably-vectorizing. This should be kept in
  // sync with {{cookiecutter.target_name}}::processVeczFlags!
  env_name = env_debug_prefix + "_VF";
  if (auto *vecz_vf_flags_env = std::getenv(env_name.c_str())) {
    llvm::SmallVector<llvm::StringRef, 4> flags;
    llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "S" || r == "s") {
        return true;
      }
    }
  }

  return false;
}

namespace {{cookiecutter.target_name}} {
{{cookiecutter.target_name.capitalize()}}Module::{{cookiecutter.target_name.capitalize()}}Module({{cookiecutter.target_name.capitalize()}}Target &target, compiler::BaseContext &context,
               uint32_t &num_errors, std::string &log)
    : BaseModule(target, context, num_errors, log) {}

void {{cookiecutter.target_name.capitalize()}}Module::clear() {
  BaseModule::clear();
  object_code.clear();
}

compiler::Result {{cookiecutter.target_name.capitalize()}}Module::createBinary(cargo::array_view<std::uint8_t> &binary) {
  if (!finalized_llvm_module) {
    return compiler::Result::FINALIZE_PROGRAM_FAILURE;
  }

  // Lock the context, this is necessary due to analysis/pass managers being
  // owned by the LLVMContext and we are making heavy use of both below.
  std::lock_guard<compiler::BaseContext> contextLock(context);
  // Numerous things below touch LLVM's global state, in particular
  // retriggering command-line option parsing at various points. Ensure we
  // avoid data races by locking the LLVM global mutex.
  std::lock_guard<std::mutex> globalLock(compiler::utils::getLLVMGlobalMutex());

  // Write to an Elf object
  auto *TM = getTargetMachine();
  llvm::SmallVector<char, 512> objectBinary;
  llvm::raw_svector_ostream ostream(objectBinary);

  {
    compiler::Result err = compiler::Result::FAILURE;
    llvm::CrashRecoveryContext CRC;
    llvm::CrashRecoveryContext::Enable();
    bool crashed = !CRC.RunSafely([&] {
      err = compiler::emitCodeGenFile(*finalized_llvm_module, TM, ostream);
    });
    llvm::CrashRecoveryContext::Disable();
    if (crashed) {
      return compiler::Result::FINALIZE_PROGRAM_FAILURE;
    }
    if (compiler::Result::SUCCESS != err) {
      return err;
    }
  }

  llvm::ArrayRef<uint8_t> inputBinary{
      reinterpret_cast<uint8_t *>(objectBinary.data()),
      static_cast<std::size_t>(objectBinary.size())};

  llvm::SmallVector<std::string, 4> lld_args;
  // Set the entry point to the zero address to avoid a linker warning. The
  // entry point will not be used directly.
  lld_args.push_back("-e0");

  if ({{cookiecutter.link_shared}}) {
    lld_args.push_back("--shared");
  }

  {
    bool linkSuccess = false;
    llvm::CrashRecoveryContext CRC;
    llvm::CrashRecoveryContext::Enable();
    bool crashed = !CRC.RunSafely([&] {
      auto linkResult = compiler::utils::lldLinkToBinary(
          inputBinary, getTarget().hal_device_info->linker_script,
          getTarget().rt_lib, getTarget().rt_lib_size, lld_args);
      if (auto E = linkResult.takeError()) {
        std::string errStr = toString(std::move(E));
        addBuildError(errStr);
        if (auto callback = target.getNotifyCallbackFn()) {
          callback(errStr.c_str(), /*data*/ nullptr, /*data_size*/ 0);
        }
        return;
      }
      auto size = (*linkResult)->getBufferSize();
      if (cargo::success != object_code.alloc(size)) {
        return;
      }
      std::memcpy(object_code.data(), (*linkResult)->getBufferStart(), size);
      linkSuccess = true;
    });
    llvm::CrashRecoveryContext::Disable();
    if (crashed || !linkSuccess) {
      return compiler::Result::LINK_PROGRAM_FAILURE;
    }
  }

  // copy the generated ELF file to a specified path if desired
#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  if (!getTarget().env_debug_prefix.empty()) {
    std::string env_name = getTarget().env_debug_prefix + "_SAVE_ELF_PATH";
    if (const auto copyElfPath = llvm::sys::Process::GetEnv(env_name.c_str())) {
      llvm::SmallVector<char, 8> resultPath;
      std::error_code error;
      llvm::raw_fd_ostream of(*copyElfPath, error);
      if (error) {
        llvm::errs() << "Unable to open ELF file " << *copyElfPath << " :\n";
        llvm::errs() << "\t" << error.message() << "\n";
      } else {
        const uint8_t *elf_data = object_code.data();
        of.write(reinterpret_cast<const char *>(elf_data), object_code.size());
        llvm::errs() << "Writing ELF file  to " << *copyElfPath << "\n";
      }
    }
  }
#endif

  // return the binary buffer.
  binary = object_code;

  return compiler::Result::SUCCESS;
}

// No deferred support so just return nullptr
compiler::Kernel *{{cookiecutter.target_name.capitalize()}}Module::createKernel(const std::string &) { return nullptr; }

const {{cookiecutter.target_name.capitalize()}}Target &{{cookiecutter.target_name.capitalize()}}Module::getTarget() const {
  return *static_cast<{{cookiecutter.target_name.capitalize()}}Target *>(&target);
}

llvm::ModulePassManager {{cookiecutter.target_name.capitalize()}}Module::getLateTargetPasses(
    compiler::utils::PassMachinery &pass_mach) {
  if (getOptions().llvm_stats) {
    llvm::EnableStatistics();
  }

  const auto &env_debug_prefix = getTarget().env_debug_prefix;

#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  if (!env_debug_prefix.empty()) {
    std::string env_name_dump_ir = env_debug_prefix + "_DUMP_IR";
    if (const char *dumpIR = getenv(env_name_dump_ir.c_str())) {
      addIRSnapshotStages(dumpIR);
    }
    std::string env_name_dump_asm = env_debug_prefix + "_DUMP_ASM";
    if (getenv(env_name_dump_asm.c_str())) {
      std::string name = compiler::BaseModule::getTargetSnapshotName(
          "{{cookiecutter.target_name}}",
          {{cookiecutter.target_name}}::{{cookiecutter.target_name_capitals}}_SNAPSHOT_BACKEND);
      addInternalSnapshot(name.c_str());
    }
  }
#endif

  compiler::BasePassPipelineTuner tuner(options);

  llvm::ModulePassManager PM;

  PM.addPass(compiler::utils::TransferKernelMetadataPass());

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "{{cookiecutter.target_name}}", {{cookiecutter.target_name_capitals}}_SNAPSHOT_INPUT, snapshots);

  {% if "replace_mem"  in cookiecutter.feature.split(";") -%}
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceMemIntrinsicsPass()));
  {% endif -%} 

  // Forcibly compute the BuiltinInfoAnalysis so that cached retrievals work.
  PM.addPass(llvm::RequireAnalysisPass<compiler::utils::BuiltinInfoAnalysis,
                                       llvm::Module>());

  // This potentially fixes up any structs to match the spir alignment
  // before we change to the backend layout
  PM.addPass(compiler::utils::AlignModuleStructsPass());

  // Add builtin replacement passes here directly to PM if needed

  if (isEarlyBuiltinLinkingEnabled(env_debug_prefix)) {
    PM.addPass(compiler::utils::LinkBuiltinsPass(/*EarlyLinking*/ true));
  }

  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "{{cookiecutter.target_name}}", {{cookiecutter.target_name_capitals}}_SNAPSHOT_VECTORIZED, snapshots);

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::HandleBarriersOptions HBOpts;
  HBOpts.IsDebug = options.opt_disable;
  HBOpts.ForceNoTail = hasForceNoTail(env_debug_prefix);
  PM.addPass(compiler::utils::HandleBarriersPass(HBOpts));

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "{{cookiecutter.target_name}}", {{cookiecutter.target_name_capitals}}_SNAPSHOT_BARRIER, snapshots);

  compiler::addPrepareWorkGroupSchedulingPasses(PM);

  compiler::utils::AddKernelWrapperPassOptions KWOpts;
  // We don't bundle kernel arguments in a packed struct.
  KWOpts.IsPackedStruct = false;
  PM.addPass(compiler::utils::AddKernelWrapperPass(KWOpts));

  PM.addPass(compiler::utils::ReplaceLocalModuleScopeVariablesPass());

  // Add final passes here by adding directly to PM as needed
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
  // Add an additional wrapper pass for refsi
  PM.addPass({{cookiecutter.target_name}}::RefSiM1WrapperPass());
{% endif -%}

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  addLLVMDefaultPerModulePipeline(PM, pass_mach.getPB(), options);

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "{{cookiecutter.target_name}}", {{cookiecutter.target_name_capitals}}_SNAPSHOT_SCHEDULED, snapshots);

  // With all passes scheduled, add a snapshot to view the assembly/object
  // file, if requested.
  if (auto snapshot = compiler::BaseModule::shouldTakeTargetSnapshot(
          "{{cookiecutter.target_name}}", {{cookiecutter.target_name_capitals}}_SNAPSHOT_BACKEND, snapshots)) {
    PM.addPass(compiler::utils::SimpleCallbackPass(
        [snapshot, TM = pass_mach.getTM(), this](llvm::Module &m) {
          takeBackendSnapshot(&m, TM, *snapshot);
        }));
  }

  return PM;
}

static llvm::TargetMachine *createTargetMachine(const {{cookiecutter.target_name_capitalize}}Target &target) {
  // Init the llvm target machine.
  llvm::TargetOptions options;
  std::string error;
  const llvm::Target *llvm_target =
      llvm::TargetRegistry::lookupTarget(target.llvm_triple, error);
  if (nullptr == llvm_target) {
    return nullptr;
  }

  options.MCOptions.ABIName = target.llvm_abi;

  return llvm_target->createTargetMachine(
      target.llvm_triple, target.llvm_cpu, target.llvm_features, options,
      llvm::Reloc::Model::Static, llvm::CodeModel::Small,
      llvm::CodeGenOpt::Aggressive);
}

llvm::TargetMachine *{{cookiecutter.target_name.capitalize()}}Module::getTargetMachine() {
  if (!target_machine.get()) {
    target_machine.reset(createTargetMachine(getTarget()));
  }
  return target_machine.get();
}
{% if "clmul"  in cookiecutter.feature.split(";") -%}
  std::unique_ptr<compiler::utils::BILangInfoConcept> createSimpleTargetCLBuiltinInfo(
      llvm::Module *Builtins) {
    return std::make_unique<CLTargetBuiltinInfo>(
        std::make_unique<compiler::utils::SimpleCLBuiltinLoader>(Builtins));
  }
{% endif -%}

std::unique_ptr<compiler::utils::PassMachinery> {{cookiecutter.target_name.capitalize()}}Module::createPassMachinery() {
  auto *TM = getTargetMachine();
  auto *Builtins = getTarget().getBuiltins();
  const auto &BaseContext = getTarget().getContext();

  compiler::utils::DeviceInfo Info = compiler::initDeviceInfoFromMux(
      getTarget().getCompilerInfo()->device_info);

  auto Callback = [Builtins](const llvm::Module &) {
{% if "clmul"  in cookiecutter.feature.split(";") -%}
    return compiler::utils::BuiltinInfo(createSimpleTargetCLBuiltinInfo(Builtins));
{% else -%}
    return compiler::utils::BuiltinInfo(compiler::utils::createCLBuiltinInfo(Builtins));
{% endif -%}
  };
  llvm::LLVMContext &Ctx = Builtins->getContext();
  return multi_llvm::make_unique<{{cookiecutter.target_name.capitalize()}}PassMachinery>(
      Ctx, TM, Info, Callback, BaseContext.isLLVMVerifyEachEnabled(),
      BaseContext.getLLVMDebugLoggingLevel(),
      BaseContext.isLLVMTimePassesEnabled());
}

void {{cookiecutter.target_name.capitalize()}}Module::initializePassMachineryForFrontend(
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
  llvm::Triple TT = llvm::Triple(target_machine->getTargetTriple());
  auto TLII = llvm::TargetLibraryInfoImpl(TT);

  switch (CGO.getVecLib()) {
    case clang::CodeGenOptions::Accelerate:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::Accelerate);
      break;
    case clang::CodeGenOptions::SVML:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::SVML);
      break;
    case clang::CodeGenOptions::MASSV:
      TLII.addVectorizableFunctionsFromVecLib(
          llvm::TargetLibraryInfoImpl::MASSV);
      break;
    case clang::CodeGenOptions::LIBMVEC:
      switch (TT.getArch()) {
        default:
          break;
        case llvm::Triple::x86_64:
          TLII.addVectorizableFunctionsFromVecLib(
              llvm::TargetLibraryInfoImpl::LIBMVEC_X86);
          break;
      }
      break;
    default:
      break;
  }

  TLII.disableAllFunctions();

  pass_mach.getFAM().registerPass(
      [&TLII] { return llvm::TargetLibraryAnalysis(TLII); });
  auto *TM = pass_mach.getTM();
  if (TM) {
    pass_mach.getFAM().registerPass(
        [TM] { return llvm::TargetIRAnalysis(TM->getTargetIRAnalysis()); });
  }

  pass_mach.initializeFinish();
}

void {{cookiecutter.target_name.capitalize()}}Module::initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &pass_mach) const {
  pass_mach.initializeStart();
  // Ensure that the optimizer doesn't inject calls to library functions that
  // can't be supported on a free-standing device.
  //
  // We cannot use PassManagerBuilder::LibraryInfo here, since the analysis
  // has to be added to the pass manager prior to other passes being added.
  // This is because other passes might require TargetLibraryInfoWrapper, and
  // if they do a TargetLibraryInfoImpl object with default settings will be
  // created prior to adding the pass. Trying to add a
  // TargetLibraryInfoWrapper analysis with disabled functions later will have
  // no affect, due to the analysis already being registered with the pass
  // manager.
  auto LibraryInfo =
      llvm::TargetLibraryInfoImpl(pass_mach.getTM()->getTargetTriple());
  LibraryInfo.disableAllFunctions();
  pass_mach.getFAM().registerPass(
      [&LibraryInfo] { return llvm::TargetLibraryAnalysis(LibraryInfo); });
  pass_mach.initializeFinish();
}

// Snapshot Support

static auto checkSnapshotAvailable(
    const char *stage, const std::vector<std::string> &available_snapshots) {
  return std::find_if(available_snapshots.begin(), available_snapshots.end(),
                      [stage](const std::string &s) { return s == stage; });
}

static void printSnapshot(size_t snapshot_size, const char *snapshot_data,
                          void *, void *) {
  llvm::dbgs() << llvm::StringRef(snapshot_data, snapshot_size) << "\n";
}

void {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Module::addInternalSnapshot(const char *stage) {
  auto it = checkSnapshotAvailable(stage, getTarget().available_snapshots);
  if (it == getTarget().available_snapshots.end()) {
    return;
  }
  compiler::BaseModule::SnapshotDetails snapshot;
  snapshot.format = compiler::SnapshotFormat::TEXT;
  snapshot.stage = it->c_str();
  snapshot.callback = printSnapshot;
  snapshot.user_data = nullptr;

  snapshots.push_back(snapshot);
}

bool {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Module::getStagesFromDumpIRString(
    std::vector<std::string> &stages, const char *dump_ir_string) {
  if (!dump_ir_string || !strcmp(dump_ir_string, "") ||
      !strcmp(dump_ir_string, "0")) {
    return false;
  }

  // If just passed 1, add a default snapshot corresponding to the latest IR
  // stage in the pipeline.
  if (!strcmp(dump_ir_string, "1")) {
    stages.push_back(compiler::BaseModule::getTargetSnapshotName(
        "{{cookiecutter.target_name}}", {{cookiecutter.target_name}}::{{cookiecutter.target_name_capitals}}_SNAPSHOT_SCHEDULED));
    return true;
  }

  // Try to parse a list of stages.
  const char delimiter = ',';
  size_t num_valid_stages = 0;
  std::stringstream ss(dump_ir_string);
  std::string stage_name;
  while (std::getline(ss, stage_name, delimiter)) {
    // Try a full match
    auto it =
        checkSnapshotAvailable(stage_name.c_str(), getTarget().available_snapshots);
    if (it == getTarget().available_snapshots.end()) {
      // Else try partial matches
      const std::string prefixes[] = {
          "cl_snapshot_compilation_",
          "cl_snapshot_{{cookiecutter.target_name}}_"};
      for (cargo::string_view prefix : prefixes) {
        std::string full_name = std::string(prefix.data()) + stage_name;
        it = checkSnapshotAvailable(full_name.c_str(),
                                    getTarget().available_snapshots);
        if (it != getTarget().available_snapshots.end()) {
          break;
        }
      }

      if (it != getTarget().available_snapshots.end()) {
        stages.push_back(*it);
        num_valid_stages++;
      }
    }
  }

  return num_valid_stages != 0;
}

bool {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Module::addIRSnapshotStages(const char *stages_text) {
  std::vector<std::string> stages;
  if (getStagesFromDumpIRString(stages, stages_text)) {
    for (const std::string &stage : stages) {
      addInternalSnapshot(stage.c_str());
    }
    return true;
  }
  return false;
}

void {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Module::takeBackendSnapshot(
    llvm::Module *M, llvm::TargetMachine *TM,
    const compiler::BaseModule::SnapshotDetails &snapshot) {
  llvm::SmallVector<char, 1024> snapshot_data;
  llvm::raw_svector_ostream stream(snapshot_data);

  // Clone the module so we leave it in the same state after we compile.
  auto clonedM = std::unique_ptr<llvm::Module>(llvm::CloneModule(*M));
  bool emit_assembly = snapshot.format != compiler::SnapshotFormat::BINARY;
  compiler::emitCodeGenFile(*clonedM, TM, stream, emit_assembly);

  cargo::string_view suffix_str =
      snapshot.format == compiler::SnapshotFormat::BINARY ? ".o" : ".s";
  snapshot.callback(snapshot_data.size(), snapshot_data.data(),
                    (void *)suffix_str.data(), snapshot.user_data);
}

}  // namespace {{cookiecutter.target_name}}
