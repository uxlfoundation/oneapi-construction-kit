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
/// @brief Definitions for the OpenCL context API.

#ifndef CL_CONTEXT_H_INCLUDED
#define CL_CONTEXT_H_INCLUDED

#include <CL/cl.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <cl/base.h>
#include <compiler/context.h>
#include <compiler/target.h>
#include <mux/mux.h>
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
#include <extension/intel_unified_shared_memory.h>
#endif

#include <compiler/module.h>

#include <memory>
#include <mutex>
#include <unordered_map>

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Context creation callback function pointer definition.
///
/// @param errinfo Pointer to the error string.
/// @param private_info Pointer to the implementation defined binary data, can
/// be used to log additional information to help with debugging.
/// @param cb Size of the binary data pointed to by `private_info`.
/// @param user_data Pointer to user supplied data.
using pfn_notify_context_t = void(CL_CALLBACK *)(const char *errinfo,
                                                 const void *private_info,
                                                 size_t cb, void *user_data);

#ifdef CL_VERSION_3_0
/// @brief Context destructor callback function pointer definition.
///
/// @param[in] context The context being deleted.
/// @param[in] user_data A pointer to user supplied data.
using pfn_notify_context_destructor_t = void(CL_CALLBACK *)(cl_context context,
                                                            void *user_data);
#endif

/// @}
}  // namespace cl

/// @addtogroup cl
/// @{

/// @brief Definition of the OpenCL context object.
struct _cl_context final : public cl::base<_cl_context> {
  /// @brief Context notification callback state.
  struct notify_callback_t final {
    /// @brief Invoke the context notification callback.
    ///
    /// @param[in] errinfo Pointer to the error string.
    /// @param[in] private_info Pointer to the implementation defined binary
    /// data, can be used to log additional information to help with debugging.
    /// @param[in] cb Size of the binary data pointed to by `private_info`.
    void operator()(const char *errinfo, const void *private_info, size_t cb) {
      pfn_notify(errinfo, private_info, cb, user_data);
    }

    /// @brief Conversion to `bool`.
    ///
    /// @return Returns `true` if `pfn_notify` is not null, `false` otherwise.
    operator bool() { return pfn_notify != nullptr; };

    /// @brief User callback function pointer, may be null.
    cl::pfn_notify_context_t pfn_notify;
    /// @brief User callback function state, may be null.
    void *user_data;
  };

#ifdef CL_VERSION_3_0
  /// @brief Context destructor callback state.
  struct destructor_callback_t final {
    /// @brief Invoke the context destructor callback.
    ///
    /// @param[in] context The context being deleted.
    /// @param[in] user_data A pointer to user supplied data.
    void operator()(cl_context context) { pfn_notify(context, user_data); }

    /// @brief User destructor callback function pointer, may be null.
    cl::pfn_notify_context_destructor_t pfn_notify;
    /// @brief User callback function state, may be null.
    void *user_data;
  };
#endif

  /// @brief Create a context object.
  ///
  /// @param devices List of devices for context to target.
  /// @param properties List of properties to for context to enable.
  /// @param notify User provided notification callback.
  static cargo::expected<cl_context, cl_int> create(
      cargo::array_view<const cl_device_id> devices,
      cargo::array_view<const cl_context_properties> properties,
      notify_callback_t notify);

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  _cl_context(_cl_context &&) = delete;

  /// @brief Destructor.
  ~_cl_context();

  /// @brief Query if the context targets a device.
  ///
  /// @param device Device to query the context with.
  ///
  /// @return Return true if the context targets the device, false otherwise.
  bool hasDevice(const cl_device_id device) const;

  /// @brief Get the index of the given device.
  ///
  /// @param device The device whose index to find.
  ///
  /// @return Returns the index of the device on success, `-1` on failure.
  cl_uint getDeviceIndex(const cl_device_id device) const;

  /// @brief Get a mux callback if one was supplied by the user.
  ///
  /// @return Returns a valid `mux_callback_info_t` if the user supplied a
  /// context callback, `nullptr` otherwise.
  mux_callback_info_t getMuxCallback() {
    return notify_callback ? &mux_callback : nullptr;
  }

  /// @brief Access the lazily allocated compiler context.
  ///
  /// @return Returns a pointer to the compiler context.
  compiler::Context *getCompilerContext();

  /// @brief Access the compiler target for a particular device.
  ///
  /// @param device The device whose index to find.
  ///
  /// @return Returns a pointer to the compiler target.
  compiler::Target *getCompilerTarget(const cl_device_id device);

  /// @brief Notify the OpenCL user via the context callback, if provided.
  ///
  /// If `notify_callback` is not provided, no action is taken.
  ///
  /// @param[in] errinfo Pointer to the error string.
  /// @param[in] private_info Pointer to the implementation defined binary
  /// data, can be used to log additional information to help with debugging.
  /// @param[in] cb Size of the binary data pointed to by `private_info`.
  void notify(const char *errinfo, const void *private_info, size_t cb) {
    if (notify_callback) {
      notify_callback(errinfo, private_info, cb);
    }
  }

#ifdef CL_VERSION_3_0
  /// @brief Push a context destructor callback onto the stack.
  ///
  /// @param[in] callback Context destructor callback to push to the stack.
  /// @param[in] user_data User data for the context destructor callback.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` when no error occurs.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurs.
  cl_int pushDestructorCallback(cl::pfn_notify_context_destructor_t callback,
                                void *user_data);
#endif

#if defined(OCL_EXTENSION_cl_khr_il_program) || defined(CL_VERSION_3_0)
  /// @brief Get device specific information needed to compile a SPIR-V module.
  ///
  /// @param device_info Target device information to compile the module.
  ///
  /// @return Returns device specific SPIR-V information, or nullopt if there's
  /// no SPIR-V device info for a given Mux device.
  cargo::optional<const compiler::spirv::DeviceInfo &> getSPIRVDeviceInfo(
      mux_device_info_t device_info);
#endif

  /// @brief List of devices the context targets.
  cargo::dynamic_array<cl_device_id> devices;
  /// @brief Mutex to protect accesses for _cl_context::context with is not
  /// thread safe except for USM which has its own mutex. This must not be
  /// used above a command queue mutex, as it call program destructor during
  /// clean up
  std::mutex mutex;

  /// @brief Mutex to protect accesses USM allocations. Note due to the nature
  /// of usm allocations and queue related activities it is sometimes needed to
  /// stay around beyond just accessing the list. It must not be below the
  /// general context mutex or the queue mutex.
  std::mutex usm_mutex;

  /// @brief List of the context's enabled properties.
  cargo::dynamic_array<cl_context_properties> properties;
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  /// @brief List of allocations made through the USM extension entry points.
  cargo::small_vector<std::unique_ptr<extension::usm::allocation_info>, 1>
      usm_allocations;
#endif
  std::mutex &getCommandQueueMutex() { return command_queue_mutex; }

 private:
  /// @brief Default constructor, made private to enforce use of `create`.
  _cl_context()
      : base<_cl_context>(cl::ref_count_type::EXTERNAL),
        notify_callback{},
        mux_callback{} {}

  /// @brief Whether the compiler context has already been initialized.
  std::once_flag compiler_context_initialized;
  /// @brief Compiler context, lazily allocated when required.
  std::unique_ptr<compiler::Context> compiler_context;
  /// @brief A mutex that guards the compiler_targets map.
  std::mutex compiler_targets_mutex;
  /// @brief A mutex that guards any command queues.
  std::mutex command_queue_mutex;
  /// @brief Map of OpenCL devices to compiler targets.
  std::unordered_map<cl_device_id, std::unique_ptr<compiler::Target>>
      compiler_targets;
  /// @brief User provided notification callback.
  notify_callback_t notify_callback;
  /// @brief Mux callback information.
  mux_callback_info_s mux_callback;
#ifdef CL_VERSION_3_0
  /// @brief Storage for the "stack" of context destructor callbacks.
  cargo::small_vector<destructor_callback_t, 1> destructor_callbacks;
#endif

#if defined(OCL_EXTENSION_cl_khr_il_program) || defined(CL_VERSION_3_0)
  // @brief SPIR-V device info per Mux device.
  std::unordered_map<mux_device_info_t, compiler::spirv::DeviceInfo>
      spv_device_infos;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL context object.
///
/// @param[in] properties Properties to enable when creating the context.
/// @param[in] num_devices Number of elements in the `devices` list.
/// @param[in] devices List of devices to target with the context.
/// @param[in] pfn_notify Error callback function pointer.
/// @param[in] user_data User data to be passed to `pfn_notify`.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new context object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateContext.html
CL_API_ENTRY cl_context CL_API_CALL
CreateContext(const cl_context_properties *properties, cl_uint num_devices,
              const cl_device_id *devices, cl::pfn_notify_context_t pfn_notify,
              void *user_data, cl_int *errcode_ret);

/// @brief Create an OpenCL context object from device type.
///
/// @param[in] properties Properties to enable when creating the context.
/// @param[in] device_type Type of device to target with the context.
/// @param[in] pfn_notify Error callback function pointer.
/// @param[in] user_data User data to be passed to `pfn_notify`.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new context object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateContextFromType.html
CL_API_ENTRY cl_context CL_API_CALL CreateContextFromType(
    const cl_context_properties *properties, cl_device_type device_type,
    cl::pfn_notify_context_t pfn_notify, void *user_data, cl_int *errcode_ret);

/// @brief Increment the context reference count.
///
/// @param[in] context Context to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainContext.html
CL_API_ENTRY cl_int CL_API_CALL RetainContext(cl_context context);

/// @brief Decrement the context reference count.
///
/// @param[in] context Context to decrement the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseContext.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseContext(cl_context context);

/// @brief Query the context for information.
///
/// @param[in] context Context to query for information.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of `param_value` storage.
/// @param[out] param_value Pointer to value to store the information in.
/// @param[out] param_value_size_ret Return size in bytes required to store
/// information in `param_value`.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetContextInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetContextInfo(cl_context context,
                                               cl_context_info param_name,
                                               size_t param_value_size,
                                               void *param_value,
                                               size_t *param_value_size_ret);

/// @}
}  // namespace cl

#endif  // CL_CONTEXT_H_INCLUDED
