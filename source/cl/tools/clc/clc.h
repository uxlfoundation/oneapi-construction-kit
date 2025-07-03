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
#ifndef CL_TOOLS_CLC_H_INCLUDED
#define CL_TOOLS_CLC_H_INCLUDED

#include <cargo/expected.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>
#include <cl/binary/binary.h>
#include <compiler/library.h>

#include <map>
#include <vector>

namespace clc {

/// @brief Function return status
enum result : int { success = 0, failure = 1 };

/// @brief Input type.
enum class input_type { opencl_c, spirv, spir };

/// @brief Compiles OpenCL kernels.
class driver {
 public:
  /// @brief Default constructor.
  driver();

  /// @brief Loads any arguments from command-line.
  result parseArguments(int argc, char **argv);

  /// @brief Initializes the right platform and device and creates a context.
  ///
  /// @return Returns a `clc::result`.
  result setupContext();

  /// @brief Loads and compiles the given input program.
  ///
  /// @return Returns a `clc::result`.
  result buildProgram();

  /// @brief Writes the binary obtained from compilation to the output file.
  ///
  /// @return Returns a `clc::result`.
  result saveBinary();

  /// @brief Requests additional information to `stderr` during the runtime of
  /// the program.
  bool verbose;
  /// @brief Prevents `saveBinary()` from actually writing the output data.
  bool dry_run;
  /// @brief The path to the input source code, or `"-"` for `stdin`.
  std::string input_file;
  /// @brief Path to the output file or `"-"` for `stdout`.
  cargo::string_view output_file;
  /// @brief Device index to select from multiple devices. Takes precedence over
  /// device name.
  size_t device_idx;
  /// @brief Device name substring to select from multiple devices.
  cargo::string_view device_name_substring;
  /// @brief List of compile options passed to `clBuildProgram`.
  std::vector<std::string> cl_build_args;
  /// @brief Strips the header containing argument and kernel count
  /// informations.
  bool strip_binary_header;

 private:
  /// @brief Selected compiler.
  const compiler::Info *compiler_info;
  /// @brief Compiler context to drive compilation.
  std::unique_ptr<compiler::Context> context;
  /// @brief Compiler target to drive compilation.
  std::unique_ptr<compiler::Target> compiler_target;
  /// @brief Compiler module being compiled.
  std::unique_ptr<compiler::Module> module;
  /// @brief Printf calls descriptors generated during module finalization.
  std::vector<builtins::printf::descriptor> printf_calls;
  /// @brief Program information generated during module finalization.
  compiler::ProgramInfo program_info;
  /// @brief Storage for the compiled binary.
  std::string generated_output_file;
  /// @brief Build log for the compiler Module
  std::string module_log;

  /// @brief Find the desired `compiler::Info` from `device_name_substring`.
  ///
  /// @return Returns a `clc::result`.
  result findDevice();
};

}  // namespace clc

#endif  // CL_TOOLS_CLC_H_INCLUDED
