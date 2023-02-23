// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file allocator_helper.h
///
/// @brief Metadata API C++ allocation helper.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MD_DETAIL_ALLOCATOR_HELPER_H_INCLUDED
#define MD_DETAIL_ALLOCATOR_HELPER_H_INCLUDED
#include <metadata/detail/callback_allocator.h>
#include <metadata/metadata.h>

#include <memory>
namespace md {
/// @addtogroup md
/// @{

template <template <class U> class AllocatorType = md::callback_allocator>
struct AllocatorHelper {
  /// @brief Construct a new Allocator Helper object.
  ///
  /// @param hooks A pointer to the hooks object.
  /// @param userdata A void pointer to user supplied data.
  AllocatorHelper(md_hooks *hooks, void *userdata)
      : m_hooks(hooks), m_userdata(userdata) {}

  /// @brief Get an allocator object for a specific type.
  ///
  /// @tparam T The allocation type for the allocator.
  /// @return AllocatorType<T>
  template <class T>
  AllocatorType<T> get_allocator() const {
    return AllocatorType<T>(m_hooks, m_userdata);
  }

  /// @brief Constructs a shared_ptr to an object of type T with the custom
  /// allocator.
  ///
  /// @tparam T The type to be constructed.
  /// @tparam Args The type parameter list.
  /// @param args Arguments forwarded to T's constructor
  /// @return std::shared_ptr<T>
  template <class T, class... Args>
  std::shared_ptr<T> allocate_shared(Args... args) {
    return std::allocate_shared<T, AllocatorType<std::shared_ptr<T>>>(
        get_allocator<std::shared_ptr<T>>(), std::forward<Args>(args)...);
  }

  md_hooks *m_hooks;
  void *m_userdata;
};

/// @}
}  // namespace md
#endif  // MD_DETAIL_ALLOCATOR_HELPER_H_INCLUDED
