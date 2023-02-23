// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Utility macros used to implement the OpenCL API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_MACROS_H_INCLUDED
#define CL_MACROS_H_INCLUDED

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
/// @brief Assert that a condition is true or display message and abort.
///
/// Much like the C assert standard library macro OCL_ASSERT checks `CONDITION`
/// to verify that it is true before continuing execution. If `CONDITION` is
/// false then @p MESSAGE is displayed along with the source location before
/// aborting execution.
///
/// @param CONDITION Condition to test.
/// @param MESSAGE Message to display on assertion failure.
#define OCL_ASSERT(CONDITION, MESSAGE)                                     \
  if (!(CONDITION)) {                                                      \
    std::fprintf(stderr, "%s:%d: %s %s\n", __FILE__, __LINE__, #CONDITION, \
                 MESSAGE);                                                 \
    std::abort();                                                          \
  }                                                                        \
  (void)0
#else
#define OCL_ASSERT(CONDITION, MESSAGE) (void)0
#endif

/// @brief Display a message to stderr and abort.
///
/// @param MESSAGE Message to display prior to aborting.
#define OCL_ABORT(MESSAGE)                                            \
  {                                                                   \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, MESSAGE); \
    std::abort();                                                     \
  }                                                                   \
  (void)0

/// @brief      OCL_CHECK is mainly used to perform OpenCL API parameter
///             validation checking.
/// @param      CONDITION - A bool condition.
/// @param      ACTION - The code to execute on bool condition being true.
#define OCL_CHECK(CONDITION, ACTION) \
  if (CONDITION) {                   \
    ACTION;                          \
  }                                  \
  (void)0

/// @brief      OCL_SET_IF_NOT_NULL is mainly used to check OpenCL API
///             parameters when pointer types to know to return a value
///             via that object if given.
/// @param      POINTER - A bool condition compare pointer to NULL.
/// @param      VALUE - Assign the object pointed to by the pointer a value.
#define OCL_SET_IF_NOT_NULL(POINTER, VALUE) \
  if ((POINTER) != nullptr) {               \
    *(POINTER) = VALUE;                     \
  }                                         \
  (void)0

#define OCL_UNUSED(x) ((void)(x))

/// @brief Extension version when in OpenCL-3.0 mode.
///
/// Useful macro for reducing boilerplate when conditionally defining
/// extension versions.
#if defined(CL_VERSION_3_0)
#define CA_CL_EXT_VERSION(MAJOR, MINOR, PATCH) \
  , CL_MAKE_VERSION_KHR(MAJOR, MINOR, PATCH)
#else
#define CA_CL_EXT_VERSION(MAJOR, MINOR, PATCH)
#endif

#endif  // CL_MACROS_H_INCLUDED
