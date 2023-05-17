// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef COMPILER_TOOLS_MUXC_H_INCLUDED
#define COMPILER_TOOLS_MUXC_H_INCLUDED

#include <base/base_pass_machinery.h>
#include <base/context.h>
#include <compiler/target.h>
#include <mux/mux.hpp>

namespace muxc {

/// @brief Function return status
enum result : int { success = 0, failure = 1 };

/// @brief Parses pipelines to run LLVM passes provided from the target's Mux
/// Compiler
class driver {
 public:
  /// @brief Default constructor.
  driver();

  /// @brief Loads any arguments from command-line.
  void parseArguments(int argc, char **argv);

  /// @brief Initializes the compiler context
  ///
  /// @return Returns a `muxc::result`.
  result createContext();

  /// @brief Initializes the right platform and device
  ///
  /// @return Returns a `muxc::result`.
  result setupContext();

  /// @brief Create the pass machinery
  ///
  /// @return Returns a `PassMachinery` in a `unique_ptr`.
  std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery();

  /// @brief Run the pass pipeline provided
  ///
  /// @return Returns a `muxc::result`.
  result runPipeline(compiler::utils::PassMachinery *);

 private:
  /// @brief Selected compiler.
  const compiler::Info *CompilerInfo;
  /// @brief Compiler context to drive compilation.
  std::unique_ptr<compiler::Context> CompilerContext;
  /// @brief Compiler target to drive compilation.
  std::unique_ptr<compiler::Target> CompilerTarget;
  /// @brief Compiler module being compiled.
  std::unique_ptr<compiler::Module> CompilerModule;

  /// @brief Find the desired `compiler::Info` from `device_name_substring`.
  ///
  /// @return Returns a `muxc::result`.
  result findDevice();
};

}  // namespace muxc

#endif  // COMPILER_TOOLS_MUXC_H_INCLUDED
