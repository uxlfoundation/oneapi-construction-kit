// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl_ext.h>

#include <cl/config.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/program.h>
#include <extension/khr_spir.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <mutex>

extension::khr_spir::khr_spir()
    : extension("cl_khr_spir",
#ifdef OCL_EXTENSION_cl_khr_spir
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}

cl_int extension::khr_spir::GetDeviceInfo(cl_device_id device,
                                          cl_device_info param_name,
                                          size_t param_value_size,
                                          void *param_value,
                                          size_t *param_value_size_ret) const {
  size_t value_size = 0;
  const char *value = nullptr;

  switch (param_name) {
    case CL_DEVICE_SPIR_VERSIONS: {
      static const char spir_version[] = "1.2";
      value_size = sizeof(spir_version);
      value = spir_version;
      break;
    }
    default: {
      // Use default implementation that uses the name set in the constructor as
      // the name usage specifies.
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
    }
  }

  OCL_CHECK(nullptr != param_value && param_value_size < value_size,
            return CL_INVALID_VALUE);

  if (nullptr != param_value) {
    std::memcpy(param_value, value, value_size);
  }

  OCL_SET_IF_NOT_NULL(param_value_size_ret, value_size);

  return CL_SUCCESS;
}
