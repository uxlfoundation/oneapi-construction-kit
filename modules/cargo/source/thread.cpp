// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "cargo/thread.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <array>
#include <cstdlib>

#if CARGO_HAS_PTHREAD_SETNAME_NP || CARGO_HAS_PTHREAD_GETNAME_NP
#include <pthread.h>
#endif

#include <cstring>

cargo::result cargo::thread::set_name(const std::string &name) noexcept {
#ifdef _WIN32
  constexpr size_t len = 1024;
  std::array<wchar_t, len> wname{};
  std::mbstowcs(wname.data(), name.c_str(), len);
  if (FAILED(SetThreadDescription(native_handle(), wname.data()))) {
    return cargo::unknown_error;
  }
#elif CARGO_HAS_PTHREAD_SETNAME_NP
  switch (pthread_setname_np(native_handle(), name.c_str())) {
    case 0:
      break;
    case ERANGE:
      return cargo::out_of_bounds;
    default:
      return cargo::unknown_error;
  }
#else
  (void)name;
  return cargo::unsupported;
#endif
  return cargo::success;
}

[[nodiscard]] cargo::error_or<std::string> cargo::thread::get_name() noexcept {
#if defined(_WIN32) || CARGO_HAS_PTHREAD_GETNAME_NP
  constexpr size_t len = 1024;
  std::array<char, len> buffer;
#ifdef _WIN32
  std::array<wchar_t, len> wbuffer;
  auto *wbufptr = wbuffer.data();
  if (FAILED(GetThreadDescription(native_handle(), &wbufptr))) {
    return cargo::unknown_error;
  }
  std::wcstombs(buffer.data(), wbuffer.data(), len);
#elif CARGO_HAS_PTHREAD_GETNAME_NP
  if (pthread_getname_np(native_handle(), buffer.data(), len)) {
    return cargo::unknown_error;
  }
#endif
  return {buffer.data(), std::strlen(buffer.data())};
#else
  return cargo::unsupported;
#endif  // defined(_WIN32) || CARGO_HAS_PTHREAD_GETNAME_NP
}
