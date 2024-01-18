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

#include <cargo/small_vector.h>
#include <cl/binary/binary.h>
#include <cl/binary/spirv.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/platform.h>
#include <compiler/loader.h>
#include <tracer/tracer.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

cargo::expected<cl_context, cl_int> _cl_context::create(
    cargo::array_view<const cl_device_id> devices,
    cargo::array_view<const cl_context_properties> properties,
    notify_callback_t notify_callback) {
  std::unique_ptr<_cl_context> context(new _cl_context);
  if (!context) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  if (context->devices.alloc(devices.size()) != cargo::success) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  if (context->properties.alloc(properties.size()) != cargo::success) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  std::copy(devices.begin(), devices.end(), context->devices.begin());
  std::for_each(devices.begin(), devices.end(),
                &cl::retainInternal<cl_device_id>);
  std::copy(properties.begin(), properties.end(), context->properties.begin());
  context->notify_callback = notify_callback;
  if (context->notify_callback) {
    // When the user provides a context callback at creation we must pass this
    // on to Mux, in order to use them we setup a mux_callback_info_s here.
    context->mux_callback.callback = [](void *user_data, const char *message,
                                        const void *data, size_t data_size) {
      auto context = static_cast<cl_context>(user_data);
      context->notify_callback(message, data, data_size);
    };
    context->mux_callback.user_data = context.get();
  }

  // Create SPIR-V device infos.
#if defined(OCL_EXTENSION_cl_khr_il_program) || defined(CL_VERSION_3_0)
  for (_cl_device_id *device : devices) {
    auto device_info = device->mux_device->info;
    context->spv_device_infos[device_info] =
        *cl::binary::getSPIRVDeviceInfo(device_info, device->profile);
  }
#endif

  return context.release();
}

_cl_context::~_cl_context() {
  // Clear our references to compiler targets, they must not outlive their
  // respective Contexts.
  compiler_targets.clear();
  // The compiler context must be destroyed before we release the internal
  // references to the devices within the context.
  compiler_context.reset();
  // In applications which release the context in a global variables destructor
  // releasing the devices here may cause them to be destroyed at this point if
  // their internal reference count is 1, therefore any objects in the context
  // when depend on the devices must be destroyed before they are released.
  for (auto device : devices) {
    cl::releaseInternal(device);
  }
#ifdef CL_VERSION_3_0
  // Call the destructor callbacks in reverse order as stated in the spec.
  std::for_each(
      destructor_callbacks.rbegin(), destructor_callbacks.rend(),
      [&](_cl_context::destructor_callback_t &callback) { callback(this); });
#endif
}

bool _cl_context::hasDevice(const cl_device_id device) const {
  return std::find(devices.begin(), devices.end(), device) != devices.end();
}

cl_uint _cl_context::getDeviceIndex(const cl_device_id device) const {
  auto it = std::find(devices.begin(), devices.end(), device);
  if (it == devices.end()) {
    OCL_ABORT("Device not found in context!");
  }
  return static_cast<cl_uint>(std::distance(devices.begin(), it));
}

compiler::Context *_cl_context::getCompilerContext() {
#ifdef CA_RUNTIME_COMPILER_ENABLED
  std::call_once(compiler_context_initialized, [this]() {
    OCL_ASSERT(!compiler_context, "compiler::Context predates initialization.");
    // Note: We are guaranteed to have at least 1 device (checked in
    // cl::CreateContext), and guaranteed to have exactly 1 platform instance
    // (enforced via _cl_platform_id::getInstance()).
    compiler_context =
        compiler::createContext(devices[0]->platform->getCompilerLibrary());
    if (compiler_context) {
      cargo::small_vector<mux_device_info_t, 4> mux_device_infos;
      for (auto dev : devices) {
        if (mux_device_infos.push_back(dev->mux_device->info) !=
            cargo::success) {
          OCL_ABORT("Failed to allocate devices list for compiler::Context.");
        }
      }
    }
  });
  return compiler_context.get();
#else
  return nullptr;
#endif
}

compiler::Target *_cl_context::getCompilerTarget(const cl_device_id device) {
  const std::lock_guard<std::mutex> guard{compiler_targets_mutex};

  auto it = compiler_targets.find(device);
  if (it != compiler_targets.end()) {
    return it->second.get();
  }

  if (!device->compiler_available) {
    return nullptr;
  }

  std::unique_ptr<compiler::Target> target =
      device->compiler_info->createTarget(
          getCompilerContext(),
          notify_callback ? notify_callback : compiler::NotifyCallbackFn{});
  if (!target) {
    return nullptr;
  }

  if (target->init(cl::binary::detectBuiltinCapabilities(
          device->mux_device->info)) != compiler::Result::SUCCESS) {
    return nullptr;
  }

  compiler_targets[device] = std::move(target);
  return compiler_targets[device].get();
}

#ifdef CL_VERSION_3_0
cl_int _cl_context::pushDestructorCallback(
    cl::pfn_notify_context_destructor_t callback, void *user_data) {
  if (destructor_callbacks.push_back({callback, user_data}) ==
      cargo::bad_alloc) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  return CL_SUCCESS;
}
#endif

#if defined(OCL_EXTENSION_cl_khr_il_program) || defined(CL_VERSION_3_0)
cargo::optional<const compiler::spirv::DeviceInfo &>
_cl_context::getSPIRVDeviceInfo(mux_device_info_t device_info) {
  auto it = spv_device_infos.find(device_info);
  if (it != spv_device_infos.end()) {
    return {it->second};
  }
  return cargo::nullopt;
}
#endif

namespace {
/// @brief Utility functions to parse a cl_context_properties array.
///
/// A cl_context_properties array is an array that contains a
/// cl_context_properties value, directly followed by a pointer to associated
/// data, and finished by a cl_context_properties value of 0.
///
/// For example:
///   [CL_CONTEXT_PLATFORM, <platform id pointer>, 0]
///
/// @param[in] properties The cl_context_properties array to parse.
/// @param[out] propertiesLength The total length of the input array.
/// @param[out] platform The platform that was extracted from the properties, or
/// nullptr if the properties didn't specify any platforms.
///
/// @return CL_SUCCESS if the parsing was successful, otherwise an appropriate
/// cl error code.
cl_int parseProperties(const cl_context_properties *properties,
                       uint32_t &propertiesLength, cl_platform_id &platform) {
  if (nullptr == properties) {
    propertiesLength = 0;
    platform = nullptr;
    return CL_SUCCESS;
  }

  bool parsedPlatform = false;
  bool parsedInterop = false;
  cl_platform_id platformid = nullptr;

  uint32_t length = 0;
  const cl_context_properties *property = properties;
  while (0 != property[0]) {
    switch (property[0]) {
      case CL_CONTEXT_PLATFORM:
        platformid = reinterpret_cast<cl_platform_id>(property[1]);

        if (parsedPlatform || (platformid != _cl_platform_id::getInstance())) {
          return CL_INVALID_PROPERTY;
        }

        parsedPlatform = true;
        break;
      case CL_CONTEXT_INTEROP_USER_SYNC:
        if (parsedInterop) {
          return CL_INVALID_PROPERTY;
        }
        parsedInterop = true;
        // No-op for now
        break;
      default:
        return CL_INVALID_PROPERTY;
    }

    // Skip the property we just processed and its value
    length += 2;
    property = properties + length;
  }
  // Increment the length to account for the final 0
  ++length;

  platform = platformid;
  propertiesLength = length;
  return CL_SUCCESS;
}
}  // namespace

CL_API_ENTRY cl_context CL_API_CALL cl::CreateContext(
    const cl_context_properties *properties, cl_uint num_devices,
    const cl_device_id *devices,
    void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *user_data, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateContext");
  uint32_t length = 0;
  cl_platform_id platform = nullptr;
  if (auto error = parseProperties(properties, length, platform)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  // Store pfn_notify (Redmine issue #4994).
  if (!devices || (0 == num_devices) || (!pfn_notify && user_data)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
    return nullptr;
  }

  for (cl_uint i = 0; i < num_devices; ++i) {
    // Make sure the device is valid
    if (nullptr == devices[i]) {
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
      return nullptr;
    }

    // If we didn't get a platform id from the properties use the one in the
    // devices
    if (nullptr == platform) {
      platform = devices[i]->platform;
    }

    // Check that all the devices belong the the same platform
    if (platform != devices[i]->platform) {
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
      return nullptr;
    }
  }

  // Check that we got the correct platform
  if (platform != _cl_platform_id::getInstance()) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PLATFORM);
    return nullptr;
  }

  if (!devices || (0 == num_devices) || (!pfn_notify && user_data)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
    return nullptr;
  }

  auto context = _cl_context::create(
      {devices, num_devices}, {properties, length}, {pfn_notify, user_data});
  if (!context) {
    OCL_SET_IF_NOT_NULL(errcode_ret, context.error());
    return nullptr;
  }

  if (auto error = platform->getCompilerLibraryLoaderError()) {
    context.value()->notify(error->c_str(), nullptr, 0);
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return context.value();
}

CL_API_ENTRY cl_context CL_API_CALL cl::CreateContextFromType(
    const cl_context_properties *properties, cl_device_type device_type,
    void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *user_data, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateContextFromType");
  uint32_t length = 0;
  cl_platform_id platform = nullptr;
  if (auto error = parseProperties(properties, length, platform)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  if (platform != _cl_platform_id::getInstance()) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PLATFORM);
    return nullptr;
  }

  cl_uint num_devices;
  if (auto error =
          cl::GetDeviceIDs(platform, device_type, 0, nullptr, &num_devices)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }
  cargo::small_vector<cl_device_id, 4> devices;
  if (devices.resize(num_devices)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }
  if (auto error = cl::GetDeviceIDs(platform, device_type, num_devices,
                                    devices.data(), nullptr)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  if (devices.empty() || (!pfn_notify && user_data)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
    return nullptr;
  }

  auto context = _cl_context::create(devices, {properties, length},
                                     {pfn_notify, user_data});
  if (!context) {
    OCL_SET_IF_NOT_NULL(errcode_ret, context.error());
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return context.value();
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainContext(cl_context context) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainContext");
  OCL_CHECK(!context, return CL_INVALID_CONTEXT);

  return cl::retainExternal(context);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseContext(cl_context context) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseContext");
  OCL_CHECK(!context, return CL_INVALID_CONTEXT);

  return cl::releaseExternal(context);
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetContextInfo(
    cl_context context, cl_context_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetContextInfo");
  OCL_CHECK(!context, return CL_INVALID_CONTEXT);

  switch (param_name) {
    case CL_CONTEXT_REFERENCE_COUNT:
      if (param_value) {
        OCL_CHECK(param_value_size < sizeof(cl_uint), return CL_INVALID_VALUE);
        *(static_cast<cl_uint *>(param_value)) = context->refCountExternal();
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_uint));
      break;
    case CL_CONTEXT_NUM_DEVICES:
      if (param_value) {
        OCL_CHECK(param_value_size < sizeof(cl_uint), return CL_INVALID_VALUE);
        *(static_cast<cl_uint *>(param_value)) = context->devices.size();
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_uint));
      break;
    case CL_CONTEXT_DEVICES: {
      const size_t devices_size =
          sizeof(cl_device_id) * context->devices.size();
      OCL_CHECK(param_value && (param_value_size < devices_size),
                return CL_INVALID_VALUE);
      if (param_value) {
        std::uninitialized_copy(context->devices.begin(),
                                context->devices.end(),
                                static_cast<cl_device_id *>(param_value));
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, devices_size);
      break;
    }
    case CL_CONTEXT_PROPERTIES: {
      const size_t properties_size =
          sizeof(cl_context_properties) * context->properties.size();
      OCL_CHECK(param_value && (param_value_size < properties_size),
                return CL_INVALID_VALUE);
      if (param_value) {
        std::uninitialized_copy(
            context->properties.begin(), context->properties.end(),
            static_cast<cl_context_properties *>(param_value));
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, properties_size);
      break;
    }
    default: {
      return extension::GetContextInfo(context, param_name, param_value_size,
                                       param_value, param_value_size_ret);
    }
  }
  return CL_SUCCESS;
}
