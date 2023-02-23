// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Contains helper function to map PAPI error codes onto mux result
/// codes.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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

}  // namespace host
#endif
