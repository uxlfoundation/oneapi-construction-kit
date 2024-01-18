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

/// @file
///
/// @brief Endianness conversion helpers.

#ifndef CARGO_ENDIAN_H_INCLUDED
#define CARGO_ENDIAN_H_INCLUDED

#include <cargo/type_traits.h>

#include <cstdint>
#include <iterator>

namespace cargo {

/// @brief Functions reversing the byte order in an integer.
uint8_t byte_swap(uint8_t v);
/// @brief Functions reversing the byte order in an integer.
uint16_t byte_swap(uint16_t v);
/// @brief Functions reversing the byte order in an integer.
uint32_t byte_swap(uint32_t v);
/// @brief Functions reversing the byte order in an integer.
uint64_t byte_swap(uint64_t v);

namespace detail {
struct endianness_helper {
 private:
  static constexpr uint32_t dword = 0x01020304;
#ifdef _MSC_VER  // ignore truncation warning to make this constexpr
#pragma warning(disable : 4309)
#endif
  static constexpr uint8_t first = static_cast<const uint8_t &>(dword);
#ifdef _MSC_VER
#pragma warning(default : 4309)
#endif

 public:
  static constexpr bool little = first == 0x04;
  static constexpr bool big = first == 0x01;
  static_assert(little || big, "Unsupported endianness");
};
}  // namespace detail

/// @brief Detect if the system is little endian.
///
/// @return Returns true if the system is little endian, false otherwise.
constexpr bool is_little_endian() { return detail::endianness_helper::little; }

/// @brief Reads an integer into the native endian format from a little-endian
/// byte iterator.
template <typename InputIterator>
typename std::enable_if_t<
    std::is_same_v<std::remove_const_t<
                       typename std::iterator_traits<InputIterator>::reference>,
                   uint8_t &>,
    InputIterator>
read_little_endian(uint8_t *v, InputIterator it) {
  *v = *it++;
  return it;
}

/// @brief Reads an integer into the native endian format from a little-endian
/// byte iterator.
template <typename InputIterator>
typename std::enable_if_t<
    std::is_same_v<std::remove_const_t<
                       typename std::iterator_traits<InputIterator>::reference>,
                   uint8_t &>,
    InputIterator>
read_little_endian(uint16_t *v, InputIterator it) {
  *v = *it++;
  *v |= *it++ << 8;
  return it;
}

/// @brief Reads an integer into the native endian format from a little-endian
/// byte iterator.
template <typename InputIterator>
typename std::enable_if_t<
    std::is_same_v<std::remove_const_t<
                       typename std::iterator_traits<InputIterator>::reference>,
                   uint8_t &>,
    InputIterator>
read_little_endian(uint32_t *v, InputIterator it) {
  *v = *it++;
  *v |= *it++ << 8;
  *v |= *it++ << 16;
  *v |= *it++ << 24;
  return it;
}

/// @brief Reads an integer into the native endian format from a little-endian
/// byte iterator.
template <typename InputIterator>
typename std::enable_if_t<
    std::is_same_v<std::remove_const_t<
                       typename std::iterator_traits<InputIterator>::reference>,
                   uint8_t &>,
    InputIterator>
read_little_endian(uint64_t *v, InputIterator it) {
  *v = *it++;
  *v |= static_cast<uint64_t>(*it++) << 8;
  *v |= static_cast<uint64_t>(*it++) << 16;
  *v |= static_cast<uint64_t>(*it++) << 24;
  *v |= static_cast<uint64_t>(*it++) << 32;
  *v |= static_cast<uint64_t>(*it++) << 40;
  *v |= static_cast<uint64_t>(*it++) << 48;
  *v |= static_cast<uint64_t>(*it++) << 56;
  return it;
}

/// @brief Reads an integer into the native endian format from a big-endian
/// byte iterator.
template <typename Integer, typename InputIterator>
typename std::enable_if_t<
    std::is_same_v<std::remove_const_t<
                       typename std::iterator_traits<InputIterator>::reference>,
                   uint8_t &>,
    InputIterator>
read_big_endian(Integer *v, InputIterator it) {
  // read from little-endian to native and swap to achieve the same effect as a
  // big-endian read with less code duplication
  std::remove_cv_t<Integer> le;
  it = read_little_endian(&le, it);
  *v = byte_swap(le);
  return it;
}

/// @brief Writes a native-endian integer to a little-endian stream byte
/// iterator.
template <typename OutputIterator>
typename std::enable_if_t<
    std::is_same_v<typename std::iterator_traits<OutputIterator>::reference,
                   uint8_t &>,
    OutputIterator>
write_little_endian(uint8_t v, OutputIterator it) {
  *it++ = v;
  return it;
}

/// @brief Writes a native-endian integer to a little-endian stream byte
/// iterator.
template <typename OutputIterator>
typename std::enable_if_t<
    std::is_same_v<typename std::iterator_traits<OutputIterator>::reference,
                   uint8_t &>,
    OutputIterator>
write_little_endian(uint16_t v, OutputIterator it) {
  *it++ = v & 0xFF;
  *it++ = (v >> 8) & 0xFF;
  return it;
}

/// @brief Writes a native-endian integer to a little-endian stream byte
/// iterator.
template <typename OutputIterator>
typename std::enable_if_t<
    std::is_same_v<typename std::iterator_traits<OutputIterator>::reference,
                   uint8_t &>,
    OutputIterator>
write_little_endian(uint32_t v, OutputIterator it) {
  *it++ = v & 0xFF;
  *it++ = (v >> 8) & 0xFF;
  *it++ = (v >> 16) & 0xFF;
  *it++ = (v >> 24) & 0xFF;
  return it;
}

/// @brief Writes a native-endian integer to a little-endian stream byte
/// iterator.
template <typename OutputIterator>
typename std::enable_if_t<
    std::is_same_v<typename std::iterator_traits<OutputIterator>::reference,
                   uint8_t &>,
    OutputIterator>
write_little_endian(uint64_t v, OutputIterator it) {
  *it++ = v & 0xFF;
  *it++ = (v >> 8) & 0xFF;
  *it++ = (v >> 16) & 0xFF;
  *it++ = (v >> 24) & 0xFF;
  *it++ = (v >> 32) & 0xFF;
  *it++ = (v >> 40) & 0xFF;
  *it++ = (v >> 48) & 0xFF;
  *it++ = (v >> 56) & 0xFF;
  return it;
}

/// @brief Writes a native-endian integer to a big-endian stream byte iterator.
template <typename Integer, typename OutputIterator>
typename std::enable_if_t<
    std::is_same_v<typename std::iterator_traits<OutputIterator>::reference,
                   uint8_t &>,
    OutputIterator>
write_big_endian(Integer v, OutputIterator it) {
  // write swapped native to little-endian to achieve the same effect as a
  // big-endian write with less code duplication
  return write_little_endian(byte_swap(v), it);
}

}  // namespace cargo

#endif
