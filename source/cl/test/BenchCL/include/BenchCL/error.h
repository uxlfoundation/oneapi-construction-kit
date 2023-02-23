// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Base class and reference counter API for all OpenCL API objects.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BENCHCL_ERROR_H_INCLUDED
#define BENCHCL_ERROR_H_INCLUDED

// ASSERT_EQ_ERRCODE is so-named to match the same macro in UnitCL.  The
// 'STATUS' parameter is always evaluated, but the returned error code is only
// compared with 'EXPECTED' (via 'assert') in assert builds.  This is because
// OpenCL functions are used within constructors and destructors in BenchCL,
// there is nothing that can be done with error codes in general.
#ifndef NDEBUG
#define ASSERT_EQ_ERRCODE(EXPECTED, STATUS) assert((EXPECTED) == (STATUS))
#else
#define ASSERT_EQ_ERRCODE(EXPECTED, STATUS) (void)(STATUS)
#endif

#endif  // BENCHCL_ERROR_H_INCLUDED
