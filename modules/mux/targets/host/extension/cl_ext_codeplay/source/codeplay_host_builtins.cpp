// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cl/config.h>
#include <cl/device.h>
#include <extension/codeplay_host_builtins.h>

extension::codeplay_host_builtins::codeplay_host_builtins()
    : extension("cl_codeplay_host_builtins",
#ifdef OCL_EXTENSION_cl_codeplay_host_builtins
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_host_builtins::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // This extension is only valid on a ComputeAorta host CPU device.
  if ((0 != std::strncmp(CA_HOST_CL_DEVICE_NAME_PREFIX,
                         device->mux_device->info->device_name,
                         std::strlen(CA_HOST_CL_DEVICE_NAME_PREFIX))) ||
      (mux_device_type_cpu != device->mux_device->info->device_type)) {
    return CL_INVALID_DEVICE;
  }

  // The base class's GetDeviceInfo handles CL_DEVICE_EXTENSIONS and
  // CL_DEVICE_EXTENSIONS_WITH_VERSION queries.
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}
