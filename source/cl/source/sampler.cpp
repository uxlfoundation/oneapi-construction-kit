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

#include <cargo/attributes.h>
#include <cl/macros.h>
#include <cl/sampler.h>
#include <cl/validate.h>
#include <libimg/host.h>
#include <stddef.h>
#include <stdint.h>
#include <tracer/tracer.h>

_cl_sampler::_cl_sampler(cl_context context, cl_bool normalized_coords,
                         cl_addressing_mode addressing_mode,
                         cl_filter_mode filter_mode)
    : base<_cl_sampler>(cl::ref_count_type::EXTERNAL),
      context(context),
      normalized_coords(normalized_coords),
      addressing_mode(addressing_mode),
      filter_mode(filter_mode) {
  sampler_value = libimg::HostCreateSampler(normalized_coords, addressing_mode,
                                            filter_mode);
}

CL_API_ENTRY cl_sampler CL_API_CALL
cl::CreateSampler(cl_context context, cl_bool normalized_coords,
                  cl_addressing_mode addressing_mode,
                  cl_filter_mode filter_mode, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateSampler");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return NULL);
  // Validate input values.
  switch (normalized_coords) {
    default:
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
      return NULL;
    case CL_TRUE:
      [[fallthrough]];
    case CL_FALSE:
      break;
  }
  switch (addressing_mode) {
    default:
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
      return NULL;
    case CL_ADDRESS_MIRRORED_REPEAT:
      [[fallthrough]];
    case CL_ADDRESS_REPEAT:
      OCL_CHECK(CL_TRUE != normalized_coords,
                OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
                return NULL);
      [[fallthrough]];
    case CL_ADDRESS_CLAMP_TO_EDGE:
      [[fallthrough]];
    case CL_ADDRESS_CLAMP:
      [[fallthrough]];
    case CL_ADDRESS_NONE:
      break;
  }
  switch (filter_mode) {
    default:
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
      return NULL;
    case CL_FILTER_NEAREST:  // Intentional fall-through.
      [[fallthrough]];
    case CL_FILTER_LINEAR:
      break;
  }
  // Validate iamge support.
  const cl_int err = cl::validate::ImageSupportForAnyDevice(context);
  OCL_CHECK(CL_SUCCESS != err, OCL_SET_IF_NOT_NULL(errcode_ret, err);
            return nullptr);

  _cl_sampler *sampler =
      new _cl_sampler(context, normalized_coords, addressing_mode, filter_mode);
  OCL_CHECK(!sampler, OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
            return NULL);

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);

  return sampler;
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainSampler(cl_sampler sampler) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainSampler");
  OCL_CHECK(!sampler, return CL_INVALID_SAMPLER);
  return cl::retainExternal(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseSampler(cl_sampler sampler) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseSampler");
  OCL_CHECK(!sampler, return CL_INVALID_SAMPLER);
  return cl::releaseExternal(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetSamplerInfo(
    cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetSamplerInfo");
  OCL_CHECK(!sampler, return CL_INVALID_SAMPLER);
#define SAMPLER_INFO_CASE(TYPE, SIZE_RET, POINTER, VALUE)            \
  case TYPE: {                                                       \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);             \
    OCL_CHECK(param_value &&param_value_size < SIZE_RET,             \
              return CL_INVALID_VALUE);                              \
    OCL_SET_IF_NOT_NULL((static_cast<POINTER>(param_value)), VALUE); \
  } break

  switch (param_name) {
    default: {
      return extension::GetSamplerInfo(sampler, param_name, param_value_size,
                                       param_value, param_value_size_ret);
    }
      SAMPLER_INFO_CASE(CL_SAMPLER_REFERENCE_COUNT, sizeof(cl_uint), cl_uint *,
                        sampler->refCountExternal());
      SAMPLER_INFO_CASE(CL_SAMPLER_CONTEXT, sizeof(_cl_context *),
                        _cl_context **, sampler->context);
      SAMPLER_INFO_CASE(CL_SAMPLER_NORMALIZED_COORDS, sizeof(cl_bool),
                        cl_bool *, sampler->normalized_coords);
      SAMPLER_INFO_CASE(CL_SAMPLER_ADDRESSING_MODE, sizeof(cl_addressing_mode),
                        cl_addressing_mode *, sampler->addressing_mode);
      SAMPLER_INFO_CASE(CL_SAMPLER_FILTER_MODE, sizeof(cl_filter_mode),
                        cl_filter_mode *, sampler->filter_mode);
  }
#undef SAMPLER_INFO_CASE

  return CL_SUCCESS;
}
