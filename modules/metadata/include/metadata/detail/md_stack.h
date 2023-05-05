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

/// @file md_stack.h
///
/// @brief Metadata API Stack.

#ifndef MD_DETAIL_MD_STACK_H_INCLUDED
#define MD_DETAIL_MD_STACK_H_INCLUDED
#include <cargo/expected.h>
#include <metadata/detail/basic_map.h>
#include <metadata/detail/md_value.h>
#include <metadata/detail/stack_serializer.h>

namespace md {
/// @addtogroup md
/// @{

/// @brief An implementation of a stack.
///
/// @tparam AllocatorType Defaulted to callback_allocator
/// @tparam StackType Defaulted to std::vector
/// @tparam ArrayType Defaulted to std::vector
/// @tparam MapType Defaulted to basic_map
template <template <class U> class AllocatorType = md::callback_allocator,
          template <class U, typename... Args> class StackType = std::vector,
          template <class U, typename... Args> class ArrayType = std::vector,
          template <class U, class V, template <class> class> class MapType =
              basic_map>
class basic_stack {
 public:
  using allocator_helper_t = AllocatorHelper<AllocatorType>;
  using self_t = basic_stack<AllocatorType, StackType, ArrayType, MapType>;

  using element_t = basic_value<AllocatorType>;
  using stack_t = StackType<element_t, AllocatorType<element_t>>;
  using unsigned_t = uint64_t;
  using signed_t = int64_t;
  using real_t = double;
  using string_t =
      std::basic_string<char, std::char_traits<char>, AllocatorType<char>>;
  using map_t = MapType<element_t, element_t, AllocatorType>;
  using array_t = ArrayType<element_t, AllocatorType<element_t>>;
  using byte_arr_t = ArrayType<uint8_t, AllocatorType<uint8_t>>;

  using iterator = typename stack_t::iterator;
  using const_iterator = typename stack_t::const_iterator;
  using reference = typename stack_t::reference;
  using const_reference = typename stack_t::const_reference;

  /// @brief Construct a new basic stack object.
  ///
  /// @param alloc A reference to an allocator helper.
  /// @param reserve A hint on how much space to pre-reserve on the stack.
  basic_stack(allocator_helper_t alloc, size_t reserve = 0,
              md_fmt out_fmt = md_fmt::MD_FMT_RAW_BYTES)
      : m_alloc(std::move(alloc)),
        m_stack(m_alloc.template get_allocator<element_t>()),
        finalized(false),
        out_fmt(out_fmt) {
    if (reserve) {
      m_stack.reserve(reserve);
    }
  }

  /// @brief Default virtual destructor for inheritance.
  virtual ~basic_stack() noexcept = default;

  /// @brief Get the index of the top element on the stack.
  ///
  /// @return An index to the top element on the stack. If the stack is empty
  /// md_err::MD_E_EMPTY_STACK is returned.
  cargo::expected<size_t, md_err> top() const {
    if (m_stack.empty()) {
      return cargo::make_unexpected(md_err::MD_E_EMPTY_STACK);
    }
    return m_stack.size() - 1;
  }

  /// @brief Push an unsigned integer onto the stack.
  ///
  /// @param val The value to be pushed to the stack.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_unsigned(unsigned_t val) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_UINT, std::move(val));
    return top();
  }

  /// @brief Push a signed integer onto the stack.
  ///
  /// @param val The value to be pushed to the stack.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_signed(signed_t val) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_SINT, std::move(val));
    return top();
  }

  /// @brief Push a real-valued number onto the stack.
  ///
  /// @param val The value to be pushed to the stack.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_real(real_t val) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_REAL, std::move(val));
    return top();
  }

  /// @brief Push a zero-terminated string onto the stack.
  ///
  /// @param val The value to be pushed to the stack.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_zstr(const char *val) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    string_t str(val, m_alloc.template get_allocator<char>());
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_ZSTR, std::move(str));
    return top();
  }

  /// @brief Push a map onto the stack.
  ///
  /// @param size_hint A hint on how much space to pre-reserve in the map.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_map(size_t size_hint = 0) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    map_t map(m_alloc);
    if (size_hint) {
      map.reserve(size_hint);
    }
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_HASH, std::move(map));
    return top();
  }

  /// @brief Push an array onto the stack.
  ///
  /// @param size_hint A hint on how much space to pre-reserve in the array.
  /// @return The index in the stack of the newly pushed value.
  cargo::expected<size_t, md_err> push_arr(size_t size_hint = 0) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    array_t arr(m_alloc.template get_allocator<element_t>());
    if (size_hint) {
      arr.reserve(size_hint);
    }
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_ARRAY, std::move(arr));
    return top();
  }

  /// @brief Push a raw byte-array onto the stack.
  ///
  /// @param start A pointer to the start of the byte-array.
  /// @param len The length of the byte-array.
  /// @return The index in the stack of the newly pushed byte-array.
  cargo::expected<size_t, md_err> push_bytes(const uint8_t *start, size_t len) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    byte_arr_t bytes(m_alloc.template get_allocator<uint8_t>());
    bytes.reserve(len);
    bytes.insert(bytes.begin(), start, &start[len]);
    m_stack.emplace_back(m_alloc, md_value_type::MD_TYPE_BYTESTR,
                         std::move(bytes));
    return top();
  }

  /// @brief Pop an element from the top of the stack.
  ///
  /// @return The index to the top of the stack after the value has been
  /// removed or MD_E_EMPTY_STACK.
  cargo::expected<size_t, md_err> pop() {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    if (m_stack.empty()) {
      return cargo::make_unexpected(md_err::MD_E_EMPTY_STACK);
    }
    m_stack.pop_back();
    return top();
  }

  /// @brief Append a value to the end of an array.
  ///
  /// @param arr_idx The index of the array on the stack.
  /// @param elem_idx The index of the element (to be appended) on the stack.
  /// @return The index of the appended element within the array. If the action
  /// cannot be completed and unexpected md_err is returned.
  cargo::expected<size_t, md_err> arr_append(size_t arr_idx, size_t elem_idx) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    auto top_idx = top();
    if (!top_idx.has_value()) {
      return cargo::make_unexpected(top_idx.error());
    }
    if (elem_idx <= arr_idx) {
      return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
    }
    if (elem_idx > top_idx.value() || arr_idx > top_idx.value()) {
      return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
    }
    const element_t &arr = m_stack.at(arr_idx);
    if (arr.get_type() != md_value_type::MD_TYPE_ARRAY) {
      return cargo::make_unexpected(md_err::MD_E_TYPE_ERR);
    }
    auto *arr_store = arr.template get<array_t>();

    // get the value off the stack
    const element_t &elem = m_stack.at(elem_idx);
    arr_store->push_back(elem);
    return arr_store->size() - 1;
  }

  /// @brief Insert a new key-value pair into the hashtable.
  ///
  /// @param map_idx The hashtable index on the stack.
  /// @param key_idx The key index on the stack.
  /// @param val_idx The value index on the stack.
  /// @return The index within the hashtable to which the key-value pair
  /// was added. If the action cannot be completed and unexpected md_err is
  /// returned.
  cargo::expected<size_t, md_err> hash_set_kv(size_t map_idx, size_t key_idx,
                                              size_t val_idx) {
    if (finalized) {
      return cargo::make_unexpected(md_err::MD_E_STACK_FINALIZED);
    }
    auto top_idx = top();
    if (!top_idx.has_value()) {
      return cargo::make_unexpected(top_idx.error());
    }
    if (key_idx <= map_idx || val_idx <= map_idx) {
      return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
    }
    if (map_idx > top_idx.value() || key_idx > top_idx.value() ||
        val_idx > top_idx.value()) {
      return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
    }
    const element_t &map_elem = m_stack.at(map_idx);
    if (map_elem.get_type() != md_value_type::MD_TYPE_HASH) {
      return cargo::make_unexpected(md_err::MD_E_TYPE_ERR);
    }

    const element_t &key = m_stack.at(key_idx);
    auto key_type = key.get_type();
    if (key_type == md_value_type::MD_TYPE_HASH ||
        key_type == md_value_type::MD_TYPE_ARRAY ||
        key_type == md_value_type::MD_TYPE_BYTESTR) {
      return cargo::make_unexpected(md_err::MD_E_KEY_ERR);
    }
    const element_t &value = m_stack.at(val_idx);
    auto *map_store = map_elem.template get<map_t>();
    const auto inserted = map_store->insert(std::make_pair(key, value));
    if (!inserted.second) {
      return cargo::make_unexpected(md_err::MD_E_DUPLICATE_KEY);
    }
    return map_store->size() - 1;
  }

  /// @brief Finalize the stack: The stack's contents are serialized using the
  /// stored output format and written out to the binary.
  ///
  /// @param binary The binary into which the serialized bytes are written.
  /// @param endianness The desired endianness.
  void finalize(std::vector<uint8_t> &binary, MD_ENDIAN endianness) {
    switch (out_fmt) {
      case md_fmt::MD_FMT_RAW_BYTES:
        RawStackSerializer<self_t>::serialize(*this, binary, endianness);
        break;
      case md_fmt::MD_FMT_MSGPACK:
        BasicMsgPackStackSerializer<self_t>::serialize(*this, binary,
                                                       endianness);
        break;
      case md_fmt::MD_FMT_JSON:
      case md_fmt::MD_FMT_LLVM_BC_MD:
      case md_fmt::MD_FMT_LLVM_TEXT_MD:
      default:
        assert(0 && "Output format unsupported");
        break;
    }
    m_stack.clear();
  }

  /// @brief Freeze the given stack.
  ///
  /// @return MD_SUCCESS is the stack is successfully frozen.
  /// MD_E_STACK_FINALIZED if the stack was previously frozen.
  md_err freeze_stack() {
    if (finalized) {
      return md_err::MD_E_STACK_FINALIZED;
    }
    finalized = true;
    return md_err::MD_SUCCESS;
  }

  /// @brief Get a reference to the custom allocator.
  ///
  /// @return allocator_helper_t&
  const allocator_helper_t &get_alloc_helper() const { return m_alloc; }

  /// @brief Get the beginning of the stack.
  ///
  /// @return A read/write iterator to the beginning of the stack.
  iterator begin() { return m_stack.begin(); }

  /// @brief Get the beginning of the stack.
  ///
  /// @return  A read only iterator to the beginning of the stack.
  const_iterator begin() const { return m_stack.begin(); }

  /// @brief Get the end of the stack.
  ///
  /// @return A read/write iterator to the end of the stack.
  iterator end() { return m_stack.end(); }

  /// @brief Get the end of the stack.
  ///
  /// @return A read only iterator to the end of the stack.
  const_iterator end() const { return m_stack.end(); }

  /// @brief Element access to the data contained within the stack.
  ///
  /// @param idx The index of the element from which the data should be
  /// accessed.
  /// @return A read/write reference to the data.
  reference at(size_t idx) { return m_stack.at(idx); }

  /// @brief Element access to the data contained within the stack.
  ///
  /// @param idx The index of the element from which the data should be
  /// accessed.
  /// @return Read only reference to the data.
  const_reference at(size_t idx) const { return m_stack.at(idx); }

  /// @brief Check if the stack is empty.
  ///
  /// @return true if there are no elements on the stack.
  /// @return false otherwise.
  bool empty() { return m_stack.empty(); }

  /// @brief Set the output format of the stack.
  ///
  /// @param fmt The desired format.
  void set_out_fmt(md_fmt fmt) { out_fmt = fmt; }

  /// @brief Get the output format of the stack.
  ///
  /// @return md_fmt
  md_fmt get_out_fmt() const { return out_fmt; }

 private:
  allocator_helper_t m_alloc;
  stack_t m_stack;
  bool finalized;
  md_fmt out_fmt;
};

/// @}
}  // namespace md

/// @brief Implement the API definition of md_stack_ as a specific
/// instantiation of basic_stack
struct md_stack_ final : public md::basic_stack<> {
  using basic_stack::basic_stack;
};

#endif  // MD_DETAIL_MD_STACK_H_INCLUDED
