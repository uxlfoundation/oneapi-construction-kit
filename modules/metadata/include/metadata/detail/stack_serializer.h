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

/// @file stack_serializer.h
///
/// @brief Metadata API Stack Serializer.

#ifndef MD_DETAIL_STACK_SERIALIZER_H_INCLUDED
#define MD_DETAIL_STACK_SERIALIZER_H_INCLUDED
#include <cargo/array_view.h>
#include <cargo/endian.h>
#include <metadata/detail/metadata_impl.h>
#include <metadata/detail/utils.h>

#include <cassert>
#include <cstdint>
#include <vector>

namespace {
/// @brief Write a number type as raw bytes to the output with the correct
/// endianness.
///
/// @tparam NumberTy The type of the number.
/// @param num The number.
/// @param output The output binary.
/// @param endianness The desired endianness.
template <class NumberTy,
          std::enable_if_t<std::is_integral_v<NumberTy> ||
                               std::is_floating_point_v<NumberTy>,
                           bool> = true>
void serialize_number(NumberTy num, std::vector<uint8_t> &output,
                      MD_ENDIAN endianness) {
  const auto width = sizeof(NumberTy);
  auto *data_ptr = reinterpret_cast<uint8_t *>(&num);
  cargo::array_view<uint8_t> data(data_ptr, width);
  constexpr MD_ENDIAN host_endianness =
      cargo::is_little_endian() ? MD_ENDIAN::LITTLE : MD_ENDIAN::BIG;
  // When the host endianness differs from the requested target endianness, we
  // insert the bytes into the output binary in reverse order which
  // effectively performs the required byteswap.
  if (host_endianness != endianness) {
    output.insert(output.end(), data.rbegin(), data.rend());
  } else {
    output.insert(output.end(), data.begin(), data.end());
  }
}

}  // namespace

namespace md {
/// @addtogroup md
/// @{

/// @brief Implementation of a raw stack-serializer. This is intended to be used
/// when the output format md_fmt::MD_FMT_RAW_BYTES is selected.
///
/// Serialization is completed by converting each item on the stack into raw
/// bytes and formatted correctly according to the desired endian format.
/// De-serialization simply pushes a single raw byte-array to the stack which
/// wraps the block.
///
/// @tparam StackType Type of the stack to be used.
template <class StackType>
class RawStackSerializer {
  using stack_t = typename StackType::stack_t;
  using element_t = typename StackType::element_t;
  using output_t = std::vector<uint8_t>;

  /// @brief Serialize an individual element, and write the result to the
  /// output binary.
  ///
  /// @param elem The element to be serialized.
  /// @param output The output binary.
  /// @param endianness The desired endianness.
  static void serialize(const element_t &elem, output_t &output,
                        MD_ENDIAN endianness) {
    switch (elem.get_type()) {
      case md_value_type::MD_TYPE_SINT: {
        auto *val = elem.template get<typename StackType::signed_t>();
        serialize_number(*val, output, endianness);
      } break;
      case md_value_type::MD_TYPE_UINT: {
        auto *val = elem.template get<typename StackType::unsigned_t>();
        serialize_number(*val, output, endianness);
        break;
      }
      case md_value_type::MD_TYPE_REAL: {
        auto *val = elem.template get<typename StackType::real_t>();
        serialize_number(*val, output, endianness);
        break;
      }
      case md_value_type::MD_TYPE_ZSTR: {
        auto *val = elem.template get<typename StackType::string_t>();
        const char *str = val->c_str();
        output.insert(output.end(), str, str + val->size() + 1);
        break;
      }
      case md_value_type::MD_TYPE_BYTESTR: {
        auto *val = elem.template get<typename StackType::byte_arr_t>();
        output.insert(output.end(), val->begin(), val->end());
        break;
      }
      case md_value_type::MD_TYPE_ARRAY: {
        auto *arr = elem.template get<typename StackType::array_t>();
        for (auto &item : *arr) {
          serialize(item, output, endianness);
        }
        break;
      }
      case md_value_type::MD_TYPE_HASH: {
        auto *hash = elem.template get<typename StackType::map_t>();
        for (auto &pair : *hash) {
          serialize(pair.first, output, endianness);
          serialize(pair.second, output, endianness);
        }
        break;
      }

      default:
        break;
    }
  }

 public:
  /// @brief Serialize an metadata stack into a binary block.
  ///
  /// @param stack The stack to be serialized.
  /// @param output An output vector where the binary block will be written to.
  /// @param endianness The desired output endianness.
  static void serialize(StackType &stack, std::vector<uint8_t> &output,
                        MD_ENDIAN endianness) {
    for (const element_t &elem : stack) {
      serialize(elem, output, endianness);
    }
  }

  /// @brief Deserialize a block.
  ///
  /// @param stack A reference to a stack where the deserialized data will be
  /// added.
  /// @param block_start A pointer to the start of the block.
  /// @param block_len The length of the block.
  static void deserialize(StackType &stack, uint8_t *block_start,
                          size_t block_len, MD_ENDIAN) {
    stack.push_bytes(block_start, block_len);
  }
};

/// @brief These values are specified my the Message Pack spec and describe the
/// data should be interpreted.
enum MsgPackFmt : uint8_t {
  MSG_PACK_UINT_64 = 0xcf,
  MSG_PACK_INT_64 = 0xd3,
  MSG_PACK_DOUBLE = 0xcb,
  MSG_PACK_STR_16 = 0xda,
  MSG_PACK_BIN_16 = 0xc5,
  MSG_PACK_BIN_32 = 0xc6,
  MSG_PACK_ARR_16 = 0xdc,
  MSG_PACK_MAP_16 = 0xde,
};

/// @brief Basic Message Pack serializer for the metadata API.
///
/// We only support a minimal subset of message pack types for the necessary
/// types used by the metadata API. The only valid types are specified in
/// `MsgPackFmt`. Arrays, Strings and Maps are all limited to 2^(16)-1 or 65535
/// elements, which should be sufficient for our purposes.
/// @tparam StackType Type of the stack to be used.
template <class StackType>
class BasicMsgPackStackSerializer {
 public:
  using stack_t = typename StackType::stack_t;
  using element_t = typename StackType::element_t;
  using output_t = std::vector<uint8_t>;

 private:
  /// @brief Deserialize a stack element.
  ///
  /// @param stack A reference to the stack to which the deserialized data will
  /// be added.
  /// @param data A pointer to the element.
  /// @param fmt The message pack format of the element.
  /// @return Pointer in the binary to the next element.
  static uint8_t *deserialize_element(StackType &stack, uint8_t *data,
                                      MsgPackFmt fmt) {
    switch (fmt) {
      case MsgPackFmt::MSG_PACK_UINT_64: {
        auto val = md::utils::read_value<uint64_t>(data, MD_ENDIAN::BIG);
        stack.push_unsigned(val);
        return data += sizeof(uint64_t);
      }
      case MsgPackFmt::MSG_PACK_INT_64: {
        auto val = md::utils::read_value<uint64_t>(data, MD_ENDIAN::BIG);
        stack.push_signed(cargo::bit_cast<int64_t>(val));
        return data += sizeof(uint64_t);
      }
      case MsgPackFmt::MSG_PACK_DOUBLE: {
        auto val = md::utils::read_value<uint64_t>(data, MD_ENDIAN::BIG);
        stack.push_real(cargo::bit_cast<typename StackType::real_t>(val));
        return data += sizeof(uint64_t);
      }
      case MsgPackFmt::MSG_PACK_BIN_16: {
        const uint16_t bin_len =
            md::utils::read_value<uint16_t>(data, MD_ENDIAN::BIG);
        data += sizeof(uint16_t);
        stack.push_bytes(data, bin_len);
        return data += bin_len * sizeof(uint8_t);
      }
      case MsgPackFmt::MSG_PACK_BIN_32: {
        const uint32_t bin_len =
            md::utils::read_value<uint32_t>(data, MD_ENDIAN::BIG);
        data += sizeof(uint32_t);
        stack.push_bytes(data, bin_len);
        return data += bin_len * sizeof(uint8_t);
      }
      case MsgPackFmt::MSG_PACK_STR_16: {
        const uint16_t str_len =
            md::utils::read_value<uint16_t>(data, MD_ENDIAN::BIG);
        data += sizeof(uint16_t);
        const typename StackType::string_t str(
            (char *)data, str_len,
            stack.get_alloc_helper().template get_allocator<char>());
        stack.push_zstr(str.c_str());
        return data += str_len * sizeof(uint8_t);
      }
      case MsgPackFmt::MSG_PACK_ARR_16: {
        const uint16_t arr_len =
            md::utils::read_value<uint16_t>(data, MD_ENDIAN::BIG);
        data += sizeof(uint16_t);
        const size_t arr_idx = stack.push_arr(arr_len).value();
        for (size_t i = 0; i < arr_len; ++i) {
          auto type = static_cast<MsgPackFmt>(*data);
          data += sizeof(MsgPackFmt);
          data = deserialize_element(stack, data, type);
          stack.arr_append(arr_idx, stack.top().value());
          stack.pop();
        }
        return data;
      }
      case MsgPackFmt::MSG_PACK_MAP_16: {
        const uint16_t map_len =
            md::utils::read_value<uint16_t>(data, MD_ENDIAN::BIG);
        data += sizeof(uint16_t);
        const size_t map_idx = stack.push_map(map_len).value();
        for (size_t i = 0; i < map_len; ++i) {
          auto key_type = static_cast<MsgPackFmt>(*data);
          data += sizeof(MsgPackFmt);
          data = deserialize_element(stack, data, key_type);

          auto val_type = static_cast<MsgPackFmt>(*data);
          data += sizeof(MsgPackFmt);
          data = deserialize_element(stack, data, val_type);

          stack.hash_set_kv(map_idx, stack.top().value() - 1,
                            stack.top().value());
          stack.pop();
          stack.pop();
        }
        return data;
      }
      default:
        assert(0 && "Invalid or unsupported MsgPack type qualifier");
        return nullptr;
    }
  }

  /// @brief Serialize an individual stack element.
  ///
  /// @param elem The element to be serialized.
  /// @param output The output binary where the bytes will be written.
  static void serialize_element(const element_t &elem, output_t &output) {
    switch (elem.get_type()) {
      case md_value_type::MD_TYPE_UINT: {
        output.emplace_back(MsgPackFmt::MSG_PACK_UINT_64);
        auto *val = elem.template get<typename StackType::unsigned_t>();
        serialize_number(*val, output, MD_ENDIAN::BIG);
        break;
      }
      case md_value_type::MD_TYPE_SINT: {
        output.emplace_back(MsgPackFmt::MSG_PACK_INT_64);
        auto *val = elem.template get<typename StackType::signed_t>();
        serialize_number(*val, output, MD_ENDIAN::BIG);
        break;
      }
      case md_value_type::MD_TYPE_REAL: {
        output.emplace_back(MsgPackFmt::MSG_PACK_DOUBLE);
        auto *val = elem.template get<typename StackType::real_t>();
        serialize_number(*val, output, MD_ENDIAN::BIG);
        break;
      }
      case md_value_type::MD_TYPE_ZSTR: {
        output.emplace_back(MsgPackFmt::MSG_PACK_STR_16);
        auto *val = elem.template get<typename StackType::string_t>();
        const uint16_t str_len = val->size();
        serialize_number(str_len, output, MD_ENDIAN::BIG);
        output.insert(output.end(), val->begin(), val->end());
        break;
      }
      case md_value_type::MD_TYPE_BYTESTR: {
        auto *val = elem.template get<typename StackType::byte_arr_t>();
        if (val->size() >= std::numeric_limits<uint16_t>::max()) {
          output.emplace_back(MsgPackFmt::MSG_PACK_BIN_32);
          const uint32_t byte_str_len = val->size();
          serialize_number(byte_str_len, output, MD_ENDIAN::BIG);
        } else {
          output.emplace_back(MsgPackFmt::MSG_PACK_BIN_16);
          const uint16_t byte_str_len = val->size();
          serialize_number(byte_str_len, output, MD_ENDIAN::BIG);
        }
        output.insert(output.end(), val->begin(), val->end());
        break;
      }
      case md_value_type::MD_TYPE_ARRAY: {
        output.emplace_back(MsgPackFmt::MSG_PACK_ARR_16);
        auto *val = elem.template get<typename StackType::array_t>();
        const uint16_t arr_len = val->size();
        serialize_number(arr_len, output, MD_ENDIAN::BIG);
        for (const auto &item : *val) {
          serialize_element(item, output);
        }
        break;
      }
      case md_value_type::MD_TYPE_HASH: {
        output.emplace_back(MsgPackFmt::MSG_PACK_MAP_16);
        auto *val = elem.template get<typename StackType::map_t>();
        const uint16_t map_len = val->size();
        serialize_number(map_len, output, MD_ENDIAN::BIG);
        for (const auto &kv : *val) {
          serialize_element(kv.first, output);
          serialize_element(kv.second, output);
        }
        break;
      }
      default:
        assert(0 && "Invalid Value Type!");
        break;
    }
  }

 public:
  /// @brief Deserialize a block
  ///
  /// @param stack A reference to the stack to which the deserialized data will
  /// be added.
  /// @param block_start A pointer to the start of the block.
  /// @param block_len The length of the block.
  static void deserialize(StackType &stack, uint8_t *block_start,
                          size_t block_len, const MD_ENDIAN) {
    const uint8_t *origin = block_start;
    while (((origin + block_len) - block_start) > 0) {
      auto type = static_cast<MsgPackFmt>(*block_start);
      block_start += sizeof(MsgPackFmt);
      block_start = deserialize_element(stack, block_start, type);
    }
  }

  /// @brief Serialize an metadata stack into a binary block.
  ///
  /// @param stack The stack to be serialized.
  /// @param output An output vector where the binary block will be written to.
  /// @param endianness The desired output endianness.
  static void serialize(StackType stack, std::vector<uint8_t> &output,
                        MD_ENDIAN) {
    for (const auto &elem : stack) {
      serialize_element(elem, output);
    }
  }
};
/// @}
}  // namespace md

#endif  // MD_DETAIL_STACK_SERIALIZER_H_INCLUDED
