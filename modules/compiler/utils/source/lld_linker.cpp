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

#include <cargo/string_algorithm.h>
#include <compiler/utils/lld_linker.h>
#include <lld/Common/CommonLinkerContext.h>
#include <lld/Common/Driver.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

LLD_HAS_DRIVER(elf)

namespace compiler {
namespace utils {

void appendMLLVMOptions(cargo::array_view<cargo::string_view> options,
                        std::vector<std::string> &lld_args) {
  for (unsigned i = 0, e = options.size(); i != e; i++) {
    const auto &opt = options[i];
    lld_args.push_back("-mllvm");
    lld_args.push_back(std::string(opt.data(), opt.size()));
    // If the next 'option' is in fact a separated option value, glue it to the
    // previous option:
    //   {"--foo", "x"}  -> {"-mllvm", "--foo=x"}
    if (i + 1 < e) {
      const auto &next_opt = options[i + 1];
      if (!next_opt.empty() && next_opt[0] != '-') {
        i++;
        lld_args.back().append("=");
        lld_args.back().append(std::string(next_opt.data(), next_opt.size()));
      }
    }
  }
}

Expected<std::unique_ptr<MemoryBuffer>> lldLinkToBinary(
    const ArrayRef<uint8_t> rawBinary, const std::string &linkerScriptStr,
    const uint8_t *linkerLib, unsigned int linkerLibBytes,
    const SmallVectorImpl<std::string> &additionalLinkArgs) {
  struct TemporaryFile {
    TemporaryFile() = default;
    TemporaryFile(const Twine &Prefix, StringRef Suffix) {
      ErrorCode = sys::fs::createTemporaryFile(Prefix, Suffix, FileName);
    }
    TemporaryFile(const TemporaryFile &) = delete;
    TemporaryFile(TemporaryFile &&other) {
      std::swap(FileName, other.FileName);
      std::swap(ErrorCode, other.ErrorCode);
    }
    ~TemporaryFile() {
      if (!getErrorCode() && FileName.size()) {
        (void)sys::fs::remove(FileName);
      }
    }
    TemporaryFile &operator=(const TemporaryFile &) = delete;
    TemporaryFile &operator=(TemporaryFile &&other) {
      std::swap(FileName, other.FileName);
      std::swap(ErrorCode, other.ErrorCode);
      return *this;
    }
    const char *getFileName() const { return FileName.c_str(); }
    std::error_code getErrorCode() const { return ErrorCode; }
    explicit operator bool() const { return bool(ErrorCode); }

   private:
    // mutable in order to allow calling c_str().
    mutable SmallString<128> FileName;
    std::error_code ErrorCode;
  };

  const TemporaryFile objFile("lld", "o");
  if (objFile) return errorCodeToError(objFile.getErrorCode());
  const TemporaryFile elfFile("lld", "elf");
  if (elfFile) return errorCodeToError(elfFile.getErrorCode());
  const TemporaryFile linkerScript("lld", "ld");
  if (linkerScript) return errorCodeToError(linkerScript.getErrorCode());
  TemporaryFile linkRTFile;
  if (linkerLib) {
    linkRTFile = TemporaryFile("lld_rt", "a");
    if (linkRTFile) return errorCodeToError(linkRTFile.getErrorCode());
    FILE *flinklib = fopen(linkRTFile.getFileName(), "wb+");
    if (nullptr != flinklib) {
      if (fwrite(linkerLib, 1, linkerLibBytes, flinklib) != linkerLibBytes) {
        return createStringError(
            inconvertibleErrorCode(),
            "unable to write linker lib to temporary file");
      }
      if (fclose(flinklib) != 0) {
        return createStringError(inconvertibleErrorCode(),
                                 "unable to close temporary linker lib file");
      }
    }
  }
  FILE *f = fopen(objFile.getFileName(), "wb+");
  if (!f) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to open temporary object file");
  }
  if (fwrite(rawBinary.data(), 1, rawBinary.size(), f) != rawBinary.size()) {
    (void)fclose(f);
    return createStringError(inconvertibleErrorCode(),
                             "unable to write binary to temporary object file");
  }
  if (fclose(f) != 0) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to close temporary object file");
  }

  FILE *fl = fopen(linkerScript.getFileName(), "w+");
  if (!fl) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to open temporary linker script file");
  }
  if (fwrite(linkerScriptStr.c_str(), 1, linkerScriptStr.length(), fl) !=
      linkerScriptStr.length()) {
    (void)fclose(fl);
    return createStringError(inconvertibleErrorCode(),
                             "unable to write to temporary linker script file");
  }
  if (fclose(fl) != 0) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to close temporary linker script file");
  }

  std::vector<std::string> args = {"ld.lld", objFile.getFileName()};

#if !defined(NDEBUG) || defined(CA_ENABLE_LLVM_OPTIONS_IN_RELEASE) || \
    defined(CA_ENABLE_DEBUG_SUPPORT)
  if (auto *env = std::getenv("CA_LLVM_OPTIONS")) {
    auto split_llvm_options = cargo::split(env, " ");
    compiler::utils::appendMLLVMOptions(split_llvm_options, args);
  }
#endif  // NDEBUG

#if LLVM_VERSION_LESS(20, 0)
  // LLVM's register allocator has issues with early-clobbers if subreg liveness
  // is enabled. The InitUndef pass documents this and attempts to work around
  // it, but prior to <https://github.com/llvm/llvm-project/pull/90967>, the
  // InitUndef pass would not work reliably when multiple functions were
  // processed, because internal state from one function would be kept around
  // when processing the next. As we have no good way of fixing the InitUndef
  // pass in older LLVM versions, disable subreg liveness instead.
  args.push_back("-mllvm");
  args.push_back("-enable-subreg-liveness=false");
#endif

  for (const auto &arg : additionalLinkArgs) {
    args.push_back(arg);
  }
  args.push_back((Twine("--script=") + linkerScript.getFileName()).str());
  if (linkerLib) {
    args.push_back(linkRTFile.getFileName());
  }
  args.push_back("-o");
  args.push_back(elfFile.getFileName());

  std::vector<const char *> lld_args(args.size());
  for (unsigned i = 0, e = lld_args.size(); i != e; i++) {
    lld_args[i] = args[i].c_str();
  }

  std::string stderrStr;
  raw_string_ostream stderrOS(stderrStr);
  const ::lld::Result s = ::lld::lldMain(lld_args, outs(), stderrOS,
                                         {{::lld::Gnu, &lld::elf::link}});
  const bool linkResult = !s.retCode && s.canRunAgain;
  ::lld::CommonLinkerContext::destroy();

  if (!linkResult) {
    return createStringError(inconvertibleErrorCode(), stderrStr.c_str());
  }

  // Read the output file size
  auto bufferOrError = MemoryBuffer::getFile(elfFile.getFileName());

  if (!bufferOrError) {
    return errorCodeToError(bufferOrError.getError());
  }

  return std::move(*bufferOrError);
}

}  // namespace utils
}  // namespace compiler
