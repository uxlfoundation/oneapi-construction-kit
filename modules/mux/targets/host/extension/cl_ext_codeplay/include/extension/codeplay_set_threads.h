// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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

  virtual void *
  GetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                         const char *func_name) const override;

}; // class codeplay_set_threads
} // namespace extension

#endif // EXTENSION_CODEPLAY_SET_THREADS_INCLUDED
