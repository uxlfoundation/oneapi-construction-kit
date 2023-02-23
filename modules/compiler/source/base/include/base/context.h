// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_BASE_CONTEXT_H_INCLUDED
#define COMPILER_BASE_CONTEXT_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/string_view.h>
#include <compiler/context.h>
#include <compiler/utils/pass_machinery.h>
#include <mux/mux.h>
// LLVMContext lives inside a unique_ptr, so we need the header here
#include <llvm/IR/LLVMContext.h>

#include <memory>
#include <mutex>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Compiler context implementation.
class BaseContext : public Context {
 public:
  BaseContext();
  ~BaseContext();

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  BaseContext(BaseContext &&) = delete;

  /// @brief Checks if a binary stream is valid SPIR.
  ///
  /// @param binary View of the SPIR binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  bool isValidSPIR(cargo::array_view<const std::uint8_t> binary) override;

  /// @brief Checks if a binary stream is valid SPIR-V.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  bool isValidSPIRV(cargo::array_view<const uint32_t> code) override;

  /// @brief Get a description of all of a SPIR-V modules specializable
  /// constants.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns a map of the modules specializable constants on success,
  /// otherwise returns an error string.
  cargo::expected<spirv::SpecializableConstantsMap, std::string>
  getSpecializableConstants(cargo::array_view<const uint32_t> code) override;

  /// @brief Locks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::lock`.
  void lock() override;

  /// @brief Attempts to acquire the lock on the underlying mutex, used to
  /// control access to the underlying LLVM context.
  ///
  /// Forwards to `std::mutex::try_lock`.
  ///
  /// @return Returns true if the lock was acquired, false otherwise.
  bool try_lock() override;

  /// @brief Unlocks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::unlock`.
  void unlock() override;

  /// @brief LLVM context.
  llvm::LLVMContext llvm_context;

  bool isLLVMVerifyEachEnabled() const { return llvm_verify_each; }

  bool isLLVMTimePassesEnabled() const { return llvm_time_passes; }

  compiler::utils::DebugLogging getLLVMDebugLoggingLevel() const {
    return llvm_debug_passes;
  }

 private:
  /// @brief Mutex for accessing the LLVMContext.
  std::mutex llvm_mutex;

  /// @brief True if compiler passes should be individually verified.
  ///
  /// If false, the default is to verify before/after each pass pipeline.
  bool llvm_verify_each = false;

  /// @brief True if compiler passes should be individually timed, with a
  /// summary reported for each pipeline.
  bool llvm_time_passes = false;

  /// @brief Debug logging level used with compiler passes.
  compiler::utils::DebugLogging llvm_debug_passes =
      compiler::utils::DebugLogging::None;

};  // class ContextImpl
/// @}
}  // namespace compiler

#endif  // COMPILER_BASE_CONTEXT_H_INCLUDED
