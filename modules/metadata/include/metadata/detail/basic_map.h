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

/// @file basic_map.h
///
/// @brief Metadata API basic_map implementation.

#ifndef MD_DETAIL_BASIC_MAP_H_INCLUDED
#define MD_DETAIL_BASIC_MAP_H_INCLUDED
#include <metadata/detail/allocator_helper.h>

#include <algorithm>
#include <vector>

namespace md {
/// @addtogroup md
/// @{

/// @brief An implementation of a basic map.
///
/// This class is intended to be used as a replacement for std::map where the
/// number of key-value pairs is small. This map is implemented as std::vector
/// of std::pair(s), therefore lookups involve a linear search of all elements
/// in the vector. For a small number of key-value pairs this implementation is
/// preferable to std::map as it does not require rebalancing or rehashing. It
/// also incorporates the use of the custom allocator to handle all allocations.
///
/// @tparam KeyType
/// @tparam MappedType
/// @tparam AllocatorType
template <class KeyType, class MappedType,
          template <class U> class AllocatorType = md::callback_allocator>
class basic_map {
 public:
  using key_type = KeyType;
  using mapped_type = MappedType;
  using value_type = std::pair<key_type, mapped_type>;
  using map_type = std::vector<value_type, AllocatorType<value_type>>;
  using iterator = typename map_type::iterator;
  using const_iterator = typename map_type::const_iterator;
  using reverse_iterator = typename map_type::reverse_iterator;
  using const_reverse_iterator = typename map_type::const_reverse_iterator;
  using reference = typename map_type::reference;
  using const_reference = typename map_type::const_reference;

  using allocator_helper_t = AllocatorHelper<AllocatorType>;

  /// @brief Construct a new basic map object.
  ///
  /// @param alloc A reference to an allocator helper.
  basic_map(allocator_helper_t &alloc)
      : m_alloc(alloc), m_data(alloc.template get_allocator<value_type>()) {}

  /// @brief Returns a read/write iterator to the beginning of the map.
  ///
  /// @return iterator
  iterator begin() { return m_data.begin(); }

  /// @brief Returns a const iterator to the beginning of the map.
  ///
  /// @return const_iterator
  const_iterator begin() const { return m_data.begin(); }

  /// @brief Returns a const iterator to the beginning of the map.
  ///
  /// @return const_iterator
  const_iterator cbegin() const { return m_data.cbegin(); }

  /// @brief Returns an read/write iterator to the end of the map.
  ///
  /// @return iterator
  iterator end() { return m_data.end(); }

  /// @brief Returns an const iterator to the end of the map.
  ///
  /// @return const_iterator
  const_iterator end() const { return m_data.end(); }

  /// @brief Returns an const iterator to the end of the map.
  ///
  /// @return const_iterator
  const_iterator cend() const { return m_data.cend(); }

  /// @brief Get the beginning of a read/write reverse iterator.
  ///
  /// @return reverse_iterator
  reverse_iterator rbegin() { return m_data.rbegin(); }

  /// @brief Get the beginning of a read only reverse iterator.
  ///
  /// @return const_reverse_iterator
  const_reverse_iterator rbegin() const { return m_data.rbegin(); }

  /// @brief Get the beginning of a read only reverse iterator.
  ///
  /// @return const_reverse_iterator
  const_reverse_iterator crbegin() const { return m_data.crbegin(); }

  /// @brief Get the end of a read/write reverse iterator.
  ///
  /// @return reverse_iterator
  reverse_iterator rend() { return m_data.rend(); }

  /// @brief Get the end of a read only reverse iterator.
  ///
  /// @return const_reverse_iterator
  const_reverse_iterator rend() const { return m_data.rend(); }

  /// @brief Get the end of a read only reverse iterator.
  ///
  /// @return const_reverse_iterator
  const_reverse_iterator crend() const { return m_data.crend(); }

  /// @brief Check if the map is empty.
  ///
  /// @return true if there are no key/value pairs.
  /// @return false otherwise.
  bool empty() const { return m_data.empty(); }

  /// @brief Get the size of the map.
  ///
  /// @return size_t
  size_t size() const { return m_data.size(); }

  /// @brief Get the maximum allowed size of the map.
  ///
  /// @return size_t
  size_t max_size() const { return m_data.max_size(); }

  /// @brief Remove all key/value pairs from the map.
  void clear() { m_data.clear(); }

  /// @brief Insert a key/value pair into the map.
  ///
  /// @param value The element value to insert.
  /// @return Returns a pair consisting of an iterator to the inserted element
  /// (or to the element that prevented the insertion) and a bool denoting
  /// whether the insertion took place.
  std::pair<iterator, bool> insert(const value_type &value) {
    iterator it = find(value.first);
    if (it != end()) {
      return std::make_pair<iterator, bool>(std::move(it), false);
    }
    m_data.emplace_back(std::move(value));
    return std::make_pair<iterator, bool>(m_data.end() - 1, true);
  }

  /// @brief Remove an element at the given position.
  ///
  /// @param pos Iterator pointing to the element to be erased.
  /// @return An iterator pointing to the next element (or end()). This function
  /// will decrease the size of map by 1.
  iterator erase(iterator pos) { return m_data.erase(pos); }

  /// @brief Remove an element at the given position.
  ///
  /// @param pos A read only iterator pointing to the element to be erased.
  /// @return An iterator pointing to the next element (or end()). This function
  /// will decrease the size of map by 1.
  iterator erase(const_iterator pos) { return m_data.erase(pos); }

  /// @brief Remove a range of elements.
  ///
  /// @param first Iterator pointing to the first element to be removed.
  /// @param last Iterator pointing to the last element to be removed.
  /// @return An iterator pointing to the next element (or end()).
  iterator erase(iterator first, iterator last) {
    return m_data.erase(first, last);
  }

  /// @brief Remove a range of elements.
  ///
  /// @param first Iterator pointing to the first element to be removed.
  /// @param last Iterator pointing to the last element to be removed.
  /// @return An iterator pointing to the next element (or end()).
  iterator erase(const_iterator first, const_iterator last) {
    return m_data.erase(first, last);
  }

  /// @brief Erase a key/value pair from the map.
  ///
  /// @param key The key to look for.
  /// @return The number of key/value pairs removed. If the Key is present in
  /// the map, it is erased and 1 is returned. Otherwise, 0 is returned.
  size_t erase(const key_type &key) {
    const const_iterator it = find(key);
    if (it == end()) {
      return 0;
    }
    return erase(it), 1;
  }

  /// @brief Pre-reserve space in the map.
  ///
  /// @param n_elements The number of elements to reserve space for.
  void reserve(size_t n_elements) { m_data.reserve(n_elements); }

  /// @brief Element access to the data contained within the map.
  ///
  /// @param idx The index of the element for which data should be accessed.
  /// @return Read/Write reference to the data.
  reference at(size_t idx) { return m_data.at(idx); }

  /// @brief Element access to the data contained within the map.
  ///
  /// @param idx The index of the element for which data should be accessed.
  /// @return A read only reference to the element.
  const_reference at(size_t idx) const { return m_data.at(idx); }

  /// @brief Find an element in the map from its key.
  ///
  /// @param key The key to look for.
  /// @return A read only const_iterator pointing to the found element.
  const_iterator find(const key_type &key) const {
    return std::find_if(
        m_data.cbegin(), m_data.cend(),
        [&key](const value_type &elem) { return elem.first == key; });
  }

  /// @brief Find an element in the map from its key.
  ///
  /// @param key The key to look for.
  /// @return A read/write iterator pointing to the found element.
  iterator find(const key_type &key) {
    return std::find_if(
        m_data.begin(), m_data.end(),
        [&key](const value_type &elem) { return elem.first == key; });
  }

 private:
  allocator_helper_t m_alloc;
  map_type m_data;
};

/// @}
}  // namespace md
#endif  // MD_DETAIL_BASIC_MAP_H_INCLUDED
