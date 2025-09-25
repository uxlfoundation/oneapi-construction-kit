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

/// @file
///
/// @brief Clang Thread Safety Analysis Attribute Detection

#ifndef CARGO_THREAD_SAFETY_H_INCLUDED
#define CARGO_THREAD_SAFETY_H_INCLUDED

#ifdef __clang__
#define CARGO_TS_ATTRIBUTE(x) __attribute__((x))
#else
#define CARGO_TS_ATTRIBUTE(x)
#endif

#define CARGO_TS_CAPABILITY(x) CARGO_TS_ATTRIBUTE(capability(x))

#define CARGO_TS_SCOPED_CAPABILITY CARGO_TS_ATTRIBUTE(scoped_lockable)

#define CARGO_TS_GUARDED_BY(x) CARGO_TS_ATTRIBUTE(guarded_by(x))

#define CARGO_TS_PT_GUARDED_BY(x) CARGO_TS_ATTRIBUTE(pt_guarded_by(x))

#define CARGO_TS_ACQUIRED_BEFORE(...) \
  CARGO_TS_ATTRIBUTE(acquired_before(__VA_ARGS__))

#define CARGO_TS_ACQUIRED_AFTER(...) \
  CARGO_TS_ATTRIBUTE(acquired_after(__VA_ARGS__))

#define CARGO_TS_REQUIRES(...) \
  CARGO_TS_ATTRIBUTE(requires_capability(__VA_ARGS__))

#define CARGO_TS_REQUIRES_SHARED(...) \
  CARGO_TS_ATTRIBUTE(requires_shared_capability(__VA_ARGS__))

#define CARGO_TS_ACQUIRE(...) \
  CARGO_TS_ATTRIBUTE(acquire_capability(__VA_ARGS__))

#define CARGO_TS_ACQUIRE_SHARED(...) \
  CARGO_TS_ATTRIBUTE(acquire_shared_capability(__VA_ARGS__))

#define CARGO_TS_RELEASE(...) \
  CARGO_TS_ATTRIBUTE(release_capability(__VA_ARGS__))

#define CARGO_TS_RELEASE_SHARED(...) \
  CARGO_TS_ATTRIBUTE(release_shared_capability(__VA_ARGS__))

#define CARGO_TS_RELEASE_GENERIC(...) \
  CARGO_TS_ATTRIBUTE(release_generic_capability(__VA_ARGS__))

#define CARGO_TS_TRY_ACQUIRE(...) \
  CARGO_TS_ATTRIBUTE(try_acquire_capability(__VA_ARGS__))

#define CARGO_TS_TRY_ACQUIRE_SHARED(...) \
  CARGO_TS_ATTRIBUTE(try_acquire_shared_capability(__VA_ARGS__))

#define CARGO_TS_EXCLUDES(...) CARGO_TS_ATTRIBUTE(locks_excluded(__VA_ARGS__))

#define CARGO_TS_ASSERT_CAPABILITY(x) CARGO_TS_ATTRIBUTE(assert_capability(x))

#define CARGO_TS_ASSERT_SHARED_CAPABILITY(x) \
  CARGO_TS_ATTRIBUTE(assert_shared_capability(x))

#define CARGO_TS_RETURN_CAPABILITY(x) CARGO_TS_ATTRIBUTE(lock_returned(x))

#define CARGO_TS_NO_THREAD_SAFETY_ANALYSIS \
  CARGO_TS_ATTRIBUTE(no_thread_safety_analysis)

#endif  // CARGO_THREAD_SAFETY_H_INCLUDED
