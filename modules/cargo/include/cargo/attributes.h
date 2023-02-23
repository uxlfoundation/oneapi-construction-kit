// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief C++ attribute detection.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_ATTRIBUTE_H_INCLUDED
#define CARGO_ATTRIBUTE_H_INCLUDED

#ifdef __has_cpp_attribute

/// @def CARGO_NODISCARD
/// @brief C++17 attribute `[[nodiscard]]` or compiler specific fallback.
#if __has_cpp_attribute(nodiscard)
#define CARGO_NODISCARD [[nodiscard]]
#elif __has_cpp_attribute(gnu::warn_unused_result)
#define CARGO_NODISCARD [[gnu::warn_unused_result]]
#endif  // __has_cpp_attribute(nodiscard)

/// @def CARGO_FALLTHROUGH
/// @brief C++17 attribute `[[fallthrough]]` or compiler specific fallback.
#if __has_cpp_attribute(fallthrough)
#define CARGO_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define CARGO_FALLTHROUGH [[clang::fallthrough]]
#endif  // __has_cpp_attribute(fallthrough)

/// @def CARGO_REINITIALIZES
/// @brief Clang-specific `[[clang::reinitializes]]` attribute.
#if __has_cpp_attribute(clang::reinitializes)
#define CARGO_REINITIALIZES [[clang::reinitializes]]
#endif  // __has_cpp_attribute(clang::reinitializes)

#endif  // __has_cpp_attribute

// Ensure that all attribute macros are defined even when the attribute is not
// supported by the compiler.

#ifndef CARGO_NODISCARD
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
// GCC 4.8 supports a [[nodiscard]] analogue but does not support
// __has_cpp_attribute so define it here.
#define CARGO_NODISCARD [[gnu::warn_unused_result]]
#else
#define CARGO_NODISCARD
#endif
#endif  // CARGO_NODISCARD

#ifndef CARGO_FALLTHROUGH
#define CARGO_FALLTHROUGH
#endif  // CARGO_FALLTHROUGH

#ifndef CARGO_REINITIALIZES
#define CARGO_REINITIALIZES
#endif  // CARGO_REINITIALIZES

#endif  // CARGO_ATTRIBUTE_H_INCLUDED
