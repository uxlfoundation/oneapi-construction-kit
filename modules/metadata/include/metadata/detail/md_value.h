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

/// @file md_value.h
///
/// @brief Metadata API Value.

#ifndef MD_DETAIL_MD_VALUE_H_INCLUDED
#define MD_DETAIL_MD_VALUE_H_INCLUDED
#include <metadata/detail/allocator_helper.h>
#include <metadata/metadata.h>

namespace md {
/// @addtogroup md
/// @{

/// @brief Represents a basic value which can be pushed to a stack.
///
/// @tparam AllocatorType A custom allocator type to handle allocations.
template <template <class U> class AllocatorType = md::callback_allocator>
class basic_value {
  using allocator_helper_t = AllocatorHelper<AllocatorType>;

  /// @brief Runtime Polymorphism pattern adapted from Sean Parent's conference
  /// talk at NDC London 2017 - "Better Code: Runtime Polymorphism".
  struct type_concept {
    virtual ~type_concept() = default;
  };
  template <class BasicType>
  struct storage final : type_concept {
    storage(BasicType x) : m_data(std::move(x)) {}
    BasicType m_data;
  };

  allocator_helper_t m_alloc;
  md_value_type m_type;
  std::shared_ptr<type_concept> m_data;

 public:
  /// @brief Construct a new basic value object.
  ///
  /// @tparam T The concrete type of the value to be stored.
  /// @tparam Args The template parameter list.
  /// @param alloc A reference to an allocator helper.
  /// @param value_type A tag type hint.
  /// @param obj The object to be created.
  /// @param args Arguments forwarded to the type's constructor.
  template <class T, class... Args>
  basic_value(allocator_helper_t alloc, md_value_type value_type, T &&obj,
              Args... args)
      : m_alloc(std::move(alloc)),
        m_type(value_type),
        m_data(m_alloc.template allocate_shared<storage<T>>(
            std::forward<T>(obj, args...))) {}

  /// @brief Default virtual destructor for inheritance.
  virtual ~basic_value() = default;

  /// @brief Get a pointer to the underlying data object.
  ///
  /// WARNING: If a different type is requested that differs from the actual
  /// underlying type this will result in UB.
  ///
  /// @tparam T The underlying data type.
  /// @return T* to the stored data object.
  template <class T>
  T *get() const {
    auto *store = static_cast<storage<T> *>(m_data.get());
    return static_cast<T *>(&store->m_data);
  }

  /// @brief Get the type tag the value.
  ///
  /// @return md_value_type
  md_value_type get_type() const { return m_type; }

  /// @brief Get a reference to the custom allocator.
  ///
  /// @return Reference to an allocator helper used to allocated memory.
  allocator_helper_t &get_alloc_helper() { return m_alloc; }

  /// @brief Equality operator.
  ///
  /// @param rhs The object to compare against.
  /// @return `true` if data and tag match, `false` otherwise.
  inline bool operator==(const basic_value &rhs) const {
    return m_type == rhs.m_type && m_data == rhs.m_data;
  }
};

/// @}
}  // namespace md

/// @brief Implement the API definition of md_value_ as a specific
/// instantiation of basic_value
struct md_value_ final : public md::basic_value<> {
  using basic_value::basic_value;
};

#endif  // MD_DETAIL_MD_VALUE_H_INCLUDED
