// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "ucl/types.h"

#include "kts/precision.h"

namespace ucl {
template <>
inline std::ostream &operator<<(std::ostream &out,
                                const ScalarType<Half, HalfTag> &scalar) {
  out << kts::ucl::ConvertHalfToFloat(scalar.value()) << "fp16";
  return out;
}
}  // namespace ucl
