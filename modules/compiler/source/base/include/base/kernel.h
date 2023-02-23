// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler kernel API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_KERNEL_H_INCLUDED
#define BASE_KERNEL_H_INCLUDED

#include <cargo/dynamic_array.h>
#include <compiler/kernel.h>

namespace compiler {
class BaseKernel : public Kernel {
 public:
  using Kernel::Kernel;
};
}  // namespace compiler

#endif  // BASE_KERNEL_H_INCLUDED
