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

// This file does not need to depend on LLVM

#ifndef COMPILER_UTILS_VECTORIZATION_FACTOR_H_INCLUDED
#define COMPILER_UTILS_VECTORIZATION_FACTOR_H_INCLUDED

namespace compiler {
namespace utils {
/// @addtogroup utils
/// @{

/// @brief A vectorization factor.
class VectorizationFactor {
 public:
  /// @brief Creates a scalar vectorization factor.
  VectorizationFactor() : KnownMin(1u), IsScalable(false) {}
  /// @brief Creates a fixed/scalable vectorization factor with the known
  /// minimum number of elements.
  /// @param[in] KnownMin The known minimum number of elements.
  /// @param[in] IsScalable Whether or not this factor is scalable: that is,
  /// the true factor is scaled by an unknown amount, determined at runtime.
  VectorizationFactor(unsigned KnownMin, bool IsScalable)
      : KnownMin(KnownMin), IsScalable(IsScalable) {}

  /// @brief Returns whether the vectorization factor is a scalar amount
  /// (exactly one value).
  bool isScalar() const { return !IsScalable && KnownMin == 1; }

  /// @brief Returns whether the vectorization factor is a vector amount (more
  /// than one fixed-length value, or possible one scalable value).
  bool isVector() const {
    return (IsScalable && KnownMin != 0) || KnownMin > 1;
  }

  /// @brief Creates a scalar vectorization factor.
  static VectorizationFactor getScalar() {
    return VectorizationFactor{1u, false};
  }

  /// @brief Creates a fixed-width vectorization factor.
  /// @param[in] Width The known number of elements.
  static VectorizationFactor getFixedWidth(unsigned Width) {
    return VectorizationFactor{Width, false};
  }

  /// @brief Creates a scalable vectorization factor.
  /// @param[in] KnownMin The known multiple of elements. The true number
  /// of elements will be a runtime-determined multiple of this.
  static VectorizationFactor getScalable(unsigned KnownMin) {
    return VectorizationFactor{KnownMin, true};
  }

  /// @brief Sets the IsScalable property of this vectorization factor.
  void setIsScalable(const bool V) { IsScalable = V; }
  /// @brief Returns whether this vectorization factor is scalable.
  bool isScalable() const { return IsScalable; }

  /// @brief Sets the known minimum number of elements.
  void setKnownMin(unsigned W) { KnownMin = W; }
  /// @brief Returns the known minimum number of elements this vectorization
  /// factor represents.
  unsigned getKnownMin() const { return KnownMin; }

  VectorizationFactor operator*(unsigned other) const {
    auto res = *this;
    res.KnownMin *= other;
    return res;
  }

  bool operator==(const VectorizationFactor &other) const {
    return KnownMin == other.KnownMin && IsScalable == other.IsScalable;
  }

  bool operator!=(const VectorizationFactor &other) const {
    return !operator==(other);
  }

  bool operator==(unsigned other) const {
    return !IsScalable && KnownMin == other;
  }

  bool operator!=(unsigned other) const { return !operator==(other); }

 private:
  unsigned KnownMin = 1;
  bool IsScalable = false;
};

/// @}

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_VECTORIZATION_FACTOR_H_INCLUDED
