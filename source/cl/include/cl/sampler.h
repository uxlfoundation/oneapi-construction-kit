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
/// @brief Definitions of the OpenCL sampler API.

#ifndef CL_SAMPLER_H_INCLUDED
#define CL_SAMPLER_H_INCLUDED

#include <CL/cl.h>
#include <cl/base.h>

/// @addtogroup cl
/// @{

/// @brief Definition of OpenCL API object _cl_sampler.
struct _cl_sampler final : public cl::base<_cl_sampler> {
  /// @brief Sampler constructor.
  ///
  /// @param[in] context Context the sampler belongs to.
  /// @param[in] normalised_coords Enable or disable normalized coordinates.
  /// @param[in] addressing_mode Specify out of range coordinate behaviour.
  /// @param[in] filter_mode Specify type of read image filter.
  _cl_sampler(cl_context context, cl_bool normalised_coords,
              cl_addressing_mode addressing_mode, cl_filter_mode filter_mode);

  _cl_sampler() = delete;

  /// @brief Context the sampler belongs to.
  cl_context context;
  /// @brief Enable or disable normalized coordinates.
  cl_bool normalized_coords;
  /// @brief Specify out of range coordinate behavior.
  cl_addressing_mode addressing_mode;
  /// @brief Specify type of read image filter.
  cl_filter_mode filter_mode;
  /// @brief Combined sampler bit field value passed to kernels.
  cl_uint sampler_value;
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL sampler object.
///
/// @param[in] context Context the sampler belong to.
/// @param[in] normalized_coords Enable or disable normalized coordinates.
/// @param[in] addressing_mode Specify out of range coordinate behavior.
/// @param[in] filter_mode Specify type of read image filter.
/// @param[out] errcode_ret Return error code.
///
/// @return Return the new sampler object.
///
/// @sa
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateSampler.html
CL_API_ENTRY cl_sampler CL_API_CALL
CreateSampler(cl_context context, cl_bool normalized_coords,
              cl_addressing_mode addressing_mode, cl_filter_mode filter_mode,
              cl_int *errcode_ret);

/// @brief Increment the samplers reference count.
///
/// @param[in] sampler Sampler to increment reference count on.
///
/// @return Return error code.
///
/// @sa
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainSampler.html
CL_API_ENTRY cl_int CL_API_CALL RetainSampler(cl_sampler sampler);

/// @brief Decrement the samplers reference count.
///
/// @param sampler Sampler to decrement reference count on.
///
/// @return Return error code.
///
/// @sa
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseSampler.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseSampler(cl_sampler sampler);

/// @brief Query the sampler information from.
///
/// @param sampler Sampler to query.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of param_value storage.
/// @param param_value Pointer to value to store query in.
/// @param param_value_size_ret Return size in bytes the query requires.
///
/// @return Return error code.
///
/// @sa
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetSamplerInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetSamplerInfo(cl_sampler sampler,
                                               cl_sampler_info param_name,
                                               size_t param_value_size,
                                               void *param_value,
                                               size_t *param_value_size_ret);

/// @}
}  // namespace cl

#endif  // CL_SAMPLER_H_INCLUDED
