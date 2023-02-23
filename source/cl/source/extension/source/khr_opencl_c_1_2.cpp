// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl.h>

#include <cl/config.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <extension/khr_opencl_c_1_2.h>

#include <cstring>

#if !defined(OCL_EXTENSION_cl_khr_opencl_c_1_2)
#error Default OpenCL C extensions must not be turned off.
#endif

extension::khr_opencl_c_1_2::khr_opencl_c_1_2()
    : extension("cl_khr_opencl_c_1_2",
                usage_category::DEVICE CA_CL_EXT_VERSION(1, 0, 0)) {}

cl_int extension::khr_opencl_c_1_2::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret) const {
  OCL_CHECK(nullptr == device, return CL_INVALID_DEVICE);
  switch (param_name) {
    default: { return CL_INVALID_VALUE; }
    case CL_DEVICE_EXTENSIONS: {
      std::string extension_names;
      extension_names += "cl_khr_global_int32_base_atomics ";
      extension_names += "cl_khr_global_int32_extended_atomics ";
      extension_names += "cl_khr_local_int32_base_atomics ";
      extension_names += "cl_khr_local_int32_extended_atomics ";
      extension_names += "cl_khr_byte_addressable_store ";

      if (device->profile == "EMBEDDED_PROFILE") {
        if (device->mux_device->info->integer_capabilities &
            mux_integer_capabilities_64bit) {
          extension_names += "cles_khr_int64 ";
        }

        // GCOVR_EXCL_START non-deterministically executed
        if (device->mux_device->info->image2d_array_writes) {
          extension_names += "cles_khr_2d_image_array_writes ";
        }
        // GCOVR_EXCL_STOP
      }

      // Only enable these extensions if they are supported by the device.
      if (device->double_fp_config) {
        extension_names += "cl_khr_fp64 ";
      }
      if (device->image3d_writes) {
        extension_names += "cl_khr_3d_image_writes ";
      }

      // Remove the trailing space.
      extension_names.pop_back();

      OCL_CHECK(
          nullptr != param_value && param_value_size < extension_names.size(),
          return CL_INVALID_VALUE);

      if (param_value) {
        std::strncpy((char*)param_value, extension_names.data(),
                     extension_names.size());
      }

      OCL_SET_IF_NOT_NULL(param_value_size_ret, extension_names.size() + 1);
      break;
    }
#if defined(CL_VERSION_3_0)
    case CL_DEVICE_EXTENSIONS_WITH_VERSION: {
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
    }
#endif
  }
  return CL_SUCCESS;
}
