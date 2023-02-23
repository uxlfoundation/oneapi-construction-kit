// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Derived from CC0-licensed `tl::optional` library which can be found
// at https://github.com/TartanLlama/optional.  See optional.LICENSE.txt.

/// @file
///
/// @brief Macros which detect platform capabilities.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.
/// Derived from CC0-licensed `tl::optional` library which can be found
/// at https://github.com/TartanLlama/optional.  See optional.LICENSE.txt.

#ifndef CARGO_PLATFORM_DEFINES_H_INCLUDED
#define CARGO_PLATFORM_DEFINES_H_INCLUDED

#if (defined(_MSC_VER) && _MSC_VER == 1900)
#define CARGO_MSVC2015
#endif

#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 && \
     !defined(__clang__))
#define CARGO_GCC49
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 4 && \
     !defined(__clang__))
#define CARGO_GCC54
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 5 && \
     !defined(__clang__))
#define CARGO_GCC55
#endif

#ifdef CARGO_GCC49
// GCC < 5 doesn't support overloading on const&& for member functions
#define CARGO_NO_CONSTRR
#endif

#if __cplusplus > 201103L
#define CARGO_CXX14
#endif

#if __cplusplus > 201403L
#define CARGO_CXX17
#endif

// constexpr implies const in C++11, not C++14
#if (__cplusplus == 201103L || defined(CARGO_MSVC2015) || defined(CARGO_GCC49))
#define CARGO_CXX14_CONSTEXPR
#else
#define CARGO_CXX14_CONSTEXPR constexpr
#endif

#endif  // CARGO_PLATFORM_DEFINES_H_INCLUDED
