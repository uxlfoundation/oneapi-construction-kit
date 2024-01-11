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
/// @brief OCL extension API.

#ifndef EXTENSION_EXTENSION_H_INCLUDED
#define EXTENSION_EXTENSION_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_codeplay.h>
#include <cargo/optional.h>
#include <cargo/string_view.h>
#include <cl/macros.h>
#include <extension/config.h>

#include <string>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Base class for extensions.
class extension {
 public:
  /// @brief Extension usage category.
  enum class usage_category : unsigned int {
    /// @brief Do not expose the extension string.
    DISABLED,
    /// @brief Exposed in CL_PLATFORM_EXTENSIONS string.
    PLATFORM,
    /// @brief Exposed in CL_DEVICE_EXTENSIONS string.
    DEVICE,
  };

 protected:
  /// @brief Constructor.
  ///
  /// Does not take ownership of name.
  ///
  /// @param[in] name Name of the extension.
  /// @param[in] usage Usage category of the extension, controls where the
  /// extension name is reported to the user.
  extension(const char *name, usage_category usage
#if defined(CL_VERSION_3_0)
            ,
            cl_version_khr version
#endif
  );

 public:
  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  extension(extension &&) = delete;

  /// @brief Pure virtual destructor to enforce sub-classes to create instances.
  ///
  /// Implemented internally to be callable by sub-class destructors.
  virtual ~extension() = 0;

  /// @brief Query for platform info.
  ///
  /// @see clGetPlatformInfo.
  ///
  /// If name usage is kClPlatformExtensions then the extension name is used as
  /// the value for param_name CL_PLATFORM_EXTENSIONS queries.
  ///
  ///
  /// @param[in] platform OpenCL platform to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetPlatformInfo(cl_platform_id platform,
                                 cl_platform_info param_name,
                                 size_t param_value_size, void *param_value,
                                 size_t *param_value_size_ret) const;

  /// @brief Query for device info.
  ///
  /// @see clGetDeviceInfo.
  ///
  /// If name usage is kClDeviceExtensions then the extension name is used as
  /// the value for param_name CL_DEVICE_EXTENSIONS queries.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) const;

  /// @brief Query for device info.
  ///
  /// @see clGetContextInfo.
  ///
  /// @param[in] context OpenCL context to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetContextInfo(cl_context context, cl_context_info param_name,
                                size_t param_value_size, void *param_value,
                                size_t *param_value_size_ret) const;

  /// @brief Apply a property to a command queue.
  ///
  /// Extension hook for a single extended property passed to
  /// `clCreateCommandQueueWithPropertiesKHR`.
  ///
  /// @param[in] command_queue OpenCL command queue to apply property to.
  /// @param[in] property Property to enable.
  /// @param[in] value Value of the property to enable.
  ///
  /// @return Returns an optional OpenCL error code.
  /// @retval `CL_SUCCESS` if the property was successfully applied to the
  /// `command_queue`.
  /// @retval `CL_INVALID_QUEUE_PROPERTIES` if `property` is invalid.
  /// @retval `CL_INVALID_VALUE` if `value` is invalid.
  /// @retval `cargo::nullopt` if the entry point is not supported.
  virtual cargo::optional<cl_int> ApplyPropertyToCommandQueue(
      cl_command_queue command_queue, cl_queue_properties_khr property,
      cl_queue_properties_khr value) const;

  /// @brief Query for command queue info.
  ///
  /// @see clGetCommandQueueInfo.
  ///
  /// @param[in] command_queue OpenCL command queue to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetCommandQueueInfo(cl_command_queue command_queue,
                                     cl_command_queue_info param_name,
                                     size_t param_value_size, void *param_value,
                                     size_t *param_value_size_ret) const;

  /// @brief Query for image info.
  ///
  /// @see clGetImageInfo.
  ///
  /// @param[in] image OpenCL image to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetImageInfo(cl_mem image, cl_image_info param_name,
                              size_t param_value_size, void *param_value,
                              size_t *param_value_size_ret) const;

  /// @brief Query for memory object info.
  ///
  /// @see clGetMemObjectInfo.
  ///
  /// @param[in] memobj OpenCL memory object to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetMemObjectInfo(cl_mem memobj, cl_mem_info param_name,
                                  size_t param_value_size, void *param_value,
                                  size_t *param_value_size_ret) const;

  /// @brief Query for sampler info.
  ///
  /// @see clGetSamplerInfo.
  ///
  /// @param[in] sampler OpenCL sampler to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetSamplerInfo(cl_sampler sampler, cl_sampler_info param_name,
                                size_t param_value_size, void *param_value,
                                size_t *param_value_size_ret) const;

  /// @brief Query for program info.
  ///
  /// @see clGetProgramInfo.
  ///
  /// @param[in] program OpenCL program to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetProgramInfo(cl_program program, cl_program_info param_name,
                                size_t param_value_size, void *param_value,
                                size_t *param_value_size_ret) const;

  /// @brief Query for program build info.
  ///
  /// @see clGetProgramBuildInfo.
  ///
  /// @param[in] program OpenCL program to query.
  /// @param[in] device Specific device build info stored in program to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetProgramBuildInfo(cl_program program, cl_device_id device,
                                     cl_program_build_info param_name,
                                     size_t param_value_size, void *param_value,
                                     size_t *param_value_size_ret) const;

  /// @brief Query for kernel info.
  ///
  /// @see clGetKernelInfo.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) const;

  /// @brief Query for kernel work group info.
  ///
  /// @see clGetKernelWorkGroupInfo.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] device Specific device to take into account for kernel work
  /// group information.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device,
                                        cl_kernel_work_group_info param_name,
                                        size_t param_value_size,
                                        void *param_value,
                                        size_t *param_value_size_ret) const;

  /// @brief Set the argument value for a specific argument of a kernel.
  ///
  /// @param kernel A valid kernel object.
  /// @param arg_index The argument index.
  /// @param arg_size Specifies the size of the argument value, in bytes.
  /// @param arg_value Pointer to data that should be used as the argument
  /// value.
  ///
  /// @return Returns any code which can be returned from `clSetKernelArg`.
  /// @retval `CL_INVALID_KERNEL` if the extension failed to set the argument.
  virtual cl_int SetKernelArg(cl_kernel kernel, cl_uint arg_index,
                              size_t arg_size, const void *arg_value) const;

  /// @brief Query for kernel argument info.
  ///
  /// @see clGetKernelArgInfo.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] arg_indx Specific argument index of the kernel to query for
  /// information.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetKernelArgInfo(cl_kernel kernel, cl_uint arg_indx,
                                  cl_kernel_arg_info param_name,
                                  size_t param_value_size, void *param_value,
                                  size_t *param_value_size_ret) const;

#if defined(CL_VERSION_3_0)
  /// @brief Query for kernel subgroup info.
  ///
  /// @see clGetKernelSubGroupInfo.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] device Identifies device in device list associated with @p
  /// kernel.
  /// @param[in] input_value_size Size in bytes of memory pointed to by
  /// input_value.
  /// @param[in] input_value Pointer to memory where parameterization of query
  /// is passed from.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetKernelSubGroupInfo(cl_kernel kernel, cl_device_id device,
                                       cl_kernel_sub_group_info param_name,
                                       size_t input_value_size,
                                       const void *input_value,
                                       size_t param_value_size,
                                       void *param_value,
                                       size_t *param_value_size_ret) const;
#endif

#if (defined(CL_VERSION_3_0) || \
     defined(OCL_EXTENSION_cl_codeplay_kernel_exec_info))
  /// @brief Passes additional information other than argument values to a
  /// kernel.
  ///
  /// @see clSetKernelExecInfo
  ///
  /// @param kernel A valid kernel object.
  /// @param param_name Information to be passed to kernel.
  /// @param param_value_size Size in bytes of the memory pointed to by
  /// param_value.
  /// @param param_value Pointer to memory where the appropriate values
  /// determined by param_name are specified
  ///
  /// @return Returns any code which can be returned from `clSetKernelExecInfo`.
  /// @retval `CL_INVALID_KERNEL` if the extension failed to set the argument.
  virtual cl_int SetKernelExecInfo(cl_kernel kernel,
                                   cl_kernel_exec_info_codeplay param_name,
                                   size_t param_value_size,
                                   const void *param_value) const;
#endif

  /// @brief Query for event info.
  ///
  /// @see clGetEventInfo.
  ///
  /// @param[in] event OpenCL event to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetEventInfo(cl_event event, cl_event_info param_name,
                              size_t param_value_size, void *param_value,
                              size_t *param_value_size_ret) const;

  /// @brief Query for event profiling info.
  ///
  /// @see clGetEventProfilingInfo.
  ///
  /// @param[in] event OpenCL event to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int GetEventProfilingInfo(cl_event event,
                                       cl_profiling_info param_name,
                                       size_t param_value_size,
                                       void *param_value,
                                       size_t *param_value_size_ret) const;

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// @see clGetExtensionFunctionAddressForPlatform.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name Name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name `func_name` exists.
  virtual void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const;

#if defined(CL_VERSION_3_0)
  /// @brief  Constructs a cl_name_version_khr object for this extension.
  ///
  /// @return cl_name_version_khr object for this extension
  cl_name_version_khr GetNameVersion() {
    cl_name_version_khr name_version;
    name_version.version = version;
    std::memcpy(name_version.name, name.data(), name.size());
    name_version.name[name.size()] = '\0';
    return name_version;
  }
#endif

  const std::string name;
  const usage_category usage;
#if defined(CL_VERSION_3_0)
  /// @brief Major.Minor.Point version for this extension.
  ///
  /// Prior to OpenCL-3.0 versioning did not exist for extensions. As of 3.0
  /// all khr extensions start at (1,0,0). Codeplay extensions will be versioned
  /// as (0, X, 0) where X is the version revision as defined in the
  /// ComputeAorta extension spec.
  const cl_version_khr version;
#endif
};

/// @brief Aggregate all extended platform information.
///
/// @param platform Platform to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret);

/// @brief Aggregate all extended device information.
///
/// @param device Device to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                     size_t param_value_size, void *param_value,
                     size_t *param_value_size_ret);

/// @brief Aggregate all extended context information.
///
/// @param context Context to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetContextInfo(cl_context context, cl_context_info param_name,
                      size_t param_value_size, void *param_value,
                      size_t *param_value_size_ret);

/// @brief Apply a property to a command queue.
///
/// Extension hook for a single extended property passed to
/// `clCreateCommandQueueWithPropertiesKHR`.
///
/// @param[in] command_queue OpenCL command queue to apply property to.
/// @param[in] property Property to enable.
/// @param[in] value Value of the property to enable.
///
/// @return Returns an OpenCL error code.
/// @retval `CL_SUCCESS` if the property was successfully applied to the
/// `command_queue`.
/// @retval `CL_INVALID_QUEUE_PROPERTIES` if `property` is invalid.
/// @retval `CL_INVALID_VALUE` if `value` is invalid.
cl_int ApplyPropertyToCommandQueue(cl_command_queue command_queue,
                                   cl_queue_properties_khr property,
                                   cl_queue_properties_khr value);

/// @brief Aggregate all extended command queue information.
///
/// @param command_queue Command queue to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetCommandQueueInfo(cl_command_queue command_queue,
                           cl_command_queue_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret);

/// @brief Aggregate all extended image information.
///
/// @param image Image to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Requred size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetImageInfo(cl_mem image, cl_image_info param_name,
                    size_t param_value_size, void *param_value,
                    size_t *param_value_size_ret);

/// @brief Aggregate all extended memory object information.
///
/// @param memobj Memory object to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetMemObjectInfo(cl_mem memobj, cl_mem_info param_name,
                        size_t param_value_size, void *param_value,
                        size_t *param_value_size_ret);

/// @brief Aggregate all extended sampler information.
///
/// @param sampler Sampler to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetSamplerInfo(cl_sampler sampler, cl_sampler_info param_name,
                      size_t param_value_size, void *param_value,
                      size_t *param_value_size_ret);

/// @brief Aggregate all extended program information.
///
/// @param program Program to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetProgramInfo(cl_program program, cl_program_info param_name,
                      size_t param_value_size, void *param_value,
                      size_t *param_value_size_ret);

cl_int GetProgramBuildInfo(cl_program program, cl_device_id device,
                           cl_program_build_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret);

/// @brief Aggregate all extended kernel information.
///
/// @param kernel Kernel to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                     size_t param_value_size, void *param_value,
                     size_t *param_value_size_ret);

/// @brief Aggregate all kernel work group information.
///
/// @param kernel Kernel to query.
/// @param device Device being targeted.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device,
                              cl_kernel_work_group_info param_name,
                              size_t param_value_size, void *param_value,
                              size_t *param_value_size_ret);

/// @brief Set the argument value for a specific argument of a kernel.
///
/// @param kernel A valid kernel object.
/// @param arg_index The argument index.
/// @param arg_size Specifies the size of the argument value, in bytes.
/// @param arg_value Pointer to data that should be used as the argument
/// value.
///
/// @return Returns any code which can be returned from `clSetKernelArg`.
/// @retval `CL_INVALID_KERNEL` if the extension failed to set the argument.
cl_int SetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size,
                    const void *arg_value);

/// @brief Aggregate all extended kernel argument information.
///
/// @param kernel Kernel to query.
/// @param arg_indx Argument index to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetKernelArgInfo(cl_kernel kernel, cl_uint arg_indx,
                        cl_kernel_arg_info param_name, size_t param_value_size,
                        void *param_value, size_t *param_value_size_ret);

#if defined(CL_VERSION_3_0)
/// @brief Aggregate all kernel sub group information.
///
/// @param kernel Kernel to query.
/// @param device Device being targeted.
/// @param param_name Parameter name to query.
/// @param input_value_size Size in bytes of memory pointed to by
/// input_value.
/// @param input_value Pointer to memory where parameterization of query is
/// passed from.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetKernelSubGroupInfo(cl_kernel kernel, cl_device_id device,
                             cl_kernel_sub_group_info param_name,
                             size_t input_value_size, const void *input_value,
                             size_t param_value_size, void *param_value,
                             size_t *param_value_size_ret);
#endif

#if (defined(CL_VERSION_3_0) || \
     defined(OCL_EXTENSION_cl_codeplay_kernel_exec_info))
/// @brief Passes additional information other than argument values to a kernel.
///
/// @param kernel A valid kernel object.
/// @param param_name Information to be passed to kernel.
/// @param param_value_size Size in bytes of the memory pointed to by
/// `param_value`.
/// @param param_value Pointer to memory where the appropriate values determined
/// by `param_name` are specified
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int SetKernelExecInfo(cl_kernel kernel,
                         cl_kernel_exec_info_codeplay param_name,
                         size_t param_value_size, const void *param_value);
#endif

/// @brief Aggregate all extended event information.
///
/// @param event Event to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetEventInfo(cl_event event, cl_event_info param_name,
                    size_t param_value_size, void *param_value,
                    size_t *param_value_size_ret);

/// @brief Aggregate all extended event profiling information.
///
/// @param event Event to query.
/// @param param_name Parameter name to query.
/// @param param_value_size Size of `param_value` storage.
/// @param param_value Storage for returned value.
/// @param param_value_size_ret Required size of `param_value`.
///
/// @return Returns `CL_SUCCESS` or `CL_INVALID_VALUE`.
cl_int GetEventProfilingInfo(cl_event event, cl_profiling_info param_name,
                             size_t param_value_size, void *param_value,
                             size_t *param_value_size_ret);

/// @brief Access extension function address.
///
/// @param platform Platform extension exists on.
/// @param func_name Name of extension function.
///
/// @return Returns a void pointer to the extension function.
void *GetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                             const char *func_name);

/// @brief Get list of runtime extensions supported by the device.
///
/// @param[in] device Device to check.
/// @param[out] extensions_ret Space-separated view of extensions.
///
/// @return CL_SUCCESS for success or an error code otherwise.
[[nodiscard]] cl_int GetRuntimeExtensionsForDevice(
    cl_device_id device, cargo::string_view &extensions_ret);

/// @brief Get list of compiler extensions supported by the device.
///
/// @param[in] device Device to check.
/// @param[out] extensions_ret Space-separated view of extensions.
///
/// @return CL_SUCCESS for success or an error code otherwise.
[[nodiscard]] cl_int GetCompilerExtensionsForDevice(
    cl_device_id device, cargo::string_view &extensions_ret);
/// @}
}  // namespace extension

#endif  // EXTENSION_EXTENSION_H_INCLUDED
