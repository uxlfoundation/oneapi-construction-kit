// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "muxc.h"

#include <base/base_pass_machinery.h>
#include <base/module.h>
#include <compiler/library.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/ToolOutputFile.h>
#include <multi_llvm/multi_llvm.h>

// Additional arguments beyond standard llvm command options
static llvm::cl::opt<std::string> InputFilename(
    "input", llvm::cl::Positional, llvm::cl::desc("<input .bc or .ll file>"),
    llvm::cl::init("-"));

static llvm::cl::opt<std::string> PipelineText(
    "passes", llvm::cl::desc("pipeline to run, passes separated by ','"),
    llvm::cl::init(""));

static llvm::cl::opt<std::string> OutputFilename(
    "o", llvm::cl::desc("Override output filename (default stdout)"),
    llvm::cl::value_desc("filename"), llvm::cl::init("-"));

static llvm::cl::opt<bool> ListDevices("list-devices",
                                       llvm::cl::desc("list devices"),
                                       llvm::cl::value_desc("list-devices"));

static llvm::cl::opt<bool, false> WriteTextual(
    "S", llvm::cl::desc("Write module as text"));

static llvm::cl::opt<std::string> DeviceName("device",
                                             llvm::cl::desc("select device"),
                                             llvm::cl::value_desc("name"));

static llvm::cl::opt<bool> PrintPasses(
    "print-passes",
    llvm::cl::desc("Print available passes that can be specified in "
                   "--passes=foo and exit (not including LLVM ones)"));

static llvm::cl::opt<bool> HalfCap(
    "device-fp16-capabilities",
    llvm::cl::desc("Enable/Disable device fp16 capabilities"),
    llvm::cl::init(true));
static llvm::cl::opt<bool> FloatCap(
    "device-fp32-capabilities",
    llvm::cl::desc("Enable/Disable device fp32 capabilities"),
    llvm::cl::init(true));
static llvm::cl::opt<bool> DoubleCap(
    "device-fp64-capabilities",
    llvm::cl::desc("Enable/Disable device fp64 capabilities"),
    llvm::cl::init(true));

int main(int argc, char **argv) {
  muxc::driver driver;
  driver.parseArguments(argc, argv);

  // setupContext is only needed if we have a device
  if (!DeviceName.empty()) {
    if (driver.setupContext()) {
      return 1;
    }
  }

  auto PassMach = driver.createPassMachinery();
  if (!PassMach) {
    return 1;
  }

  if (PrintPasses) {
    PassMach->printPassNames(llvm::errs());
    return 0;
  }

  if (driver.runPipeline(PassMach.get())) {
    return 1;
  }

  return 0;
}

namespace muxc {

void mux_message(const char *message, const void *, size_t) {
  std::fprintf(stderr, "%s\n", message);
}

bool matchSubstring(cargo::string_view big_string, cargo::string_view filter) {
  return big_string.find(filter) != cargo::string_view::npos;
}

void printMuxCompilers(cargo::array_view<const compiler::Info *> compilers) {
  for (unsigned int i = 0, n = compilers.size(); i < n; ++i) {
    std::fprintf(stderr, "device %u: %s\n", i + 1,
                 compilers[i]->device_info->device_name);
  }
}

uint32_t detectBuiltinCapabilities(mux_device_info_t device_info) {
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

driver::driver()
    : CompilerInfo(nullptr),
      CompilerContext(compiler::createContext()),
      Module(nullptr) {}

void driver::parseArguments(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  if (ListDevices) {
    for (auto compiler : compiler::compilers()) {
      std::printf("%s\n", compiler->device_info->device_name);
    }
    std::exit(0);
  }
}

result driver::setupContext() {
  if (findDevice() == result::failure || !CompilerContext) {
    return result::failure;
  }

  CompilerTarget =
      CompilerInfo->createTarget(CompilerContext.get(), mux_message);
  if (!CompilerTarget ||
      CompilerTarget->init(detectBuiltinCapabilities(
          CompilerInfo->device_info)) != compiler::Result::SUCCESS) {
    std::fprintf(stderr, "Error: Could not create compiler target\n");
    return result::failure;
  }

  return result::success;
}

std::unique_ptr<compiler::utils::PassMachinery> driver::createPassMachinery() {
  uint32_t ModuleNumErrors = 0;
  std::string ModuleLog{};

  if (CompilerTarget) {
    Module = CompilerTarget->createModule(ModuleNumErrors, ModuleLog);
    if (!Module || ModuleNumErrors) {
      std::fprintf(stderr, "Error: Could not create compiler module :\n%s\n",
                   ModuleLog.c_str());
      return {};
    }
  }

  std::unique_ptr<compiler::utils::PassMachinery> PassMach;
  if (CompilerTarget) {
    auto *const BaseModule = static_cast<compiler::BaseModule *>(Module.get());
    PassMach = BaseModule->createPassMachinery();
  } else {
    compiler::utils::DeviceInfo Info(
        HalfCap ? compiler::utils::device_floating_point_capabilities_full : 0,
        FloatCap ? compiler::utils::device_floating_point_capabilities_full : 0,
        DoubleCap ? compiler::utils::device_floating_point_capabilities_full
                  : 0,
        64);

    auto &BaseCtx =
        *static_cast<compiler::BaseContext *>(CompilerContext.get());
    PassMach = multi_llvm::make_unique<compiler::BaseModulePassMachinery>(
        /*TM*/ nullptr, Info, /*BICallback*/ nullptr,
        BaseCtx.isLLVMVerifyEachEnabled(), BaseCtx.getLLVMDebugLoggingLevel(),
        BaseCtx.isLLVMTimePassesEnabled());
  }
  if (PassMach) {
    PassMach->initializeStart();
    PassMach->initializeFinish();
  } else {
    std::fprintf(stderr, "Error: Unable to make PassMachinery\n");
  }

  return PassMach;
}

result driver::runPipeline(compiler::utils::PassMachinery *PassMach) {
  llvm::LLVMContext Context;
#if LLVM_VERSION_GREATER_EQUAL(15, 0)
  Context.setOpaquePointers(true);
#endif
  llvm::ModulePassManager pm;
  if (auto Err = PassMach->getPB().parsePassPipeline(pm, PipelineText)) {
    llvm::errs() << llvm::formatv("Error: Parse of {0} failed : {1}\n",
                                  PipelineText, llvm::toString(std::move(Err)));
    return result::failure;
  }

  llvm::SMDiagnostic Err;

  auto &ContextChoice =
      static_cast<compiler::BaseContext *>(CompilerContext.get())->llvm_context;

  std::unique_ptr<llvm::Module> Mod =
      llvm::parseIRFile(InputFilename, Err, ContextChoice);

  if (!Mod) {
    Err.print(InputFilename.c_str(), llvm::errs());
    return result::failure;
  }
  // Open the output file.
  std::error_code EC;
  llvm::sys::fs::OpenFlags OpenFlags = llvm::sys::fs::OF_None;
  if (WriteTextual) {
    OpenFlags |= llvm::sys::fs::OF_Text;
  }
  auto Out = multi_llvm::make_unique<llvm::ToolOutputFile>(OutputFilename, EC,
                                                           OpenFlags);
  if (EC || !Out) {
    llvm::errs() << EC.message() << '\n';
    return result::failure;
  }

  pm.run(*Mod.get(), PassMach->getMAM());
  if (WriteTextual) {
    Out->os() << *Mod.get();
  } else {
    llvm::WriteBitcodeToFile(*Mod.get(), Out->os());
  }
  Out->keep();

  return result::success;
}  // namespace muxc

result driver::findDevice() {
  auto compilers = compiler::compilers();
  if (compilers.empty()) {
    std::fprintf(stderr, "Error: no compilers found\n");
    return result::failure;
  }

  bool found = false;
  for (auto compiler : compilers) {
    bool matches = true;
    matches &=
        (std::string(compiler->device_info->device_name).find(DeviceName) !=
         cargo::string_view::npos);
    if (matches) {
      if (found) {
        std::fprintf(stderr,
                     "Error: Device selection ambiguous, available devices:\n");
        printMuxCompilers(compilers);
        return result::failure;
      }
      found = true;
      CompilerInfo = compiler;
    }
  }
  if (!found) {
    std::fprintf(
        stderr,
        "Error: No device matched the given substring, available devices:\n");
    printMuxCompilers(compilers);
    return result::failure;
  }

  return result::success;
}

}  // namespace muxc
