// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_WINDOWS_H_INCLUDED
#define CL_WINDOWS_H_INCLUDED

#if defined(__MINGW32__) || defined(__MINGW64__)
// For MinGW cross-compile builds from Linux to work-around headers being
// case-sensitive on Linux but not Windows.
#include <windows.h>
#endif

#endif  // CL_WINDOWS_H_INCLUDED
