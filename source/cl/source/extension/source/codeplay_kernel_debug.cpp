// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cl/device.h>
#include <extension/codeplay_kernel_debug.h>

extension::codeplay_kernel_debug::codeplay_kernel_debug()
    : extension("cl_codeplay_kernel_debug",
#ifdef OCL_EXTENSION_cl_codeplay_kernel_debug
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_kernel_debug::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // Intercept device queries so that CL_DEVICE_EXTENSIONS will not contain
  // cl_codeplay_kernel_debug if the device does not support it.
  if (!device->compiler_available || !device->compiler_info->kernel_debug) {
    return CL_INVALID_VALUE;
  }
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}
