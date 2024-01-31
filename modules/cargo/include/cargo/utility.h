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
/// @brief Implementation of future C++ `<utility>`'s.

#ifndef CARGO_UTILITY_H_INCLUDED
#define CARGO_UTILITY_H_INCLUDED

#include <cargo/type_traits.h>

#include <cstring>
#include <memory>
#include <utility>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Implementation of C++14's `std::exchange`.
///
/// ```cpp
/// struct foo {
///   foo(foo &&other) : data(cargo::exchange(other.data, nullptr)) {}
///
///   void *data;
/// };
/// ```
///
/// @tparam T Type of the object to exchange.
/// @tparam U Type of the new value to exchange.
/// @param object Object to exchange.
/// @param new_value New value of to exchange.
///
/// @note `T` must be move constructable and objects of type `T` must be move
/// constructable from objects of type `U`.
///
/// @see http://en.cppreference.com/w/cpp/utility/exchange
///
/// @return Returns the object value of `object`.
template <class T, class U = T>
T exchange(T &object, U &&new_value) {
  T old_value = std::move(object);
  object = std::forward<U>(new_value);
  return old_value;
}

/// @brief Well formed casting of the bits in one type to bits in another type.
///
/// The implementation uses `std::memcpy` to perform the bit cast in a manner
/// which is well defined according to the C++ specification and avoids
/// undefined behaviour. `cargo::bit_cast` also asserts that the size of
/// `Source` and `Dest` types are the same, that `Source` is trivially copyable
/// using `std::is_trivially_copyable`, and that `Dest` is trivial using
/// `std::is_trivial` (i.e. it is trivially default constructible and trivially
/// copyable).
///
/// @tparam Dest Type of the bits being cast to.
/// @tparam Source Type of the bits being cast from.
/// @param source A const reference to the source of the bits to cast.
///
/// @return Returns a value of type `Dest` containing the bits contained in
/// `source`.
template <typename Dest, typename Source>
inline Dest bit_cast(const Source &source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "Dest and Source must be the same size");
  static_assert(std::is_trivial_v<Dest>, "Dest must be a trivial type");
  // Older versions of GCC we support do not have std::is_trivially_copyable.
  static_assert(std::is_trivially_copyable_v<Source>,
                "Source must be trivially copyable");
  // KLOCWORK "UNINIT.STACK.MUST" possible false positive
  // Initialization of dest looks like an uninitialized access to Klocwork
  Dest dest;
  std::memcpy(std::addressof(dest), std::addressof(source), sizeof(Source));
  return dest;
}

/// @brief Construct a `std::string`-like object from a cargo container.
///
/// The source pointer and length constructor of `Dest` is used.
///
/// The existence of an iterator constructor is not checked. Even if `Dest` has
/// an iterator constructor, this specialization is preferred, because
/// construction with iterators may require const iterators, or may not support
/// them.
///
/// Note that even though `source` is `const`, if `Dest` does not contain
/// storage (e.g., `LLVM::StringRef`, `cargo::string_view`), then the returned
/// object can be used to modify `source`. `source` is const to allow it to be a
/// temporary.
///
/// The source type is included in the template parameters so that type
/// deduction can ensure that the value type of the source can be converted to
/// the value type of `Dest`. In normal usage, `Src` can be inferred.
///
/// @tparam Dest The type of the `std::string`-like object.
/// @tparam Src The source type.
/// @param[in] source Cargo container to construct from.
///
/// @return Returns the contents of `source` copied into a `Dest` object.
template <
    class Dest, class Src,
    std::enable_if_t<is_detected<detail::data_member_fn, Src>::value &&
                     is_detected<detail::size_member_fn, Src>::value &&
                     is_detected<detail::subscript_member_fn, Src>::value &&
                     is_detected<detail::subscript_member_fn, Dest>::value &&
                     is_detected<detail::ptr_len_constructor, Dest,
                                 typename Src::const_pointer>::value &&
                     has_value_type_convertible_to<typename Src::value_type,
                                                   Dest>::value> * = nullptr>
Dest as(const Src &source) {
  static_assert(!std::is_reference_v<Dest>,
                "cargo::as cannot convert to a reference");
  return Dest(source.data(), source.size());
}

/// @brief Construct a `std::vector`-like object from a cargo container.
///
/// The iterator constructor of `Dest` is used.
///
/// The iterator constructor check uses `Src::value_type`, because `Dest` is
/// constructed with `Src` iterators.
///
/// The source type is included in the template parameters so that type
/// deduction can ensure that the value type of the source can be converted to
/// the value type of `Dest`. In normal usage, `Src` can be inferred.
///
/// @tparam Dest The type of the `std::vector`-like object.
/// @tparam Src The source type.
/// @param[in] source Cargo container to construct from.
///
/// @return Returns the contents of `source` copied into a `Dest` object.
template <
    class Dest, class Src,
    std::enable_if_t<is_detected<detail::subscript_member_fn, Src>::value &&
                     is_detected<detail::subscript_member_fn, Dest>::value &&
                     is_detected<detail::iterator_constructor, Dest,
                                 typename Src::value_type *>::value &&
                     !is_detected<detail::ptr_len_constructor, Dest,
                                  typename Src::const_pointer>::value &&
                     has_value_type_convertible_to<typename Src::value_type,
                                                   Dest>::value> * = nullptr>
Dest as(const Src &source) {
  static_assert(!std::is_reference_v<Dest>,
                "cargo::as cannot convert to a reference");
  return Dest(source.cbegin(), source.cend());
}

/// @brief Catch-all specialization that produces a compiler error.
template <class Dest, class Src,
          std::enable_if_t<!is_detected<detail::iterator_constructor, Dest,
                                        typename Src::value_type *>::value &&
                           !is_detected<detail::ptr_len_constructor, Dest,
                                        typename Src::const_pointer>::value> * =
              nullptr>
Dest as(const Src) {
  // This assertion is just the opposite of the enable_if_t above, so it will
  // always trigger when this specialization is selected.
  static_assert(
      is_detected<detail::iterator_constructor, Dest,
                  typename Src::value_type *>::value ||
          is_detected<detail::ptr_len_constructor, Dest,
                      typename Src::const_pointer>::value,
      "cargo::as cannot convert to a fixed-size container (e.g., std::array) "
      "or one that requires manual allocation (e.g., cargo::dynamic_array)");
  Dest dst;
  return dst;
}

/// @brief Construct used to represent something which is empty.
///
/// Used in `cargo::optional<T>` and `cargo::expected<T,E>` to signify that no
/// value is held, essentially a bool.
class monostate {};

/// @brief A tag type to tell optional to construct its value in-place
struct in_place_t {
  explicit in_place_t() = default;
};

/// @brief A tag to tell expected to construct its value in-place
static constexpr in_place_t in_place{};

/// @}
}  // namespace cargo

#endif  // CARGO_UTILITY_H_INCLUDED
