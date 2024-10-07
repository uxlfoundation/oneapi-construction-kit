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

#ifndef UNITCL_FILE_H_INCLUDED
#define UNITCL_FILE_H_INCLUDED

#include <array>
#include <cstdio>
#include <string>

#include "ucl/assert.h"

namespace ucl {
struct File {
  File(const std::string &name)
      : name{name}, file{std::fopen(name.c_str(), "rb")} {
    if (!file) {
      UCL_ABORT("failed to open file: %s", name.c_str());
    }
  }

  ~File() { (void)std::fclose(file); }

  template <class Container>
  Container read() {
    Container content;
    using value_type = typename Container::value_type;
    std::array<value_type, 256> buffer;
    constexpr size_t buffer_size = buffer.size();
    for (;;) {
      const size_t objects =
          std::fread(buffer.data(), sizeof(value_type), buffer_size, file);
      content.insert(content.end(), buffer.data(), buffer.data() + objects);
      if (objects < buffer_size) break;
    }
    return content;
  }

  std::string name;
  FILE *file = nullptr;
};
}  // namespace ucl

#endif  // UNITCL_FILE_H_INCLUDED
