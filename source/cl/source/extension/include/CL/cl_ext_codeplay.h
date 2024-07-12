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

#ifndef CL_EXT_CODEPLAY_H_INCLUDED
#define CL_EXT_CODEPLAY_H_INCLUDED

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*******************************
 * cl_codeplay_kernel_exec_info *
 ********************************/
typedef cl_uint cl_kernel_exec_info_codeplay;

extern CL_API_ENTRY cl_int CL_API_CALL clSetKernelExecInfoCODEPLAY(
    cl_kernel kernel, cl_kernel_exec_info_codeplay param_name,
    size_t param_value_size, const void *param_value);

typedef CL_API_ENTRY cl_int(CL_API_CALL *clSetKernelExecInfoCODEPLAY_fn)(
    cl_kernel kernel, cl_kernel_exec_info_codeplay param_name,
    size_t param_value_size,
    const void *param_value) CL_API_SUFFIX__VERSION_1_2;

/************************************
 * cl_codeplay_performance_counter   *
 ************************************/

/// @brief Accepted as `param_name` parameter to `clGetDeviceInfo`.
#define CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY 0x4260

/// @brief Accepted as a key in the `properties` key value array parameter to
/// `clCreateCommandQueueWithPropertiesKHR`.
#define CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY 0x4261

/// @brief Accepted as `param_name` parameter to `clGetEventProfilingInfo`.
#define CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY 0x4262

/// @brief Specifies a counter's unit is a generic value.
#define CL_PERFORMANCE_COUNTER_UNIT_GENERIC_CODEPLAY 0x0
/// @brief Specifies a counter's unit is a percentage.
#define CL_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_CODEPLAY 0x1
/// @brief Specifies a counter's unit is nanoseconds.
#define CL_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_CODEPLAY 0x2
/// @brief Specifies a counter's unit is bytes.
#define CL_PERFORMANCE_COUNTER_UNIT_BYTES_CODEPLAY 0x3
/// @brief Specifies a counter's unit is bytes per second.
#define CL_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_CODEPLAY 0x4
/// @brief Specifies a counter's unit is degrees kelvin.
#define CL_PERFORMANCE_COUNTER_UNIT_KELVIN_CODEPLAY 0x5
/// @brief Specifies a counter's unit is watts.
#define CL_PERFORMANCE_COUNTER_UNIT_WATTS_CODEPLAY 0x6
/// @brief Specifies a counter's unit is volts.
#define CL_PERFORMANCE_COUNTER_UNIT_VOLTS_CODEPLAY 0x7
/// @brief Specifies a counter's unit is amps.
#define CL_PERFORMANCE_COUNTER_UNIT_AMPS_CODEPLAY 0x8
/// @brief Specifies a counter's unit is hertz.
#define CL_PERFORMANCE_COUNTER_UNIT_HERTZ_CODEPLAY 0x9
/// @brief Specifies a counter's unit is cycles.
#define CL_PERFORMANCE_COUNTER_UNIT_CYCLES_CODEPLAY 0xA

/// @brief Specifies a counter's result is `cl_int`.
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT32_CODEPLAY 0x0
/// @brief Specifies a counter's result is `cl_long.`
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT64_CODEPLAY 0x1
/// @brief Specifies a counter's result is `cl_uint`.
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT32_CODEPLAY 0x2
/// @brief Specifies a counter's result is `cl_ulong`.
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT64_CODEPLAY 0x3
/// @brief Specifies a counter's result is `cl_float`.
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT32_CODEPLAY 0x4
/// @brief Specifies a counter's result is `cl_double`.
#define CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT64_CODEPLAY 0x5

/// @brief Defines the type of a performance counter unit token.
typedef cl_int cl_performance_counter_unit_codeplay;
/// @brief Defines the type of a performance counter storage token.
typedef cl_int cl_performance_counter_storage_codeplay;

/// @brief Defines a performance counter's properties.
typedef struct cl_performance_counter_codeplay {
  /// @brief Defines the unit of measurement.
  cl_performance_counter_unit_codeplay unit;
  /// @brief Defines the storage type.
  cl_performance_counter_storage_codeplay storage;
  /// @brief Defines the unique identifier.
  cl_uint uuid;
  /// @brief Defines the name.
  char name[256];
  /// @brief Defines the category.
  char category[256];
  /// @brief Defines the description.
  char description[256];
} cl_performance_counter;

/// @brief Describes a performance counter to enable.
typedef struct cl_performance_counter_desc_codeplay {
  /// @brief The unique identifier to enable.
  cl_uint uuid;
  /// @brief Optional extra data, may be NULL.
  void *data;
} cl_performance_counter_desc_codeplay;

/// @brief Describes a set of performance counters to configure.
typedef struct cl_performance_counter_config_codeplay {
  /// @brief The number of elements in the `descs` array.
  cl_uint count;
  /// @brief Array of performance counter to enable.
  cl_performance_counter_desc_codeplay *descs;
} cl_performance_counter_config_codeplay;

/// @brief Contains a performance counter result.
///
/// The union member the result is stored in is defined by the value of
/// the `storage` member of `cl_performance_counter_codeplay` of the associated
/// enabled performance counter.
typedef struct cl_performance_counter_result_codeplay {
  union {
    cl_int int32;       ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT32_CODEPLAY
    cl_long int64;      ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT64_CODEPLAY
    cl_uint uint32;     ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT32_CODEPLAY
    cl_ulong uint64;    ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT64_CODEPLAY
    cl_float float32;   ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT32_CODEPLAY
    cl_double float64;  ///< CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT64_CODEPLAY
  };
} cl_performance_counter_result_codeplay;

/******************
 * cl_codeplay_wfv *
 ******************/

/// @brief Accepted as `param_name` parameter to `clGetKernelWFVInfoCODEPLAY`.
#define CL_KERNEL_WFV_STATUS_CODEPLAY 0x1
#define CL_KERNEL_WFV_WIDTHS_CODEPLAY 0x2

/// @brief Indicates that whole function vectorization succeeded.
#define CL_WFV_SUCCESS_CODEPLAY 0
/// @brief Indicates that whole function vectorization has not been performed.
/// This status will always be returned if the underlying program was created
/// with `clCreateProgramWithBinary`.
#define CL_WFV_NONE_CODEPLAY -1
/// @brief Indicates that whole function vectorization generated an error.
#define CL_WFV_ERROR_CODEPLAY -2

/// @brief Defines the param_name of a whole function vectorization query.
typedef cl_uint cl_kernel_wfv_info_codeplay;

/// @brief Defines the status of whole function vectorization.
typedef cl_int cl_kernel_wfv_status_codeplay;

/// @brief The function is used to query whole function vectorization
/// information for a kernel, given a specified device and local work sizes.
///
/// @param[in] kernel The kernel object. It must not be NULL.
/// @param[in] device The device object. If the list of devices associated with
/// the kernel is a single device, it can be NULL.
/// @param[in] work_dim The number of dimensions used to specify the work-items
/// in the work-group. It must be greater than zero and less than or equal to
/// CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS.
/// @param[in] global_work_size Array of work_dim unsigned values that describe
/// the intended number of global work-items in work_dim dimensions. If it is
/// NULL, it will not be taken into account in calculations or validity checks.
/// @param[in] local_work_size Array of work_dim unsigned values that describe
/// the number of work-items that make up the intended work-group. If it is
/// NULL, the OpenCL runtime may choose a work-group size. If non-uniform
/// work-groups are supported, a uniform or non-uniform work-group size may be
/// chosen.
/// @param[in] param_name The whole function vectorization information to query.
/// @param[out] param_value The result of the query. If it is NULL, it will be
/// ignored and no result will be returned for the query.
/// @param[in] param_value_size The size in bytes of param_value. It must be >=
/// the size of the return type for the query.
/// @param[out] param_value_size_ret The actual size in bytes of the result of
/// the query. If it is NULL, it will be ignored.
///
/// @return CL_SUCCESS if the function is executed successfully.
/// Otherwise, it returns one of the following errors:
/// * CL_INVALID_DEVICE if device is not in the list of devices associated with
/// kernel or if device is NULL but there is more than one device associated
/// with kernel.
/// * CL_INVALID_KERNEL if kernel is not a valid kernel object.
/// * CL_INVALID_VALUE if param_name is not valid, or if size in bytes specified
/// by param_value_size is < size of return type for the specified query.
/// * CL_INVALID_WORK_DIMENSION if work_dim is not a valid value (i.e. a value
/// between 1 and CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS).
/// * CL_INVALID_GLOBAL_WORK_SIZE if any of global_work_size[0], ...,
/// global_work_size[work_dim - 1] exceed the maximum value representable by
/// size_t.
/// * CL_INVALID_GLOBAL_WORK_SIZE if any of global_work_size[0], ...,
/// global_work_size[work_dim - 1] are equal to zero when the OpenCL version is
/// less than 2.1.
/// version is not equal to or greater than 2.1.
/// * CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and does not
/// match the required work-group size for kernel in the program source.
/// * CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and is not
/// consistent with the required number of sub-groups for kernel in the program
/// source.
/// * CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and the total
/// number of work-items in the work-group computed as local_work_size[0] x ...
/// local_work_size[work_dim - 1] is greater than CL_KERNEL_WORK_GROUP_SIZE.
/// * CL_INVALID_WORK_GROUP_SIZE if the work-group size must be uniform and
/// local_work_size is not NULL, or is not equal to the required work-group size
/// specified in the kernel source.
/// * CL_INVALID_WORK_GROUP_SIZE if the number of work-items specified in any of
/// local_work_size[0], ... local_work_size[work_dim - 1] is equal to zero.
/// * CL_INVALID_WORK_ITEM_SIZE if the number of work-items specified in any of
/// local_work_size[0], ... local_work_size[work_dim - 1] is greater than the
/// corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], ...
/// CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1].
/// * CL_OUT_OF_RESOURCES if there is a failure to allocwate resources required
/// by the OpenCL implementation on the device.
/// * CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required
/// by the OpenCL implementation on the host.
extern cl_int CL_API_CALL clGetKernelWFVInfoCODEPLAY(
    cl_kernel kernel, cl_device_id device, cl_uint work_dim,
    const size_t *global_work_size, const size_t *local_work_size,
    cl_kernel_wfv_info_codeplay param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret);

typedef cl_int(CL_API_CALL *clGetKernelWFVInfoCODEPLAY_fn)(
    cl_kernel kernel, cl_device_id device, cl_uint work_dim,
    const size_t *global_work_size, const size_t *local_work_size,
    cl_kernel_wfv_info_codeplay param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // CL_EXT_CODEPLAY_H_INCLUDED
