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
/// @brief Contains helper function to map PAPI error codes onto mux result
/// codes.

#ifndef HOST_PAPI_ERROR_CODES_H_INCLUDED
#define HOST_PAPI_ERROR_CODES_H_INCLUDED

#include <mux/mux.h>
#include <papi.h>

namespace host {

/// @brief Attempts to return an equivalent Mux error code for the given PAPI
/// error code.
inline mux_result_t getMuxResult(const int papi_code) {
  switch (papi_code) {
  case PAPI_OK:
    return mux_success;
  case PAPI_EINVAL:
  case PAPI_ENOEVST:
  case PAPI_ENOTPRESET:
  case PAPI_EATTR:
  case PAPI_ECOUNT:
  case PAPI_ECOMBO:
    return mux_error_invalid_value;
  case PAPI_ENOMEM:
  case PAPI_EBUF:
    return mux_error_out_of_memory;
  case PAPI_ESBSTR:
  case PAPI_ENOCNTR:
  case PAPI_ENOSUPP:
  case PAPI_ENOIMPL:
    return mux_error_feature_unsupported;
  default:
    return mux_error_failure;
  }
}

} // namespace host
#endif
