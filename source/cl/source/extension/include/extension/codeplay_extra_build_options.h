// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of cl_codeplay_extra_build_options extension.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_EXTRA_BUILD_OPTIONS_H_INCLUDED
#define EXTENSION_CODEPLAY_EXTRA_BUILD_OPTIONS_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_extra_build_options extension.
class codeplay_extra_build_options final : public extension {
 public:
  /// @brief Default constructor.
  codeplay_extra_build_options();
};

/// @}
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_EXTRA_BUILD_OPTIONS_H_INCLUDED
