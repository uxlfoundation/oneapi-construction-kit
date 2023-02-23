// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of cl_codeplay_soft_math extension.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED
#define EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_soft_math extension.
class codeplay_soft_math final : public extension {
 public:
  /// @brief Default constructor.
  codeplay_soft_math();
};

/// @}
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED
