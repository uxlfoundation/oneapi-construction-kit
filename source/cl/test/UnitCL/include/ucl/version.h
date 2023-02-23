// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
      : Major(major), Minor(minor), Scaled(major * 100 + minor * 10) {}

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
