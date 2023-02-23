// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Extension cl_khr_opencl_c_1_2 API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED
#define EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Default extensions associated with OpenCL 1.2.
///
/// If enabled, then ClGetPlatformInfo queries for CL_PLATFORM_EXTENSIONS return
/// the following names as the query value:
/// * `cl_khr_global_int32_base_atomics`
/// * `cl_khr_global_int32_extended_atomics`
/// * `cl_khr_local_int32_base_atomics`
/// * `cl_khr_local_int32_extended_atomics`
/// * `cl_khr_byte_addressable_store`
/// * `cl_khr_fp64` is only included if the queried device supports double
///   precision floating point types.
class khr_opencl_c_1_2 final : public extension {
 public:
  /// @brief Default constructor.
  khr_opencl_c_1_2();

  /// @brief Queries for extension provided device info.
  ///
  /// If extension is enabled, then ClGetDeviceInfo queries for
  /// CL_DEVICE_EXTENSIONS return the following query value:
  /// * `cl_khr_global_int32_base_atomics`
  /// * `cl_khr_global_int32_extended_atomics`
  /// * `cl_khr_local_int32_base_atomics`
  /// * `cl_khr_local_int32_extended_atomics`
  /// * `cl_khr_byte_addressable_store`
  /// * `cl_khr_fp64` is only included if the queried device supports double
  ///   precision floating point types.
  ///
  /// @see ocl::extension::Extension::ClGetDeviceInfo for more detailed
  /// explanations.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for. If extension is enabled,
  /// then ClGetDeviceInfo queries for CL_DEVICE_EXTENSIONS return the names
  /// listed above.
  /// @param[in] param_value_size Size in bytes of the memory area param_value
  /// points to.
  /// @param[in] param_value Memory area to copy the queried info in or nullptr
  /// to not receive the info value.
  /// @param[out] param_value_size_ret References variable where to store the
  /// size of the info value or nullptr to not receive the info value.
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED
