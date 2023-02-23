// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_SET_THREADS_INCLUDED
#define EXTENSION_CODEPLAY_SET_THREADS_INCLUDED

#include <extension/extension.h>

namespace extension {

class codeplay_set_threads final : public extension {
 public:
  codeplay_set_threads();

  virtual cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) const override;

  virtual void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

};  // class codeplay_set_threads
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_SET_THREADS_INCLUDED
