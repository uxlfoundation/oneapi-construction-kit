// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_HOST_BUILTINS_INCLUDED
#define EXTENSION_CODEPLAY_HOST_BUILTINS_INCLUDED

#include <extension/extension.h>

namespace extension {

class codeplay_host_builtins final : public extension {
 public:
  codeplay_host_builtins();

  virtual cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) const override;

};  // class codeplay_host_builtins
}  // namespace extension
#endif  // EXTENSION_CODEPLAY_HOST_BUILTINS_INCLUDED
