// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file callback_allocator.h
///
/// @brief Metadata API custom callback_allocator implementation.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MD_DETAIL_CALLBACK_ALLOCATOR_H_INCLUDED
#define MD_DETAIL_CALLBACK_ALLOCATOR_H_INCLUDED
#include <metadata/metadata.h>
#include <stddef.h>
#include <stdlib.h>

#include <cassert>
#include <limits>

namespace md {
/// @addtogroup md
/// @{

/// @brief A C++ allocator that wraps allocate() & deallocate() callbacks.
///
/// This class is intended to be used as a C++ allocator for metadata objects.
/// This class essentially implements the C++ allocators interface as a wrapper
/// around `allocate()` & `deallocate()` hooks so that it can be used for
/// allocations for standard stl containers and types. If no callback is
/// provided then `std::malloc()` and `std::free()` are used for in the default
/// implementation of `allocate()` and `deallocate()` respectively.
///
/// @tparam T
template <class T>
struct callback_allocator {
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using value_type = T;

  template <class U>
  struct rebind {
    using other = callback_allocator<U>;
    using type = other;
  };

  /// @brief Construct a new callback allocator object.
  ///
  /// @param hooks A pointer to the user supplied hooks.
  /// @param userdata User provided data which is passed into the callback.
  callback_allocator(md_hooks *hooks, void *userdata)
      : userdata(userdata), hooks(hooks) {}

  /// @brief Default constructor.
  callback_allocator() : hooks(nullptr), userdata(nullptr) {}

  /// @brief Copy-Construct a new callback allocator object.
  ///
  /// @tparam U The allocated type.
  /// @param other The allocator to copy from.
  template <class U>
  callback_allocator(const callback_allocator<U> &other) {
    this->userdata = other.userdata;
    this->hooks = other.hooks;
  }

  /// @brief Allocate memory.
  ///
  /// @param count The number of objects in the requested array.
  /// @return Return pointer to the allocated array/object, or nullptr.
  [[nodiscard]] pointer allocate(const size_type count) {
    assert(count < std::numeric_limits<size_type>::max() / sizeof(T));
    void *p = nullptr;
    if (hooks && hooks->allocate) {
      p = hooks->allocate(count * sizeof(value_type), alignof(value_type),
                          userdata);
    } else {
      p = std::malloc(count * sizeof(value_type));
    }
    return static_cast<T *>(p);
  };

  /// @brief Initialize an object.
  ///
  /// @param that The allocated memory.
  /// @param val The value to initialize from.
  void construct(pointer that, const_reference val) { new ((T *)that) T(val); }

  /// @brief Destroy an object.
  ///
  /// @param that The object to destroy.
  void destroy(pointer that) { that->~T(); }

  /// @brief Copy Assignment operator.
  ///
  /// @tparam U Type to be allocated.
  /// @return Copy of the allocator.
  template <class U>
  callback_allocator<U> &operator=(const callback_allocator<U> &) {
    return *this;
  }

  /// @brief Deallocate memory.
  ///
  /// @param p A pointer to the object to be deallocated.
  void deallocate(pointer p, size_type) {
    if (hooks && hooks->deallocate) {
      hooks->deallocate(p, userdata);
    } else {
      std::free(p);
    }
  };

  void *userdata = nullptr;
  md_hooks *hooks = nullptr;
};

}  // namespace md
#endif  // MD_DETAIL_CALLBACK_ALLOCATOR_H_INCLUDED
