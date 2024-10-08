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

#ifndef UNITCL_KTS_GENERATOR_H_INCLUDED
#define UNITCL_KTS_GENERATOR_H_INCLUDED

#include <CL/cl.h>

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <set>
#include <vector>

#include "cargo/type_traits.h"
#include "cargo/utility.h"
#include "kts/type_info.h"

namespace kts {
namespace ucl {
/// @brief Class for encapsulating generation of test inputs.
class InputGenerator final {
 public:
  InputGenerator() = delete;

  /// @brief Constructor
  ///
  /// @param seed The seed used for random number generation. If 0 picks a new
  ///        value for the seed at random.
  InputGenerator(unsigned seed) : seed_(seed) {
    if (0 == seed_) {
      seed_ = std::random_device{}();
    }
    gen_.seed(seed_);
    DumpSeed();
  }

  /// @brief Prints the seed to stdout, so we users replicate failing inputs
  void DumpSeed() const;

  /// @brief Returns the random seed initalized when generator was constructed
  unsigned GetSeed() const { return seed_; }

  /// @brief Populates buffer with half precision floats.
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  void GenerateFloatData(std::vector<cl_half> &buffer);

  /// @brief Populates buffer with random floats of type T, including inf
  /// and NaN.
  ///
  /// @tparam T cl_float or cl_double
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  template <typename T,
            std::enable_if_t<std::is_floating_point_v<T>> * = nullptr>
  void GenerateFloatData(std::vector<T> &buffer);

  /// @brief Populates buffer with random float of type T while avoiding inf
  /// and NaN (hence "finite"), and allowing for range restrictions.
  ///
  /// Used to test functions with relaxed or implementation defined requirements
  /// such as the native_ math builtin variants.
  ///
  /// @tparam T cl_float or cl_double
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  ///
  /// @param low Lower bound on the range of values to be generated.
  /// @param high Upper bound on the range of values to be generated.
  ///
  /// @note Low/high nomenclature used here instead of min/max as min might
  /// imply smallest in magnitude (i.e. closest to zero), which is not
  /// necessarily the case.
  template <typename T,
            std::enable_if_t<std::is_floating_point_v<T>> * = nullptr>
  void GenerateFiniteFloatData(std::vector<T> &buffer,
                               T low = std::numeric_limits<T>::lowest(),
                               T high = std::numeric_limits<T>::max());

  /// @brief Populates buffer with random integers of type T.
  ///
  /// @tparam T Integer scalar type to fill returned buffer with
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  ///
  /// @param min Minimum value in the output range.
  /// @param max Maximum value in the output range.
  template <class T>
  void GenerateIntData(std::vector<T> &buffer,
                       T min = std::numeric_limits<T>::min(),
                       T max = std::numeric_limits<T>::max());

  /// @brief Populates buffer unique with random integers of type T.
  ///
  /// @tparam T Integer scalar type to fill returned buffer with
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  /// @param min Minimum value in the output range.
  /// @param max Maximum value in the output range.
  template <class T>
  void GenerateUniqueIntData(std::vector<T> &buffer,
                             T min = std::numeric_limits<T>::min(),
                             T max = std::numeric_limits<T>::max());

  /// @brief Generates a random integer of type T.
  ///
  /// @tparam T Integer scalar type to be returned.
  ///
  /// @param min Minimum value in the output range.
  /// @param max Maximum value in the output range.
  template <class T>
  T GenerateInt(T min = std::numeric_limits<T>::min(),
                T max = std::numeric_limits<T>::max());

  /// @brief Populates buffer with random values of type T.
  ///
  /// @tparam T Integer or floating point scalar type to fill returned buffer
  ///           with.
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  template <typename T>
  std::enable_if_t<std::is_integral_v<T>> GenerateData(std::vector<T> &buffer) {
    GenerateIntData(buffer);
  }

  /// @brief Populates buffer with random values of type T.
  ///
  /// @tparam T Integer or floating point scalar type to fill returned buffer
  ///           with.
  ///
  /// @param buffer Vector that will be populated. Size should already be
  ///        at the capacity of number of elements to fill.
  template <typename T>
  std::enable_if_t<std::is_integral_v<T>> GenerateData(std::vector<T> &buffer,
                                                       T min, T max) {
    GenerateIntData(buffer, min, max);
  }

  template <typename T>
  std::enable_if_t<std::is_floating_point_v<T>> GenerateData(
      std::vector<T> &buffer) {
    GenerateFloatData(buffer);
  }

  /// @brief Special cases that have been known to induce failures, particularly
  /// in fma() where ideally they'd be tested across every input combination.
  static const std::array<cl_ushort, 26> half_edge_cases;

 private:
  /// @brief Mersenne twister engine for generating random 64-bit ints
  std::mt19937_64 gen_;
  /// @brief seed for random number generation
  unsigned seed_;
};

template <class T, std::enable_if_t<std::is_floating_point_v<T>> *>
void InputGenerator::GenerateFiniteFloatData(std::vector<T> &buffer, T low,
                                             T high) {
  // Generate a distribution where each floating point value in the given range
  // has an equal probability.
  using UInt = typename TypeInfo<T>::AsUnsigned;

  // Since the binary representation of positive floating point values has the
  // same ordering as the floating point values they represent, a range over
  // floating point values has an equivalent range over integers. In order to
  // handle negative values, which are ordered "backwards", we invert all bits
  // other than the sign bit, if the sign bit is set. This gives us an
  // equivalent range over signed integers. Inverting the sign bit then converts
  // that to a range over unsigned integers.
  const auto sign_bit = TypeInfo<T>::sign_bit;

  UInt iMin = cargo::bit_cast<UInt>(low);
  iMin ^= (iMin & sign_bit) ? ~UInt(0) : sign_bit;

  UInt iMax = cargo::bit_cast<UInt>(high);
  iMax ^= (iMax & sign_bit) ? ~UInt(0) : sign_bit;

  std::uniform_int_distribution<UInt> dist(iMin, iMax);
  std::generate(buffer.begin(), buffer.end(), [&] {
    UInt randbits = dist(gen_);

    // Convert back from uint ordering to floating point ordering.
    randbits ^= (randbits & sign_bit) ? sign_bit : ~UInt(0);
    return cargo::bit_cast<T>(randbits);
  });
}

template <class T, std::enable_if_t<std::is_floating_point_v<T>> *>
void InputGenerator::GenerateFloatData(std::vector<T> &buffer) {
  GenerateFiniteFloatData(buffer);

  using UInt = typename TypeInfo<T>::AsUnsigned;
  // Edge cases that should always be checked
  std::array<UInt, 4> edge_cases = {
      {0, TypeInfo<T>::sign_bit, TypeInfo<T>::exponent_mask,
       TypeInfo<T>::sign_bit | TypeInfo<T>::exponent_mask}};

  const unsigned edge_N = std::min(buffer.size(), edge_cases.size());
  for (unsigned i = 0; i < edge_N; i++) {
    buffer[i] = (cargo::bit_cast<T>(edge_cases[i]));
  }

  // Fill with as representations of NaN as will fit in the
  // buffer
  const unsigned nan_variants = TypeInfo<T>::mantissa_bits * 2;
  const unsigned nans_to_fill =
      std::min(static_cast<unsigned>(buffer.size() - edge_N), nan_variants);

  UInt mantissa = 0;
  for (unsigned i = edge_N; i < nans_to_fill; i++) {
    if (i % 2) {
      buffer[i] = cargo::bit_cast<T>(TypeInfo<T>::exponent_mask | mantissa);
    } else {
      buffer[i] = cargo::bit_cast<T>(TypeInfo<T>::exponent_mask |
                                     TypeInfo<T>::sign_bit | mantissa);
      mantissa++;
    }
  }

  // Shuffle vector so edge cases aren't always at the start
  std::shuffle(buffer.begin(), buffer.end(), gen_);
}

template <class T>
void InputGenerator::GenerateIntData(std::vector<T> &buffer, T min, T max) {
  assert(min <= max && "minimum value is greater than maximum value in range!");
  // This is a work around for the fact that std::uniform_int_distribution isn't
  // defined for 8 bit types. If we get an 8 bit type we use a wider integer
  // then cast back the result.
  using LargerType = std::conditional_t<
      std::is_same_v<std::make_unsigned_t<T>, uint8_t>,
      std::conditional_t<std::is_unsigned_v<T>, uint32_t, int32_t>, T>;

  std::uniform_int_distribution<LargerType> dist(min, max);
  std::generate(buffer.begin(), buffer.end(),
                [&] { return static_cast<T>(dist(gen_)); });

  // Try to always test edge cases
  const std::array<T, 3> edge_cases = {{0, min, max}};
  const size_t N = std::min(edge_cases.size(), buffer.size());
  for (unsigned i = 0; i < N; i++) {
    buffer[i] = edge_cases[i];
  }

  // Shuffle vector so edge cases aren't always at the start
  std::shuffle(buffer.begin(), buffer.end(), gen_);
}

template <class T>
void InputGenerator::GenerateUniqueIntData(std::vector<T> &buffer, T min,
                                           T max) {
  // The user could potentially shoot themseleves in the foot here by asking for
  // more values than can actually be represented by type T.
  assert(buffer.size() == (size_t)(std::make_unsigned_t<T>)buffer.size() &&
         "Caller requested more unique values than can be represented by the "
         "type");
  // Try to always test
  // edge cases We use a set here to avoid duplicate edge cases e.g. if min ==
  // 0.
  std::set<T> edge_cases = {{0, min, max}};
  const size_t N = std::min(edge_cases.size(), buffer.size());
  std::copy_n(std::begin(edge_cases), N, std::begin(buffer));

  // We can exit early if the edge cases were enough to fill the buffer.
  if (edge_cases.size() >= buffer.size()) {
    return;
  }

  std::uniform_int_distribution<T> dist(min, max);
  // We know at this point we already have all the edge cases in our buffer, so
  // we can start after the final edge case.
  unsigned i = edge_cases.size();
  std::generate(std::next(std::begin(buffer), i), std::end(buffer), [&] {
    // Get an initial value.
    T next_value = dist(gen_);

    // Then keep updating it until we get a random value we don't already have.
    // We only need to check up to the value we are replacing.
    const auto search_start = std::begin(buffer);
    const auto search_end = std::next(search_start, i);
    while (std::find(search_start, search_end, next_value) != search_end) {
      next_value = dist(gen_);
    }
    ++i;
    return next_value;
  });

  // Shuffle vector so edge cases aren't always at the start
  std::shuffle(std::begin(buffer), std::end(buffer), gen_);
}

template <class T>
T InputGenerator::GenerateInt(T min, T max) {
  // This is a work around for the fact that std::uniform_int_distribution isn't
  // defined for 8 bit types. If we get an 8 bit type we use a wider integer
  // then cast back the result.
  using LargerType = std::conditional_t<
      std::is_same_v<std::make_unsigned_t<T>, uint8_t>,
      std::conditional_t<std::is_unsigned_v<T>, uint32_t, int32_t>, T>;
  std::uniform_int_distribution<LargerType> dist(min, max);
  return static_cast<T>(dist(gen_));
}

}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_GENERATOR_H_INCLUDED
