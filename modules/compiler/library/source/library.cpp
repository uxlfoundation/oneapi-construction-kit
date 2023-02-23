// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "compiler/library.h"

#include "base/context.h"
#include "compiler/info.h"
#include "mux/utils/id.h"

namespace compiler {
const char *llvmVersion() { return CA_COMPILER_LLVM_VERSION; }

const compiler::Info *getCompilerForDevice(mux_device_info_t device_info) {
  if (mux::objectIsInvalid(device_info)) {
    return nullptr;
  }
  // Ensure that device IDs are initialized.
  uint64_t device_infos_length;
  if (mux_success != muxGetDeviceInfos(mux_device_type_all, 0, nullptr,
                                       &device_infos_length)) {
    return nullptr;
  }
  for (const compiler::Info *info : compilers()) {
    if (info->device_info->id == device_info->id) {
      return info;
    }
  }
  return nullptr;
}

std::unique_ptr<Context> createContext() {
  return std::unique_ptr<Context>{new BaseContext};
}
}  // namespace compiler
