// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler context API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/expected.h>

#include <mutex>
#include <unordered_map>

namespace compiler {
/// @addtogroup compiler
/// @{

namespace spirv {
/// @brief Enumeration of SPIR-V constant types which can be specialized.
enum class SpecializationType : uint8_t {
  BOOL,   ///< OpTypeBool specialization constant.
  INT,    ///< OpTypeInt specialization constant.
  FLOAT,  ///< OpTypeFloat specialization constant.
};

/// @brief Description of a SPIR-V constant which can be specialized.
struct SpecializationDesc {
  /// @brief Type of the specializable constant.
  SpecializationType constant_type;
  /// @brief Size in bits of the specializable constant.
  uint32_t size_in_bits;
};

/// @brief Type for mapping SPIR-V specializaton constant ID to it's
/// description.
using SpecializableConstantsMap =
    std::unordered_map<std::uint32_t, SpecializationDesc>;
}  // namespace spirv

/// @brief Compiler context class.
///
/// Satisfies the C++ named requirement 'Lockable' so it can be used as part
/// of std::lock_guard or std::unique_lock.
class Context {
 public:
  /// @brief Virtual destructor.
  virtual ~Context() = default;

  /// @brief Checks if a binary stream is valid SPIR.
  ///
  /// @param binary View of the SPIR binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  virtual bool isValidSPIR(cargo::array_view<const std::uint8_t> binary) = 0;

  /// @brief Checks if a binary stream is valid SPIR-V.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  virtual bool isValidSPIRV(cargo::array_view<const uint32_t> code) = 0;

  /// @brief Get a description of all of a SPIR-V modules specializable
  /// constants.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns a map of the modules specializable constants on success,
  /// otherwise returns an error string.
  virtual cargo::expected<spirv::SpecializableConstantsMap, std::string>
  getSpecializableConstants(cargo::array_view<const uint32_t> code) = 0;

  /// @brief Locks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::lock`.
  virtual void lock() = 0;

  /// @brief Attempts to acquire the lock on the underlying mutex, used to
  /// control access to the underlying LLVM context.
  ///
  /// Forwards to `std::mutex::try_lock`.
  ///
  /// @return Returns true if the lock was acquired, false otherwise.
  virtual bool try_lock() = 0;

  /// @brief Unlocks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::unlock`.
  virtual void unlock() = 0;
};  // class Context
/// @}
}  // namespace compiler

#endif  // COMPILER_CONTEXT_H_INCLUDED
