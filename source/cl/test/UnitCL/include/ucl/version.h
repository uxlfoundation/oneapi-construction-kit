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

#ifndef UNITCL_VERSION_H_INCLUDED
#define UNITCL_VERSION_H_INCLUDED

#include <ostream>

#if defined(__GLIBC__) || defined(__QNX__)
// CL/cl.h and gtest/gtest.h include a libc header that might define these
// macros that clash with the Version struct, so undefine them. Both glibc
// and QNX's libc do this.
#undef major
#undef minor
#endif

namespace ucl {
/// @brief Major/minor version comparison utility.
///
/// Comparisons operate on a single integer which is scaled combination of the
/// major and minor version numbers, e.g. the version "1.2" can be defined as
/// `UCL::Version(1, 2)` and is scaled to `120` for comparisons. This scheme
/// follows the numbering used in the OpenCL headers to define the
/// `CL_VERSION_<MAJOR>_<MINOR>` macros.
struct Version {
  Version() = default;
  Version(int major, int minor)
      : Major(major), Minor(minor), Scaled((major * 100) + (minor * 10)) {}

  bool operator==(const Version &rhs) const { return Scaled == rhs.Scaled; }
  bool operator!=(const Version &rhs) const { return Scaled != rhs.Scaled; }
  bool operator<(const Version &rhs) const { return Scaled < rhs.Scaled; }
  bool operator>(const Version &rhs) const { return Scaled > rhs.Scaled; }
  bool operator<=(const Version &rhs) const { return Scaled <= rhs.Scaled; }
  bool operator>=(const Version &rhs) const { return Scaled >= rhs.Scaled; }

  int major() const { return Major; }
  int minor() const { return Minor; }

  friend std::ostream &operator<<(std::ostream &out, const Version &version) {
    out << version.Major << "." << version.Minor;
    return out;
  }

 private:
  int Major = 0;
  int Minor = 0;
  int Scaled = 0;
};
}  // namespace ucl

#endif  // UNITCL_VERSION_H_INCLUDED
