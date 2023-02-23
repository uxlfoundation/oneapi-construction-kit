// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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

  ~File() { std::fclose(file); }

  template <class Container>
  Container read() {
    Container content;
    using value_type = typename Container::value_type;
    std::array<value_type, 256> buffer;
    while (size_t objects = std::fread(buffer.data(), sizeof(value_type),
                                       buffer.size(), file)) {
      content.insert(content.end(), buffer.data(), buffer.data() + objects);
    }
    return content;
  }

  std::string name;
  FILE *file = nullptr;
};
}  // namespace ucl

#endif  // UNITCL_FILE_H_INCLUDED
