// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// @brief Definitions for the OpenCL platform API.

#ifndef CL_PLATFORM_H_INCLUDED
#define CL_PLATFORM_H_INCLUDED

#include <CL/cl.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/optional.h>
#include <cl/base.h>
#include <compiler/loader.h>
#include <mux/mux.h>

#include <mutex>

/// @addtogroup cl
/// @{

/// @brief Definition of the OpenCL platform object.
struct _cl_platform_id final : cl::base<_cl_platform_id> {
private:
  /// @brief Private default constructor.
  ///
  /// The default constructor populates the ICD dispatch table, creates all
  /// device objects, and ensures the platform is in a ready to use state.
  ///
  /// @see _cl_platform_id::getInstance()
  _cl_platform_id() : base<_cl_platform_id>(cl::ref_count_type::INTERNAL) {}

public:
  /// @brief The only way to access the single `cl_platform_id` instance.
  ///
  /// There are multiple entry points which require the platform to be
  /// initialize such as `cl::GetPlatformIDs` and
  /// `cl::GetExtensionFunctionAddress` but we only want to initialize the
  /// platform once. The `cl_platform_id` is a local static variable of the
  /// `getPlatform()` function and initialization is performed in a lambda
  /// passed to `std::call_once`.
  ///
  /// If there are any allocation failures `abort()` is called as this is a
  /// critical failure which can not be recovered from.
  ///
  /// @return Returns the only `cl_platform_id` instance on success.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if platform initialization failed.
  static cargo::expected<cl_platform_id, cl_int> getInstance();

  /// @brief Get a pointer to the compiler library handle.
  ///
  /// @return Returns a pointer to the library handle when the compiler library
  /// is loaded, `nullptr` otherwise.
  compiler::Library *getCompilerLibrary();

  /// @brief Get compiler library loader error message.
  ///
  /// @return Returns a string containing the compiler library loader error
  /// message if library loading failed, an empty optional otherwise.
  cargo::optional<std::string> getCompilerLibraryLoaderError();

  /// @brief List of devices owned by the platform.
  cargo::dynamic_array<cl_device_id> devices;

private:
  /// @brief Compiler library.
  cargo::expected<std::unique_ptr<compiler::Library>, std::string>
      compiler_library;

#if defined(CL_VERSION_3_0)
public:
  /// @brief Resolution of the timestamp returned by clGetHostTimer and
  /// clGetDeviceAndHostTimer.
  cl_ulong host_timer_resolution = 0;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Query the OpenCL implementation for available platform obejcts.
///
/// @param num_entries Number of entries in the @p platforms list.
/// @param platforms Return list of available platforms.
/// @param num_platforms Return number of available platforms.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetPlatformIDs.html
CL_API_ENTRY cl_int CL_API_CALL GetPlatformIDs(const cl_uint num_entries,
                                               cl_platform_id *platforms,
                                               cl_uint *const num_platforms);

/// @brief Query the platform for information.
///
/// @param platform Platform to query for information.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of @p param_value storage, may be
/// zero.
/// @param param_value Return value of information if not null.
/// @param param_value_size_ret Return size in bytes required for @p param_value
/// storage if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetPlatformInfo.html
CL_API_ENTRY cl_int CL_API_CALL
GetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                size_t param_value_size, void *param_value,
                size_t *const param_value_size_ret);

/// @brief Query the platform for address of extension function.
///
/// @param platform Platform to query for extension function.
/// @param func_name Name of the function to return.
///
/// @return Return function pointer to extension function.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetExtensionFunctionAddressForPlatform.html
CL_API_ENTRY void *CL_API_CALL GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name);

/// @brief Query the platform for address of extension function, deprecated in
/// OpenCL 1.2.
///
/// @param func_name Name of the function to return.
///
/// @return Return function pointer to extension function.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetExtensionFunctionAddress.html
CL_API_ENTRY void *CL_API_CALL
GetExtensionFunctionAddress(const char *func_name);

/// @}
} // namespace cl

#endif // CL_PLATFORM_H_INCLUDED
