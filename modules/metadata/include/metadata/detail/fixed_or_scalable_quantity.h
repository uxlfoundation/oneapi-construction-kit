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
#ifndef MD_FIXED_OR_SCALABLE_QUANTITY_H_INCLUDED
#define MD_FIXED_OR_SCALABLE_QUANTITY_H_INCLUDED

#include <cassert>

/// @brief A fixed quantity 'k', optionally made "scalable" denoting a
/// multiplication by an unknown non-zero runtime value.
template <typename ValueTy>
class FixedOrScalableQuantity {
 public:
  FixedOrScalableQuantity() = default;

  FixedOrScalableQuantity(ValueTy Quantity, bool Scalable)
      : Quantity(Quantity), Scalable(Scalable) {}

  /// @brief Returns the non-scalable value of one.
  static constexpr FixedOrScalableQuantity<ValueTy> getOne() {
    return FixedOrScalableQuantity<ValueTy>(1, false);
  }

  /// @brief Returns whether the value is known to be zero.
  constexpr bool isZero() const { return getKnownMinValue() == 0; }

  /// @brief Returns whether the value is known to be non-zero.
  constexpr bool isNonZero() const { return getKnownMinValue() != 0; }

  /// @brief Returns whether the value is known to be non-zero.
  constexpr explicit operator bool() const { return isNonZero(); }

  /// @brief Returns the minimum value this quantity can represent.
  constexpr ValueTy getKnownMinValue() const { return Quantity; }

  /// @brief Returns whether the quantity is scaled by a runtime quantity.
  constexpr bool isScalable() const { return Scalable; }

  /// @brief Return the minimum value with the assumption that the count is
  /// exact.
  ///
  /// Use in places where a scalable count doesn't make sense (e.g. non-vector
  /// types, or vectors in backends which don't support scalable vectors).
  constexpr ValueTy getFixedValue() const {
    assert(!isScalable() &&
           "Request for a fixed element count on a scalable object");
    return getKnownMinValue();
  }

 private:
  ValueTy Quantity = 0;
  bool Scalable = false;
};

#endif  // MD_FIXED_OR_SCALABLE_QUANTITY_H_INCLUDED
