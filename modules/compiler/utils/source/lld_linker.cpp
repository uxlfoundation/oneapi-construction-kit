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

#if LLVM_VERSION_GREATER_EQUAL(17, 0)
LLD_HAS_DRIVER(elf)
#endif

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
  SmallString<128> objFileName;
  SmallString<128> elfFileName;
  SmallString<128> linkerScriptFileName;
  SmallString<128> linkRTFileName;
  sys::fs::createTemporaryFile("lld", "o", objFileName);
  sys::fs::createTemporaryFile("lld", "elf", elfFileName);
  sys::fs::createTemporaryFile("lld", "ld", linkerScriptFileName);
  if (linkerLib) {
    sys::fs::createTemporaryFile("lld_rt", "a", linkRTFileName);
    FILE *flinklib = fopen(linkRTFileName.data(), "wb+");
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
  FILE *f = fopen(objFileName.data(), "wb+");
  if (!f) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to open temporary object file");
  }
  if (fwrite(rawBinary.data(), 1, rawBinary.size(), f) != rawBinary.size()) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to write binary to temporary object file");
  }
  if (fclose(f) != 0) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to close temporary object file");
  }

  FILE *fl = fopen(linkerScriptFileName.data(), "w+");
  if (!fl) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to open temporary linker script file");
  }
  if (fwrite(linkerScriptStr.c_str(), 1, linkerScriptStr.length(), fl) !=
      linkerScriptStr.length()) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to write to temporary linker script file");
  }
  if (fclose(fl) != 0) {
    return createStringError(inconvertibleErrorCode(),
                             "unable to close temporary linker script file");
  }

  std::vector<std::string> args = {
      "ld.lld",
      objFileName.data(),
  };

#if !defined(NDEBUG) || defined(CA_ENABLE_LLVM_OPTIONS_IN_RELEASE)
  if (auto *env = std::getenv("CA_LLVM_OPTIONS")) {
    auto split_llvm_options = cargo::split(env, " ");
    compiler::utils::appendMLLVMOptions(split_llvm_options, args);
  }
#endif  // NDEBUG

  for (const auto &arg : additionalLinkArgs) {
    args.push_back(arg);
  }
  args.push_back((Twine("--script=") + linkerScriptFileName).str());
  if (linkerLib) {
    args.push_back(linkRTFileName.data());
  }
  args.push_back("-o");
  args.push_back(elfFileName.data());

  std::vector<const char *> lld_args(args.size());
  for (unsigned i = 0, e = lld_args.size(); i != e; i++) {
    lld_args[i] = args[i].c_str();
  }

  std::string stderrStr;
  raw_string_ostream stderrOS(stderrStr);
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
  const ::lld::Result s = ::lld::lldMain(lld_args, outs(), stderrOS,
                                         {{::lld::Gnu, &lld::elf::link}});
  const bool linkResult = !s.retCode && s.canRunAgain;
  ::lld::CommonLinkerContext::destroy();
#else
  const bool linkResult =
      lld::elf::link(lld_args, outs(), stderrOS,
                     /*exitEarly*/ false, /*disableOutput*/ false);
  lld::CommonLinkerContext::destroy();
#endif

  if (linkerLib) {
    sys::fs::remove(linkRTFileName);
  }
  sys::fs::remove(linkerScriptFileName);
  sys::fs::remove(objFileName);
  if (!linkResult) {
    return createStringError(inconvertibleErrorCode(), stderrStr.c_str());
  }

  // Read the output file size
  auto bufferOrError = MemoryBuffer::getFile(elfFileName.c_str());

  sys::fs::remove(elfFileName);

  if (!bufferOrError) {
    return errorCodeToError(bufferOrError.getError());
  }

  return std::move(*bufferOrError);
}

}  // namespace utils
}  // namespace compiler
