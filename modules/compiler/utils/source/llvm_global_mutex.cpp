// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/llvm_global_mutex.h>

std::mutex &compiler::utils::getLLVMGlobalMutex() {
  static std::mutex mutex;
  return mutex;
}
