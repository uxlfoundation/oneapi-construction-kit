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

#ifndef CL_BINARY_BINARY_H_INCLUDED
#define CL_BINARY_BINARY_H_INCLUDED

#include <CL/cl.h>
#include <builtins/printf.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/binary/kernel_info.h>
#include <cl/binary/program_info.h>
#include <cl/macros.h>
#include <compiler/context.h>
#include <compiler/module.h>
#include <compiler/target.h>
#include <metadata/metadata.h>
#include <mux/mux.hpp>

#include <functional>
#include <mutex>
#include <unordered_map>

namespace cl {
namespace binary {
/// @brief Metadata block names used for OpenCL metadata
constexpr char OCL_MD_EXECUTABLE_BLOCK[] = "OpenCL_executable";
constexpr char OCL_MD_IS_EXECUTABLE_BLOCK[] = "OpenCL_is_executable";
constexpr char OCL_MD_PRINTF_INFO_BLOCK[] = "OpenCL_printf_info";
constexpr char OCL_MD_PROGRAM_INFO_BLOCK[] = "OpenCL_program_info";

/// @brief Userdata used when reading metadata from a serialized program.
struct OpenCLReadUserdata {
  cargo::array_view<const uint8_t> binary;
};

/// @brief Userdata used when serializing an OpenCL program.
struct OpenCLWriteUserdata {
  cargo::dynamic_array<uint8_t> *binary_buffer;
  cargo::array_view<const uint8_t> mux_executable;
  bool is_executable;
  compiler::Module *compiler_module;
};

/// @brief Serialize an OpenCL program into a binary with metadata.
///
/// @param[in, out] binary A buffer into which the binary will be serialized.
/// @param[in] mux_binary The mux executable to be contained within the OpenCL
/// binary.
/// @param[in] printf_calls Printf descriptor list returned by the compiler.
/// @param[in] program_info Program information returned by the compiler.
/// @param[in] kernel_arg_info If building with -cl-kernel-arg-info (if argument
/// metadata is stored).
/// @param[in] compiler_module An optional pointer to the compiler module, used
/// to get the mux executable for non-executable kernels.
///
/// @return true if the binary was serialized successfully, false
bool serializeBinary(
    cargo::dynamic_array<uint8_t> &binary,
    const cargo::array_view<const uint8_t> mux_binary,
    const std::vector<builtins::printf::descriptor> &printf_calls,
    const compiler::ProgramInfo &program_info, bool kernel_arg_info,
    compiler::Module *compiler_module);

/// @brief Deserializes an OpenCL program binary.
///
/// @param[in] binary Binary buffer of the binary.
/// @param[out] printf_calls Printf descriptor list returned by the compiler.
/// @param[out] program_info Program information returned by the compiler.
/// @param[out] executable The executable contained within the OpenCL binary.
/// @param[out] is_executable If the program is executable.
///
/// @return true if the binary was deserialized successfully, false
bool deserializeBinary(cargo::array_view<const uint8_t> binary,
                       std::vector<builtins::printf::descriptor> &printf_calls,
                       compiler::ProgramInfo &program_info,
                       cargo::dynamic_array<uint8_t> &executable,
                       bool &is_executable);

/// @brief Get OpenCL Metadata Write Hooks.
///
/// @return md_hooks
md_hooks getOpenCLMetadataWriteHooks();

/// @brief Get the OpenCL Metadata Read Hooks.
///
/// @return md_hooks
md_hooks getOpenCLMetadataReadHooks();

/// @brief Detects the OpenCL device profile string from a given Mux device.
///
/// @param compiler_available A flag determining whether a compiler is
/// available.
/// @param device Mux device to query.
///
/// @return "FULL_PROFILE" if the device supports the OpenCL specification, or
/// "EMBEDDED_PROFILE" if the device supports the OpenCL embedded profile. If
cargo::string_view detectMuxDeviceProfile(cl_bool compiler_available,
                                          mux_device_info_t device);

/// @brief Determine OpenCL builtins capabilities from Mux device.
///
/// @param device_info Mux device info.
///
/// @return Builtin capabilities to pass to `compiler::Target::init`.
uint32_t detectBuiltinCapabilities(mux_device_info_t device_info);

}  // namespace binary
}  // namespace cl
#endif  // CL_BINARY_BINARY_H_INCLUDED
