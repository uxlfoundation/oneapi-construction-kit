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

#include <cargo/allocator.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/config.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/platform.h>
#include <extension/config.h>
#include <extension/extension.h>
#include <mux/mux.h>
#include <tracer/tracer.h>
#ifdef OCL_EXTENSION_cl_khr_icd
#include <extension/khr_icd.h>
#endif

#include <CL/cl_ext.h>

#include <algorithm>
#include <cstring>

namespace {
void *default_alloc(void *user_data, size_t size, size_t alignment) {
  (void)user_data;
  void *pointer = cargo::alloc(size, alignment);
  OCL_ASSERT(pointer, "default_alloc() allocation failure!");
  return pointer;
}

void default_free(void *user_data, void *pointer) {
  (void)user_data;
  cargo::free(pointer);
}
}  // namespace

cargo::expected<cl_platform_id, cl_int> _cl_platform_id::getInstance() {
  // The only instance of cl_platform_id.
  static cargo::expected<cl_platform_id, cl_int> platform;

  // Flag to passed to std::call_once.
  static std::once_flag initialized;

  std::call_once(initialized, []() {
    // Create the only cl_platform_id.
    std::unique_ptr<_cl_platform_id> new_platform{new (std::nothrow)
                                                      _cl_platform_id};
    if (!new_platform) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Core allocator information, will be copied to each device, this is not
    // owned by the platform due to global tear down issues in the CTS.
    const mux_allocator_info_t mux_allocator = {default_alloc, default_free,
                                                nullptr};

    // Find out how many mux devices have been registered.
    uint64_t num_devices;
    auto error =
        muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &num_devices);
    if (error) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Allocate storage for the mux devices and their information.
    cargo::small_vector<mux_device_info_t, 4> mux_device_infos;
    if (mux_device_infos.resize(num_devices)) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }
    cargo::small_vector<mux_device_t, 4> mux_devices;
    if (mux_devices.resize(num_devices)) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Get all device infos.
    error = muxGetDeviceInfos(mux_device_type_all, num_devices,
                              mux_device_infos.data(), nullptr);
    if (error) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Create all mux devices.
    error = muxCreateDevices(num_devices, mux_device_infos.data(),
                             mux_allocator, mux_devices.data());
    if (error) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Load the compiler library.
    new_platform->compiler_library = compiler::loadLibrary();

    // Allocate storage for the cl_device_id's.
    if (cargo::success != new_platform->devices.alloc(num_devices)) {
      platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      return;
    }

    // Create all the cl_device_id's for the mux devices.
    for (size_t index = 0; index < mux_devices.size(); index++) {
      new_platform->devices[index] = new (std::nothrow)
          _cl_device_id(new_platform.get(), mux_allocator, mux_devices[index]);
      if (!new_platform->devices[index]) {
        platform = cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
        return;
      }
    }

    // Platform creation was a success.
    platform = new_platform.release();

#if !defined(CA_PLATFORM_WINDOWS)
    // Add an atexit handler to destroy the cl_platform_id. This is not done on
    // Windows because DLL's which we rely on are not guarenteed to be loaded
    // when atexit handlers are invoked, the advice given by Microsoft is not
    // to perform any tear down at all.
    atexit([]() {
      for (auto device : platform.value()->devices) {
        cl::releaseInternal(device);
      }
      cl::releaseInternal(platform.value());
    });
#endif
  });
  return platform;
}

compiler::Library *_cl_platform_id::getCompilerLibrary() {
  return compiler_library ? compiler_library.value().get() : nullptr;
}

cargo::optional<std::string> _cl_platform_id::getCompilerLibraryLoaderError() {
  if (!compiler_library) {
    return compiler_library.error();
  }
  return cargo::nullopt;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::GetPlatformIDs(const cl_uint num_entries, cl_platform_id *platforms,
                   cl_uint *const num_platforms) {
  const tracer::TraceGuard<tracer::OpenCL> trace("clGetPlatformIDs");
  OCL_CHECK(platforms && (0 == num_entries), return CL_INVALID_VALUE);
  OCL_CHECK(!platforms && (0 < num_entries), return CL_INVALID_VALUE);
  OCL_CHECK(!platforms && !num_platforms, return CL_INVALID_VALUE);
  auto platform = _cl_platform_id::getInstance();
  if (!platform) {
    return platform.error();
  }
  OCL_SET_IF_NOT_NULL(num_platforms, 1);
  OCL_SET_IF_NOT_NULL(platforms, platform.value());
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::GetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                    size_t param_value_size, void *param_value,
                    size_t *const param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace("clGetPlatformInfo");
  OCL_CHECK(platform != _cl_platform_id::getInstance(),
            return CL_INVALID_PLATFORM);

#define PLATFORM_INFO_CASE(PARAM, VALUE)                           \
  case PARAM: {                                                    \
    const size_t size = std::strlen(VALUE) + 1;                    \
    OCL_CHECK(param_value && (param_value_size < size),            \
              return CL_INVALID_VALUE);                            \
    if (param_value) {                                             \
      std::strncpy(static_cast<char *>(param_value), VALUE, size); \
    }                                                              \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, size);               \
  } break

#define PLATFORM_INFO_CASE_NOT_STRING(PARAM, VALUE)          \
  case PARAM: {                                              \
    const size_t size = sizeof(VALUE);                       \
    OCL_CHECK(param_value && (param_value_size < size),      \
              return CL_INVALID_VALUE);                      \
    if (param_value) {                                       \
      *static_cast<decltype(&(VALUE))>(param_value) = VALUE; \
    }                                                        \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, size);         \
  } break

  switch (param_name) {
    case CL_PLATFORM_PROFILE: {
      // If any of the cl_device_id's are FULL_PROFILE the platform is
      // FULL_PROFILE, if all cl_device_id's are EMBEDDED_PROFILE the platform
      // is EMBEDDED_PROFILE.
      const bool full_profile =
          std::any_of(platform->devices.begin(), platform->devices.end(),
                      [](const cl_device_id device) {
                        return device->profile == "FULL_PROFILE";
                      });
      const char *profile = full_profile ? "FULL_PROFILE" : "EMBEDDED_PROFILE";
      const size_t size = std::strlen(profile) + 1;
      OCL_CHECK(param_value && (param_value_size < size),
                return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(static_cast<char *>(param_value), profile, size);
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size);
    } break;
      PLATFORM_INFO_CASE(CL_PLATFORM_VERSION, CA_CL_PLATFORM_VERSION);
      PLATFORM_INFO_CASE(CL_PLATFORM_NAME, CA_CL_PLATFORM_NAME);
      PLATFORM_INFO_CASE(CL_PLATFORM_VENDOR, CA_CL_PLATFORM_VENDOR);
#if defined(CL_VERSION_3_0)
      PLATFORM_INFO_CASE_NOT_STRING(CL_PLATFORM_HOST_TIMER_RESOLUTION,
                                    platform->host_timer_resolution);
    case CL_PLATFORM_NUMERIC_VERSION: {
      OCL_CHECK(param_value && (param_value_size < sizeof(cl_version)),
                return CL_INVALID_VALUE);
      if (param_value) {
        *static_cast<cl_version_khr *>(param_value) = CL_MAKE_VERSION_KHR(
            CA_CL_PLATFORM_VERSION_MAJOR, CA_CL_PLATFORM_VERSION_MINOR, 0);
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_version));
    } break;
#endif
    case CL_PLATFORM_EXTENSIONS: {
      size_t value_size;
      const cl_int error = extension::GetPlatformInfo(
          platform, CL_PLATFORM_EXTENSIONS, 0, nullptr, &value_size);
      OCL_CHECK(error, return error);
      OCL_CHECK(param_value && (param_value_size < value_size),
                return CL_INVALID_VALUE);
      return extension::GetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
                                        param_value_size, param_value,
                                        param_value_size_ret);
    }
    default: {
      return extension::GetPlatformInfo(platform, param_name, param_value_size,
                                        param_value, param_value_size_ret);
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY void *CL_API_CALL cl::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) {
  const tracer::TraceGuard<tracer::OpenCL> trace(
      "clGetExtensionFunctionAddressForPlatform");
  OCL_CHECK(platform != _cl_platform_id::getInstance(), return nullptr);
  return extension::GetExtensionFunctionAddressForPlatform(platform, func_name);
}

CL_API_ENTRY void *CL_API_CALL
cl::GetExtensionFunctionAddress(const char *func_name) {
  const tracer::TraceGuard<tracer::OpenCL> trace(
      "clGetExtensionFunctionAddress");
  if (auto platform = _cl_platform_id::getInstance()) {
    return extension::GetExtensionFunctionAddressForPlatform(*platform,
                                                             func_name);
  }
  return nullptr;
}
