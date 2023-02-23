// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl_ext_codeplay_host.h>  // Customer library header
#include <cl/config.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <extension/codeplay_set_threads.h>  // This extension's header
#include <host/device.h>

#include <cstring>

extension::codeplay_set_threads::codeplay_set_threads()
    : extension("cl_codeplay_set_threads",
#ifdef OCL_EXTENSION_cl_codeplay_set_threads
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_set_threads::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // This extension is only valid on a ComputeAorta host CPU device.
  // If you are using this function as a template for a new extension, then
  // this if statement **must** be updated to return an error if `device` isn't
  // your device.
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

void *extension::codeplay_set_threads::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) const {
  OCL_UNUSED(platform);
#ifndef OCL_EXTENSION_cl_codeplay_set_threads
  OCL_UNUSED(func_name);
  return nullptr;
#else
  OCL_CHECK(nullptr == func_name, return nullptr);
  if (0 == strcmp("clSetNumThreadsCODEPLAY", func_name)) {
    return (void *)&clSetNumThreadsCODEPLAY;
  }

  return nullptr;
#endif
}

cl_int CL_API_CALL clSetNumThreadsCODEPLAY(cl_device_id device,
                                           cl_uint max_threads) {
  // If you are using this function as a template for a new extension, then you
  // can safely discard the contents of this function. Make sure to update the
  // function parameters both here and in your extension library header.

  OCL_UNUSED(max_threads);

  if (nullptr == device) {
    return CL_INVALID_DEVICE_TYPE;
  }

  // This extension is only valid on a ComputeAorta host CPU device.
  if ((0 != std::strncmp(CA_HOST_CL_DEVICE_NAME_PREFIX,
                         device->mux_device->info->device_name,
                         std::strlen(CA_HOST_CL_DEVICE_NAME_PREFIX))) ||
      (mux_device_type_cpu != device->mux_device->info->device_type)) {
    return CL_INVALID_DEVICE_TYPE;
  }

  if (0 == max_threads) {
    return CL_INVALID_VALUE;
  }

  // TODO: CA-1136 -- Implement this function
  return CL_DEVICE_NOT_AVAILABLE;
}
