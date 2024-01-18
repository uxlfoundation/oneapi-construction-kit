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
/// @brief Base class and reference counter API for all OpenCL API objects.

#ifndef CL_BASE_H_INCLUDED
#define CL_BASE_H_INCLUDED

#include <CL/cl.h>
#include <cl/macros.h>
#include <extension/config.h>

#include <atomic>
#include <string>
#ifdef OCL_EXTENSION_cl_khr_icd
#include <extension/khr_icd.h>
#endif

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Enumeration of reference counting initialization types.
enum class ref_count_type {
  /// @brief Initialize internal and external reference count to 1.
  INTERNAL = 1,
  /// @brief Initialize internal reference count to 1, external to 0.
  EXTERNAL = 2
};

/// @brief Get specific OpenCL object type invalid return code.
///
/// @tparam T Type of the object.
///
/// @return Returns the `CL_INVALID_<OBJECT>` which relates to `T`.
template <class T>
cl_int invalid();

/// @brief Base class of all OpenCL API object definitions.
///
/// The `::cl::base` class template makes use of CRTP (Curiously Recurring
/// Template Pattern) to ensure that the `icd_dispatch_table_ptr` data member
/// is located at the beginning of the objects storage.
///
/// ```cpp
/// struct _cl_context : public cl::base<_cl_context> {};
/// ```
///
/// @note Derived classes **must not** use virtual destructors, destruction of
/// OpenCL API objects is performed when the internal and external reference
/// counts are zero in the body of the `::cl::releaseInternal` or
/// `::cl::releaseExternal` template functions, whichever is the last to reach
/// zero.
template <typename T>
class base {
  /// @brief Pointer to the ICD dispatch table.
  ///
  /// This **must** be the **first** field of the object the ICD loader
  /// requires this field be in this location.
  ///
  /// @note Virtual functions are **not** allowed because they occupy the first
  /// bytes of the object and would displace the pointer to the dispatch table.
  const void *icd_dispatch_table_ptr;

  /// @brief Pointer type of the inheriting object.
  using object_type = T *;

 public:
  /// @brief Construct and initialize internal or external reference count.
  ///
  /// An object created via the OpenCL API must start with an external
  /// reference count of 1 and an internal reference count of 1.
  ///
  /// An object that is created internally, such as a `_cl_event`, which may be
  /// passed to an external CL API user should start with an external reference
  /// count of 1 and an internal reference count of 1.
  ///
  /// An object created internally that is not become accessible to the OpenCL
  /// application must start with an external reference count of 0 and internal
  /// reference count of 1.
  ///
  /// @param[in] type Type of reference counting to initialize.
  explicit base(const ref_count_type type)
      : icd_dispatch_table_ptr(nullptr),
        ref_count_external(ref_count_type::EXTERNAL == type),
        ref_count_internal(ref_count_type::INTERNAL == type ||
                           ref_count_type::EXTERNAL == type) {
#ifdef OCL_EXTENSION_cl_khr_icd
    icd_dispatch_table_ptr = extension::khr_icd::GetIcdDispatchTable();
#endif
  }

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  base(base &&) = delete;

  /// @brief Increment the external reference count.
  ///
  /// @note This should only be invoked by `::cl::retainExternal`.
  ///
  /// @return Returns CL_SUCCESS, CL_OUT_OF_RESOURCES if an overflow occurs, or
  /// CL_INVALID_<OBJECT> if the external reference count is 0.
  cl_int retainExternal() {
    cl_uint last_ref_count = ref_count_external;
    cl_uint next_ref_count;
    do {
      if (0u == last_ref_count) {
        return invalid<object_type>();
      }

      next_ref_count = last_ref_count + 1;
      // Check for overflow.
      if (next_ref_count < last_ref_count) {
        return CL_OUT_OF_RESOURCES;
      }
    } while (!ref_count_external.compare_exchange_weak(last_ref_count,
                                                       next_ref_count));
    return CL_SUCCESS;
  }

  /// @brief Decrement external reference count.
  ///
  /// @note This should only be invoked by `::cl::releaseExternal`.
  ///
  /// @param[in,out] should_destroy Set to true if the object is not referenced
  /// anymore and should be destroyed, set to false if the object is still
  /// referenced. Returns true if the internal reference count reached 0 as a
  /// result of this invocation, false otherwise.
  ///
  /// @return Returns CL_SUCCESS, CL_INVALID_<OBJECT> if the external reference
  /// count is 0.
  cl_int releaseExternal(bool &should_destroy) {
    cl_uint last_ref_count = ref_count_external;
    cl_uint next_ref_count;
    do {
      if (0u == last_ref_count) {
        return invalid<object_type>();
      }
      next_ref_count = last_ref_count - 1;
    } while (!ref_count_external.compare_exchange_weak(last_ref_count,
                                                       next_ref_count));
    // Last external reference is gone, release the single internal reference
    // associated with non-zero external reference counts.
    if (0u == next_ref_count) {
      releaseInternal(should_destroy);
    }
    return CL_SUCCESS;
  }

  /// @brief Return external reference count.
  ///
  /// @note The returned value should be immediately considered as stale.
  ///
  /// @return External reference count valid during the time of the call.
  cl_uint refCountExternal() const { return ref_count_external; }

  /// @brief Increment the internal reference count.
  ///
  /// @note This should only be invoked by `::cl::retainInternal` or
  /// `::cl::base::retainExternal`.
  ///
  /// @return CL_SUCCESS on success, CL_OUT_OF_RESOURCES if retain results in an
  /// overflow.
  cl_int retainInternal() {
    cl_uint last_ref_count = ref_count_internal;
    cl_uint next_ref_count;
    do {
      OCL_ASSERT(0u != last_ref_count,
                 "Cannot retain object with internal reference count of zero.");
      next_ref_count = last_ref_count + 1;
      // Check for overflow.
      if (next_ref_count < last_ref_count) {
        return CL_OUT_OF_RESOURCES;
      }
    } while (!ref_count_internal.compare_exchange_weak(last_ref_count,
                                                       next_ref_count));
    return CL_SUCCESS;
  }

  /// @brief Decrement the internal reference count.
  ///
  /// @note This should only be invoked by `::cl::releaseInternal` or
  /// `::cl::base::releaseInternal`.
  ///
  /// @param[out] should_destroy Returns true if the object can be destroyed,
  /// false otherwise.
  void releaseInternal(bool &should_destroy) {
    cl_uint last_ref_count = ref_count_internal;
    cl_uint next_ref_count;
    do {
      OCL_ASSERT(
          0u < last_ref_count,
          "Cannot release object with internal reference count of zero.");
      next_ref_count = last_ref_count - 1;
    } while (!ref_count_internal.compare_exchange_weak(last_ref_count,
                                                       next_ref_count));
    if (0u == next_ref_count) {
      OCL_ASSERT(0u == ref_count_external,
                 "Internal reference count cannot be zero when external "
                 "reference count is non-zero.");
      should_destroy = true;
    } else {
      should_destroy = false;
    }
  }

  /// @brief Return internal reference count.
  ///
  /// The returned value should be immediately considered as stale.
  ///
  /// @return Returns the internal reference count.
  cl_uint refCountInternal() const { return ref_count_internal; };

 private:
  /// @brief The external reference count, exposed to the OpenCL application.
  std::atomic<cl_uint> ref_count_external;
  /// @brief The internal reference count.
  std::atomic<cl_uint> ref_count_internal;
};

/// @brief Guard object to release an object on scope exit.
///
/// @tparam T Type of the object to release.
template <class T>
class release_guard {
 public:
  using object_type = T;

  /// @brief Constructor.
  ///
  /// @param object Object to be guarded.
  /// @param type Type of reference counter to release.
  release_guard(object_type object, const ref_count_type type)
      : object(object), type(type) {}

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  release_guard(release_guard &&) = delete;

  /// @brief Destructor.
  ~release_guard() {
    if (object) {
      switch (type) {
        case ref_count_type::EXTERNAL: {
          const cl_int retcode = releaseExternal(object);
          OCL_ASSERT(CL_SUCCESS == retcode, "External release failed!");
          OCL_UNUSED(retcode);
          break;
        }
        case ref_count_type::INTERNAL: {
          releaseInternal(object);
          break;
        }
        default: {
          OCL_ABORT("Unknown cl::ref_count_type!");
        }
      }
    }
  }

  /// @brief Determine if the object is valid.
  ///
  /// @return Returns true if the object is valid, false otherwise.
  explicit operator bool() const { return nullptr != object; }

  /// @brief Access the guarded objects members.
  ///
  /// @return Returns the object.
  object_type operator->() const { return object; }

  /// @brief Access the object.
  ///
  /// @return Returns a reference to the object.
  object_type &get() { return object; }

  /// @brief Access the object.
  ///
  /// @return Returns a const reference to the object.
  const object_type &get() const { return object; }

  /// @brief Dismiss the object from being released at scope exit.
  ///
  /// @return Returns the object.
  object_type dismiss() {
    object_type ret = object;
    object = nullptr;
    return ret;
  }

 private:
  /// @brief Object to be guarded.
  object_type object;
  /// @brief Type of reference counter to release.
  ref_count_type type;
};

/// @brief Increment an objects external reference count.
///
/// @tparam T Type of the object.
/// @param object Object to increment the reference count on.
///
/// @return Returns CL_SUCCESS, CL_OUT_OF_RESOURCES if an overflow occurs,
/// CL_INVALID_<OBJECT> if the external reference count is zero.
template <class T>
cl_int retainExternal(T object) {
  if (!object) {
    return invalid<T>();
  }
  return object->retainExternal();
}

/// @brief Decrement an objects external reference count.
///
/// @tparam T Type of the object.
/// @param object Object to decrement the reference count on, must not be
/// allocated with `new T[]`.
///
/// @return Returns CL_SUCCESS, CL_INVALID_<OBJECT> if the external reference
/// count is zero.
template <class T>
cl_int releaseExternal(T object) {
  if (!object) {
    return invalid<T>();
  }
  bool should_destroy = false;
  const cl_int error = object->releaseExternal(should_destroy);
  if (error) {
    return error;
  }
  if (should_destroy) {
    delete object;
  }
  return CL_SUCCESS;
}

/// @brief Declare specialization for `cl_mem`.
///
/// Destruction of `cl_mem` objects differs depending on if it represents a
/// buffer or an image, declaring the specialization here is a work around for
/// multiply defined symbols in Visual Studio which will instantiate the
/// template function in addition to the specialzation.
///
/// @param object Object to decrement the reference count on, must not be
/// allocated with `new T[]`.
///
/// @return Returns CL_SUCCESS, CL_INVALID_<OBJECT> if the external reference
/// count is zero.
template <>
cl_int releaseExternal<cl_mem>(cl_mem object);

/// @brief Increment an object's internal reference count.
///
/// @tparam T Type of the object.
/// @param object Object to increment the reference count on.
///
/// @return Returns CL_SUCCESS, CL_OUT_OF_RESOURCES if an overflow occurs,
/// or CL_INVALID_<OBJECT> if @p object is not valid.
template <class T>
cl_int retainInternal(T object) {
  if (!object) {
    return invalid<T>();
  }
  return object->retainInternal();
}

/// @brief Decrement an objects internal reference count.
///
/// @tparam T Type of the object.
/// @param object Object to decrement the reference count on, must not be
/// allocated with `new T[]`.
template <class T>
void releaseInternal(T object) {
  if (!object) {
    return;
  }
  bool should_destroy = false;
  object->releaseInternal(should_destroy);
  if (should_destroy) {
    delete object;
  }
}

/// @brief Declare specialization for `cl_mem`.
///
/// Destruction of `cl_mem` objects differs depending on if it represents a
/// buffer or an image, declaring the specialization here is a work around for
/// multiply defined symbols in Visual Studio which will instantiate the
/// template function in addition to the specialzation.
///
/// @param object Object to decrement the reference count on, must not be
/// allocated with `new T[]`.
template <>
void releaseInternal<cl_mem>(cl_mem object);

/// @}
}  // namespace cl

#endif  // CL_BASE_H_INCLUDED
