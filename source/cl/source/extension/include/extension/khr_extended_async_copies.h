// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of khr_extended_async_copies extension.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED
#define EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_khr_extended_async_copies extension.
class khr_extended_async_copies final : public extension {
 public:
  /// @brief Default constructor.
  khr_extended_async_copies();
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_EXTENDED_ASYNC_COPIES_H_INCLUDED
