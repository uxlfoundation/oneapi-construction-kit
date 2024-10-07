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

#include "muxc.h"

#include <base/base_module_pass_machinery.h>
#include <base/module.h>
#include <clang/Frontend/CompilerInstance.h>
#include <compiler/library.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IRPrinter/IRPrintingPasses.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/ToolOutputFile.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

// Additional arguments beyond standard llvm command options
static cl::opt<std::string> InputFilename("input", cl::Positional,
                                          cl::desc("<input .bc or .ll file>"),
                                          cl::init("-"));

static cl::opt<std::string> InputLanguage(
    "x", cl::desc("Input language ('cl' or 'ir')"));

static cl::opt<std::string> PipelineText(
    "passes", cl::desc("pipeline to run, passes separated by ','"),
    cl::init(""));

static cl::opt<std::string> OutputFilename(
    "o", cl::desc("Override output filename (default stdout)"),
    cl::value_desc("filename"), cl::init("-"));

static cl::opt<bool> ListDevices("list-devices", cl::desc("list devices"),
                                 cl::value_desc("list-devices"));

static cl::opt<bool> WriteTextual(
    "S", cl::desc("Write module as text. Deprecated: does nothing"),
    cl::init(true));

static cl::opt<CodeGenFileType> FileType(
    "filetype", cl::init(multi_llvm::CodeGenFileType::AssemblyFile),
    cl::desc("Choose a file type:"),
    cl::values(clEnumValN(multi_llvm::CodeGenFileType::AssemblyFile, "asm",
                          "Emit a textual file"),
               clEnumValN(multi_llvm::CodeGenFileType::ObjectFile, "obj",
                          "Emit a binary object file"),
               clEnumValN(multi_llvm::CodeGenFileType::Null, "null",
                          "Emit nothing, for performance testing")));

static cl::opt<int> DeviceIdx(
    "device-idx",
    cl::desc("select device by index; see --list-devices for indices.\nTakes "
             "precedence over --device"),
    cl::value_desc("idx"), cl::init(-1));

static cl::opt<std::string> DeviceName("device", cl::desc("select device"),
                                       cl::value_desc("name"));

static cl::opt<std::string> CLOptions("cl-options", cl::desc("options"));

static cl::opt<bool> PrintPasses(
    "print-passes",
    cl::desc("Print available passes that can be specified in "
             "--passes=foo and exit (not including LLVM ones)"));

static cl::opt<bool> HalfCap(
    "device-fp16-capabilities",
    cl::desc("Enable/Disable device fp16 capabilities"), cl::init(true));
static cl::opt<bool> FloatCap(
    "device-fp32-capabilities",
    cl::desc("Enable/Disable device fp32 capabilities"), cl::init(true));
static cl::opt<bool> DoubleCap(
    "device-fp64-capabilities",
    cl::desc("Enable/Disable device fp64 capabilities"), cl::init(true));

static cl::list<unsigned> SGSizes(
    "device-sg-sizes",
    cl::desc("Comma-separated list of supported sub-group sizes"),
    cl::CommaSeparated);

int main(int argc, char **argv) {
  muxc::driver driver;
  driver.parseArguments(argc, argv);

  if (auto Err = driver.setupContext()) {
    errs() << "Error setting up context: " << toString(std::move(Err)) << "\n";
    return 1;
  }

  std::unique_ptr<Module> M;
  // If we're just going to print available passes, we don't need to handle any
  // input.
  if (!PrintPasses) {
    auto MRes = driver.convertInputToIR();
    if (auto Err = MRes.takeError()) {
      errs() << "Error converting input to IR: " << toString(std::move(Err))
             << "\n";
      return 1;
    }

    assert(MRes.get() && "Could not load IR module");
    M = std::move(*MRes);
  }

  auto PassMachRes = driver.createPassMachinery();
  if (auto Err = PassMachRes.takeError()) {
    errs() << toString(std::move(Err)) << "\n";
    return 1;
  }

  std::unique_ptr<compiler::utils::PassMachinery> PassMach =
      std::move(*PassMachRes);

  if (PrintPasses) {
    PassMach->printPassNames(errs());
    return 0;
  }
  assert(M && "Could not load IR module");

  if (!PipelineText.empty()) {
    if (auto Err = driver.runPipeline(*M, *PassMach)) {
      errs() << toString(std::move(Err)) << "\n";
      return 1;
    }
  }

  if (FileType == multi_llvm::CodeGenFileType::Null) {
    return 0;
  }

  // Open the output file.
  std::error_code EC;
  sys::fs::OpenFlags OpenFlags = sys::fs::OF_None;
  if (FileType == multi_llvm::CodeGenFileType::AssemblyFile) {
    OpenFlags |= sys::fs::OF_Text;
  }
  auto Out = std::make_unique<ToolOutputFile>(OutputFilename, EC, OpenFlags);
  if (EC || !Out) {
    errs() << EC.message() << '\n';
    return 1;
  }

  llvm::ModulePassManager printMPM;
  if (FileType == multi_llvm::CodeGenFileType::AssemblyFile) {
    printMPM.addPass(llvm::PrintModulePass(Out->os()));
  } else {
    printMPM.addPass(llvm::BitcodeWriterPass(Out->os()));
  }
  printMPM.run(*M, PassMach->getMAM());
  Out->keep();

  return 0;
}

namespace muxc {

static uint32_t detectBuiltinCapabilities(mux_device_info_t device_info) {
  uint32_t caps = 0;
  if (device_info->address_capabilities & mux_address_capabilities_bits32) {
    caps |= compiler::CAPS_32BIT;
  }
  if (device_info->double_capabilities) {
    caps |= compiler::CAPS_FP64;
  }
  if (device_info->half_capabilities) {
    caps |= compiler::CAPS_FP16;
  }
  return caps;
}

void driver::parseArguments(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  if (ListDevices) {
    for (auto compiler : compiler::compilers()) {
      std::printf("%s\n", compiler->device_info->device_name);
    }
    std::exit(0);
  }
}

Error driver::setupContext() {
  CompilerContext = compiler::createContext();
  if (!CompilerContext) {
    return make_error<StringError>("Could not create compiler context",
                                   inconvertibleErrorCode());
  }

  // If the user hasn't asked for a device, we need a LLVMContext separate from
  // the 'compiler' machinery.
  if (DeviceName.empty() && DeviceIdx < 0) {
    LLVMCtx = std::make_unique<LLVMContext>();
    return Error::success();
  }

  auto InfoRes = findDevice();
  if (auto Err = InfoRes.takeError()) {
    return Err;
  }

  CompilerInfo = *InfoRes;

  CompilerTarget =
      CompilerInfo->createTarget(CompilerContext.get(), /*callback*/ nullptr);

  if (!CompilerTarget ||
      CompilerTarget->init(detectBuiltinCapabilities(
          CompilerInfo->device_info)) != compiler::Result::SUCCESS) {
    return make_error<StringError>("Could not create compiler target",
                                   inconvertibleErrorCode());
  }

  CompilerModule = CompilerTarget->createModule(ModuleNumErrors, ModuleLog);
  if (!CompilerModule || ModuleNumErrors) {
    return make_error<StringError>(
        formatv("Could not create compiler module:\n{0}\n", ModuleLog),
        inconvertibleErrorCode());
  }

  return Error::success();
}

static Expected<std::unique_ptr<Module>> parseIRFileToModule(LLVMContext &Ctx) {
  SMDiagnostic Err;
  if (auto M = parseIRFile(InputFilename, Err, Ctx)) {
    return std::move(M);
  }

  std::string ErrMsg;
  {
    raw_string_ostream ErrMsgStream(ErrMsg);
    Err.print(InputFilename.c_str(), ErrMsgStream);
  }
  return make_error<StringError>(std::move(ErrMsg), inconvertibleErrorCode());
}

Expected<std::unique_ptr<Module>> driver::convertInputToIR() {
  if (!InputLanguage.empty() && InputLanguage != "ir" &&
      InputLanguage != "cl") {
    return make_error<StringError>("input language must be '', 'ir' or 'cl'",
                                   inconvertibleErrorCode());
  }
  const StringRef IFN = InputFilename;

  auto *LLVMContextToUse =
      CompilerTarget
          ? &static_cast<compiler::BaseTarget *>(CompilerTarget.get())
                 ->getLLVMContext()
          : LLVMCtx.get();
  assert(LLVMContextToUse && "Missing LLVM Context");

  // Assume that .bc and .ll files are already IR unless told otherwise.
  if (InputLanguage == "ir" ||
      (InputLanguage.empty() &&
       (IFN.ends_with(".bc") || IFN.ends_with(".bc32") ||
        IFN.ends_with(".bc64") || IFN.ends_with(".ll")))) {
    return parseIRFileToModule(*LLVMContextToUse);
  }
  // Assume that stdin is IR unless told otherwise
  if (InputLanguage.empty() && IFN == "-") {
    return parseIRFileToModule(*LLVMContextToUse);
  }
  // Now we know we're in OpenCL mode; we need a known device.
  if (!CompilerModule) {
    return make_error<StringError>("A device must be set to compile OpenCL C",
                                   inconvertibleErrorCode());
  }

  // Parse any options
  if (CompilerModule->parseOptions(CLOptions,
                                   compiler::Options::Mode::COMPILE) !=
      compiler::Result::SUCCESS) {
    return make_error<StringError>("OpenCL C options parsing error",
                                   inconvertibleErrorCode());
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFileOrSTDIN(IFN, /*IsText=*/true);
  if (const std::error_code EC = FileOrErr.getError()) {
    return make_error<StringError>("Could not open input file: " + EC.message(),
                                   inconvertibleErrorCode());
  }
  auto SourceAsStr = FileOrErr.get()->getBuffer();
  auto *const BaseModule =
      static_cast<compiler::BaseModule *>(CompilerModule.get());
  clang::CompilerInstance instance;
  // We don't support profiles or headers
  auto M = BaseModule->compileOpenCLCToIR(instance, "FULL_PROFILE", SourceAsStr,
                                          /*input_headers*/ {});
  if (!M) {
    return make_error<StringError>("OpenCL C compilation error",
                                   inconvertibleErrorCode());
  }

  return std::move(M);
}

Expected<std::unique_ptr<compiler::utils::PassMachinery>>
driver::createPassMachinery() {
  std::unique_ptr<compiler::utils::PassMachinery> PassMach;

  if (CompilerTarget) {
    auto *const BaseModule =
        static_cast<compiler::BaseModule *>(CompilerModule.get());
    PassMach = BaseModule->createPassMachinery();
    // Forward on any frontend options we've parsed
    static_cast<compiler::BaseModulePassMachinery *>(PassMach.get())
        ->setCompilerOptions(BaseModule->getOptions());
  } else {
    compiler::utils::DeviceInfo Info(
        HalfCap ? compiler::utils::device_floating_point_capabilities_full : 0,
        FloatCap ? compiler::utils::device_floating_point_capabilities_full : 0,
        DoubleCap ? compiler::utils::device_floating_point_capabilities_full
                  : 0,
        /*max_work_width*/ 64);

    for (const auto S : SGSizes) {
      Info.reqd_sub_group_sizes.push_back(S);
    }

    auto &BaseCtx =
        *static_cast<compiler::BaseContext *>(CompilerContext.get());
    PassMach = std::make_unique<compiler::BaseModulePassMachinery>(
        *LLVMCtx, /*TM*/ nullptr, Info, /*BICallback*/ nullptr,
        BaseCtx.isLLVMVerifyEachEnabled(), BaseCtx.getLLVMDebugLoggingLevel(),
        BaseCtx.isLLVMTimePassesEnabled());
  }
  if (!PassMach) {
    return make_error<StringError>("Unable to create Passmachinery",
                                   inconvertibleErrorCode());
  }

  // Initialize the pass machinery.
  PassMach->initializeStart();
  PassMach->initializeFinish();

  return std::move(PassMach);
}

Error driver::runPipeline(Module &M, compiler::utils::PassMachinery &PassMach) {
  ModulePassManager pm;
  if (auto Err = PassMach.getPB().parsePassPipeline(pm, PipelineText)) {
    const std::string Msg =
        formatv("error: parse of pass pipeline '{0}' failed : {1}\n",
                PipelineText, toString(std::move(Err)));
    return make_error<StringError>(std::move(Msg), inconvertibleErrorCode());
  }

  pm.run(M, PassMach.getMAM());
  return Error::success();
}

Expected<const compiler::Info *> driver::findDevice() {
  auto compilers = compiler::compilers();
  if (compilers.empty()) {
    return make_error<StringError>("Error: no compilers found",
                                   inconvertibleErrorCode());
  }

  auto printMuxCompilers =
      [](cargo::array_view<const compiler::Info *> compilers) {
        std::string device_list;
        for (unsigned int i = 0, n = compilers.size(); i < n; ++i) {
          device_list += formatv("device {0}: {1}\n", i,
                                 compilers[i]->device_info->device_name);
        }
        return device_list;
      };

  // Device idx takes precedent.
  if (DeviceIdx >= 0) {
    if (static_cast<size_t>(DeviceIdx) >= compilers.size()) {
      return make_error<StringError>(
          formatv("Error: invalid device selection; out of bounds. Available "
                  "devices:\n{0}",
                  printMuxCompilers(compilers)),
          inconvertibleErrorCode());
    }
    return compilers[DeviceIdx];
  }

  const compiler::Info *info = nullptr;
  for (auto compiler : compilers) {
    bool matches = true;
    matches &=
        (std::string(compiler->device_info->device_name).find(DeviceName) !=
         cargo::string_view::npos);
    if (matches) {
      if (info) {
        return make_error<StringError>(
            formatv(
                "Error: device selection ambiguous. Available devices:\n{0}",
                printMuxCompilers(compilers)),
            inconvertibleErrorCode());
      }
      info = compiler;
    }
  }
  if (!info) {
    return make_error<StringError>(formatv("Error: no device matched the given "
                                           "substring. Available devices:\n{0}",
                                           printMuxCompilers(compilers)),
                                   inconvertibleErrorCode());
  }

  return info;
}

}  // namespace muxc
