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

#include <cargo/array_view.h>
#include <cargo/string_algorithm.h>

#include <algorithm>
#include <array>
#include <memory>

#if __linux || __APPLE__
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#endif

namespace {
struct malloc_deleter {
  void operator()(void *ptr) { std::free(ptr); }
};

template <class T>
using unique_malloc_ptr = std::unique_ptr<T, malloc_deleter>;
}  // namespace

namespace debug {
void print_backtrace(FILE *out) {
#if __linux__ || __APPLE__
  std::array<void *, 512> callstack;
  int frame_count = backtrace(callstack.data(), callstack.size());
  unique_malloc_ptr<char *> frames{
      backtrace_symbols(callstack.data(), frame_count)};

  std::array<char, 4096> buffer;
  cargo::string_view curdir{getcwd(buffer.data(), buffer.size())};

  for (int frame_index = 1; frame_index < frame_count; frame_index++) {
    cargo::string_view frame{frames.get()[frame_index]};

#if __linux__
    // Each string representing a frame returned from backtrace_symbols takes
    // the following form: `<file>(<name>[+<offset>]) [<address>]`
    auto file = frame;
    file.remove_suffix(file.size() - file.find('('));
    auto addr = frame;
    addr.remove_prefix(std::min(addr.find('[') + 1, addr.size()));
    addr.remove_suffix(addr.size() - addr.rfind(']'));
    auto name = frame;
    name.remove_prefix(std::min(name.find('(') + 1, name.size()));
    // + may not be present, to use ) as the upper bound
    name.remove_suffix(name.size() - name.find_first_of("+)"));
#elif __APPLE__
    // Each string representing a frame returned from backtrace_symbols takes
    // the following form: frame: `<number> <file> <address> <name> + <offset>`
    auto parts = cargo::split(frame, " ");
    auto file = parts[1];
    auto addr = parts[2];
    auto name = parts[3];
#endif

    if (file.starts_with(curdir)) {
      file.remove_prefix(curdir.size() + 1);
    }

    unique_malloc_ptr<char> demangled;
    if (name.starts_with("_Z")) {
      // abi::__cxa_demangle expects a null terminated string otherwise it
      // won't demangle the symbol name, so create a temporary std::string.
      demangled.reset(
          abi::__cxa_demangle(std::string{name.data(), name.size()}.c_str(),
                              nullptr, nullptr, nullptr));
      name = cargo::string_view{demangled.get()};
    }

    std::fprintf(out, "Frame %d [%.*s %.*s] %.*s\n", frame_index - 1,
                 static_cast<int>(file.size()), file.data(),
                 static_cast<int>(addr.size()), addr.data(),
                 static_cast<int>(name.size()), name.data());
  }
#else
  std::fprintf(out,
               "debug::print_backtrace() not supported on this platform\n");
  std::abort();
#endif
}
}  // namespace debug
