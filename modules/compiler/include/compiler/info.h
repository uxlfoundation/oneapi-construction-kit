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
/// @brief Compiler info API.

#ifndef COMPILER_INFO_H_INCLUDED
#define COMPILER_INFO_H_INCLUDED

#include <builtins/bakery.h>
#include <cargo/optional.h>
#include <mux/mux.h>

#include <functional>
#include <memory>

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief Context forward declaration.
class Context;

/// @brief Target forward declaration.
class Target;

/// @brief Notification callback.
///
/// This callback may be invoked by the implementation to provide more detailed
/// information about API usage.
///
/// @param[in] message C string containing the diagnostic message.
/// @param[in] data Pointer to additional binary data for the user to
/// supplement the information in `message`. May be `nullptr`.
/// @param[in] data_size Size of additional binary data in bytes, may be `0`
/// when `data` is `nullptr`.
using NotifyCallbackFn = std::function<void(
    const char *message, const void *data, size_t data_size)>;

/// @brief Compiler information.
struct Info {
  /// @brief Mux device info that this compiler will target. Must not be
  /// `nullptr`.
  mux_device_info_t device_info = nullptr;
  /// @brief A semicolon-separated, null-terminated list with static lifetime
  /// duration, of this Mux device's custom compile options.
  ///
  /// For each option a comma seperated tuple of (argument name, [1|0] denoting
  /// if value needs to be provided, help message). Option name for argument
  /// must start with a double hyphen e.g. '--enable-custom-optimization'. See
  /// the Mux specification for further details.
  const char *compilation_options = nullptr;
  /// @brief Is `true` if the compiler supports vectorization, `false`
  /// otherwise.
  bool vectorizable = false;
  /// @brief Is `true` if the compiler supports DMA optimizations, `false`
  /// otherwise.
  bool dma_optimizable = false;
  /// @brief Is `true` if the compiler supports scalable vectors, `false`
  /// otherwise.
  bool scalable_vector_support = false;
  /// @brief Is `true` if the compiler supports kernel debugging, `false`
  /// otherwise.
  bool kernel_debug = false;

  /// Default destructor.
  virtual ~Info() = default;

  /// @brief Returns a new compiler target.
  ///
  /// @param[in] context A context object to associate with this compiler
  /// target.
  /// @param[in] callback Notification message callback, may be null.
  ///
  /// @return A new compiler target.
  virtual std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context, NotifyCallbackFn callback) const = 0;

  builtins::file::capabilities_bitfield getBuiltinCapabilities() const {
    // TODO: CA-882 Resolve how capabilities are checked
    const auto reqd_caps_fp64 = mux_floating_point_capabilities_denorm |
                                mux_floating_point_capabilities_inf_nan |
                                mux_floating_point_capabilities_rte |
                                mux_floating_point_capabilities_rtz |
                                mux_floating_point_capabilities_rtp |
                                mux_floating_point_capabilities_rtn |
                                mux_floating_point_capabilities_fma;

    // deduce whether device meets all the requirements for halfs
    // TODO: CA-882 Resolve how capabilities are checked
    // cl_khr_fp16 requires either rtz or (rte | inf_nan)
    const auto reqd_caps_fp16_a = mux_floating_point_capabilities_rtz;
    const auto reqd_caps_fp16_b = mux_floating_point_capabilities_rte |
                                  mux_floating_point_capabilities_inf_nan;

    builtins::file::capabilities_bitfield caps = 0;

    // Bit width
    if (device_info->address_capabilities & mux_address_capabilities_bits32) {
      caps |= builtins::file::CAPS_32BIT;
    }
    // Doubles
    if ((device_info->double_capabilities & reqd_caps_fp64) == reqd_caps_fp64) {
      caps |= builtins::file::CAPS_FP64;
    }
    // Halfs
    if ((device_info->half_capabilities & reqd_caps_fp16_a) ==
            reqd_caps_fp16_a ||
        (device_info->half_capabilities & reqd_caps_fp16_b) ==
            reqd_caps_fp16_b) {
      caps |= builtins::file::CAPS_FP16;
    }

    return caps;
  }

  /// @brief Returns `true` if the compiler supports deferred compilation (i.e.
  /// compiler::Module::getKernel() and the compiler::Kernel class are
  /// implemented), `false` otherwise.
  virtual bool supports_deferred_compilation() const { return false; }
};

/// @brief A functor which is called when a target wants to expose a compiler.
///
/// The `compiler::Info` argument should be a pointer to a static instance of a
/// `compiler::Info` that represents a specific compiler configuration.
using AddCompilerFn = std::function<void(const Info *)>;

/// @}
}  // namespace compiler

#endif  // COMPILER_INFO_H_INCLUDED
