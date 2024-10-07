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

#ifndef SPIRV_LL_SPV_CONTEXT_H_INCLUDED
#define SPIRV_LL_SPV_CONTEXT_H_INCLUDED

#include <cargo/expected.h>
#include <cargo/optional.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <spirv-ll/assert.h>
#include <spirv/unified1/spirv.hpp>

#include <string>
#include <type_traits>

namespace llvm {
class LLVMContext;
}  // namespace llvm

namespace spirv_ll {
/// @addtogroup spirv-ll
/// @{

class Module;

/// @brief Information about the target device to used during translation.
struct DeviceInfo {
  /// @brief List of supported capabilities.
  llvm::SmallVector<spv::Capability, 64> capabilities;
  /// @brief List of supported extensions.
  llvm::SmallVector<std::string, 8> extensions;
  /// @brief List of supported extended instruction sets.
  llvm::SmallVector<std::string, 2> extInstImports;
  /// @brief Supported addressing model.
  spv::AddressingModel addressingModel;
  /// @brief Supported memory model.
  spv::MemoryModel memoryModel;
  /// @brief Size of a device memory address in bits (Vulkan only).
  uint32_t addressBits;
};

/// @brief Information about a SPIR-V translation error.
struct Error {
  /// @brief Construct an error message.
  ///
  /// @param message Message to report to the user.
  Error(const std::string &message) : message("error: " + message) {}

  /// @brief SPIR-V translation error message.
  std::string message;
};

/// @brief Enumeration of constant types which can be specialized.
enum class SpecializationType : uint8_t {
  BOOL,   ///< OpTypeBool specialization constant.
  INT,    ///< OpTypeInt specialization constant.
  FLOAT,  ///< OpTypeFloat specialization constant.
};

/// @brief Description of a constant which can be specialized.
struct SpecializationDesc {
  /// @brief Type of the specializable constant.
  SpecializationType constantType;
  /// @brief Size in bits of the specializable constant.
  uint32_t sizeInBits;
};

/// @brief Type for mapping specializaton constant ID to it's description.
using SpecializableConstantsMap =
    llvm::DenseMap<spv::Id, spirv_ll::SpecializationDesc>;

/// @brief Information about constants to be specialized.
struct SpecializationInfo {
  /// @brief A specialization constant mapping to `data`.
  struct Entry {
    /// @brief Offset in bytes into `data`.
    uint32_t offset;
    /// @brief Size of the type pointed to at `offset` into `data`.
    size_t size;
  };

  /// @brief Check if the given ID is to be specialized.
  ///
  /// @param id The SPIR-V OpCode ID to be specialized.
  ///
  /// @return Returns true if `id` has a specialization, false otherwise.
  bool isSpecialized(spv::Id id) const { return entries.count(id) != 0; }

  /// @brief Get the specialization constant value for the given ID.
  ///
  /// @tparam Number Type of the specialization value.
  /// @param id The SPIR-V OpCode ID to be specialized.
  ///
  /// @return Returns the expected specialization value unless there is a size
  /// mismatch, then an error is reported.
  template <class Number>
  cargo::expected<Number, Error> getValue(spv::Id id) const {
    static_assert(std::is_arithmetic_v<Number>,
                  "Number must be an arithmetic type");
    SPIRV_LL_ASSERT(isSpecialized(id),
                    ("no specialization for " + std::to_string(id)).c_str());
    const auto &entry = entries.lookup(id);
    if (sizeof(Number) != entry.size) {
      return cargo::make_unexpected(
          Error{"Number size does not match entries[" + std::to_string(id) +
                "].size "});
    }
    Number value;
    std::memcpy(&value, static_cast<const uint8_t *>(data) + entry.offset,
                entry.size);
    return value;
  }

  /// @brief Map of ID to offset to `data`.
  llvm::DenseMap<spv::Id, Entry> entries;
  /// @brief Buffer containing constant values to specialize.
  const void *data;
};

/// @brief Class holding the SPIR-V context information, such as the types
///
/// This class is similar to the LLVM context class. It holds the types and
/// values defined, as well as their matching LLVM Types and Values.
class Context {
 public:
  /// @brief Default construct the SPIR-V translation context.
  ///
  /// This constructor creates an `llvm::LLVMContext` which is owned by the
  /// `spirv_ll::Context`.
  Context();

  /// @brief Construct the SPIR-V translation context.
  ///
  /// This constructor uses the given `llvm::LLVMContext` but does not own it.
  ///
  /// @param[in] llvmContext The LLVM context to use.
  Context(llvm::LLVMContext *llvmContext);

  /// @brief Deleted copy constructor to prevent dynamic memory issues caused by
  /// an accidental copy.
  Context(const Context &) = delete;

  /// @brief Destructor, checks if the `llvm::LLVMContext` is owned and exists.
  ~Context();

  /// @brief Deleted assignment operator to prevent dynamic memory issues.
  Context &operator=(const Context &) = delete;

  /// @brief Deleted move assignment operator to prevent dynamic memory issues.
  Context &operator=(Context &&) = delete;

  /// @brief Get a description of all a modules specializable constants.
  ///
  /// @param code Array view of the SPIR-V binary stream.
  ///
  /// @return Returns a map of the modules specializable constants on success,
  /// otherwise report an error.
  cargo::expected<SpecializableConstantsMap, spirv_ll::Error>
  getSpecializableConstants(llvm::ArrayRef<uint32_t> code);

  /// @brief Translate a SPIR-V binary stream into a `spirv_ll::Module`.
  ///
  /// @param code Array view of the SPIR-V binary stream.
  /// @param deviceInfo Information about the target device.
  /// @param specInfo Information about specialization constants.
  ///
  /// @return Returns a `spirv_ll::Module` on success, otherwise a
  /// `spirv_ll::Error`.
  cargo::expected<spirv_ll::Module, spirv_ll::Error> translate(
      llvm::ArrayRef<uint32_t> code, const spirv_ll::DeviceInfo &deviceInfo,
      cargo::optional<const spirv_ll::SpecializationInfo &> specInfo);

  /// @brief LLVM context used for translation to LLVM IR.
  llvm::LLVMContext *llvmContext;

 private:
  /// @brief Flag to specify the ownership of `llvmContext`.
  const bool llvmContextIsOwned;
};

/// @}
}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_CONTEXT_H_INCLUDED
