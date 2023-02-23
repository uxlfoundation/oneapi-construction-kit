// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_ERROR_H_INCLUDED
#define VK_ERROR_H_INCLUDED

#include <compiler/result.h>
#include <mux/mux.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

/// @brief Display MESSAGE then abort due to catastrophic failure.
///
/// @param MESSAGE Message to display.
#define VK_ABORT(MESSAGE)                                        \
  {                                                              \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, MESSAGE); \
    abort();                                                     \
  }

/// @brief Check CONDITION and display MESSAGE then abort due to exceptional
/// failure.
///
/// @param CONDITION Condition that must be true.
/// @param MESSAGE Message to display when condition is false.
#define VK_ASSERT(CONDITION, MESSAGE) \
  {                                   \
    if (!(CONDITION)) {               \
      VK_ABORT(MESSAGE);              \
    }                                 \
  }

namespace vk {

/// @brief Function to translate mux_result values into VkResults
///
/// @param error Mux result to translate
VkResult getVkResult(mux_result_t error);

/// @brief Function to translate compiler result values into VkResults
///
/// @param error Mux compiler result to translate
VkResult getVkResult(compiler::Result error);

}  // namespace vk

#endif  // VK_ERROR_H_INCLUDED
