// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MULTI_LLVM_OPTIONAL_HELPER_H_INCLUDED
#define MULTI_LLVM_OPTIONAL_HELPER_H_INCLUDED

#include <llvm/ADT/Optional.h>

namespace multi_llvm {

template <typename T, typename U>
inline constexpr T value_or(llvm::Optional<T> O, U &&alt) {
#if (LLVM_VERSION_MAJOR >= 15)
  return O.has_value() ? O.value() : std::forward<U>(alt);
#else
  return O.hasValue() ? O.getValue() : std::forward<U>(alt);
#endif
}

}  // namespace multi_llvm

#endif  // MULTI_LLVM_OPTIONAL_HELPER_H_INCLUDED
