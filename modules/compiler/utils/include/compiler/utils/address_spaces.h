// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// LLVM address space identifiers.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ADDRESS_SPACES_H_INCLUDED
#define COMPILER_UTILS_ADDRESS_SPACES_H_INCLUDED

namespace compiler {
namespace utils {
enum AddressSpace {
  Private = 0,
  Global = 1,
  Constant = 2,
  Local = 3,
  Generic = 4,
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ADDRESS_SPACES_H_INCLUDED
