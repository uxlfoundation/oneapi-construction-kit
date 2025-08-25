// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CL_WINDOWS_H_INCLUDED
#define CL_WINDOWS_H_INCLUDED

#if defined(__MINGW32__) || defined(__MINGW64__)
// For MinGW cross-compile builds from Linux to work-around headers being
// case-sensitive on Linux but not Windows.
#include <windows.h>
#endif

#endif  // CL_WINDOWS_H_INCLUDED
