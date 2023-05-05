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

/// @file
///
/// Host's interface to perf for host profiling

#ifndef HOST_PERF_SUPPORT_H_INCLUDED
#define HOST_PERF_SUPPORT_H_INCLUDED

#if defined(__linux__)

#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace host {
/// @brief Support for Linux perf by using llvm::ObjectCache
class PerfInterface final : public llvm::ObjectCache {
  using cacheType = std::map<std::string, llvm::MemoryBufferRef>;
  std::string filename;
  std::ofstream perf_fstream;
  std::unique_ptr<llvm::raw_fd_ostream> obj_file;
  std::mutex lock;
  cacheType mem_cache;
  bool enable;

  std::unique_ptr<llvm::MemoryBufferRef> getObjectFromCache(
      const std::string &);
  std::unique_ptr<llvm::MemoryBuffer> getObjectBuffer(const std::string &);

 public:
  /// @brief Setting up ObjectCache Map and files required by linux perf tool
  PerfInterface(const std::string &);

  /// @brief Close all temporary files created during JIT'ing Machine code on
  /// host
  ~PerfInterface();

  /// @brief Write symbols that have just been JIT'ed into the "map" file.
  ///        This map file will be used to match symbols to against an executed
  ///        instruction
  ///
  /// @param module  Name of the module - used to look up machine-code for a
  /// module
  /// @param symbol  Symbol within module to write into map file
  /// @param address Final location of code to be executed from MCJIT engine
  void writePerfSymbolFile(const std::string &module, std::string symbol,
                           uint64_t address);

  /// @brief helper function to check if perf profiling has been enabled
  bool is_enabled() { return enable; }

  /// @brief Derived function called by MCJIT before compilation to check if
  /// compiled
  ///        object has been cached and can be returned
  ///
  /// @param module Module passed in by MCJIT
  ///
  /// @return if a nullptr is returned, MCJIT will compile accummulated code
  ///         otherwise, it will assume the returned buffer contains executable
  ///         code
  virtual std::unique_ptr<llvm::MemoryBuffer> getObject(
      const llvm::Module *module) override;

  /// @brief Derived function called by MCJIT post compilation. This function is
  /// used to
  ///        store compiled code, until a required symbol's attributes can be
  ///        extracted in a format useful for the linux perf tool
  ///
  /// @param module Module passed in by MCJIT
  /// @param obj    Compiled kernel code emitted by MCJIT
  virtual void notifyObjectCompiled(const llvm::Module *module,
                                    const llvm::MemoryBufferRef obj) override;
};

}  // namespace host

#endif  // defined(__linux__)
#endif  // HOST_PERF_SUPPORT_H_INCLUDED
