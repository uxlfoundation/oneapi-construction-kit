// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl_ext.h>

#include <cl/config.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <extension/khr_fp16.h>

#include <cstring>

extension::khr_fp16::khr_fp16()
    : extension("cl_khr_fp16",
#ifdef OCL_EXTENSION_cl_khr_fp16
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}

cl_int extension::khr_fp16::GetDeviceInfo(cl_device_id device,
                                          cl_device_info param_name,
                                          size_t param_value_size,
                                          void *param_value,
                                          size_t *param_value_size_ret) const {
  // Only report extension and half config if target device supports half types
  // and the extension wasn't disabled in cmake.
  const cl_device_fp_config device_support = device->half_fp_config;
  if (0 == device_support || usage_category::DISABLED == usage) {
    return CL_INVALID_VALUE;
  }

  if (CL_DEVICE_HALF_FP_CONFIG == param_name) {
    const size_t value_size = sizeof(device_support);
    OCL_CHECK(nullptr != param_value && param_value_size < value_size,
              return CL_INVALID_VALUE);

    if (nullptr != param_value) {
      std::memcpy(param_value, &device_support, value_size);
    }

    OCL_SET_IF_NOT_NULL(param_value_size_ret, value_size);
    return CL_SUCCESS;
  }

  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}
