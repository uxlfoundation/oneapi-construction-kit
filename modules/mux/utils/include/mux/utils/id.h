// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef MUX_UTILS_H_INCLUDED
#define MUX_UTILS_H_INCLUDED

#include <mux/mux.h>

namespace mux {
/// @addtogroup mux_utils
/// @{

/// @brief Construct a Mux ID by combining the target and object ID's.
///
/// @param target_id Target ID used to specify the target to invoke.
/// @param object_id Object ID used to check object type.
///
/// @return Returns the combined ID to be stored in a Mux object's `id` member
/// variable.
inline mux_id_t makeId(mux_target_id_t target_id, mux_object_id_t object_id) {
  return target_id | object_id;
}

/// @brief Get the target ID from a combined ID.
///
/// @param id Combined ID to extract target ID from.
///
/// @return Returns the extracted target ID.
inline mux_target_id_t getTargetId(mux_id_t id) {
  return ~mux_object_id_mask & id;
}

/// @brief Get the object ID from a combined ID.
///
/// @param id Combined ID to extract target ID from.
///
/// @return Returns the extracted object ID.
inline mux_object_id_t getObjectId(mux_id_t id) {
  return mux_object_id_mask & id;
}

/// @brief Set an ID from the object type and parent objects target ID.
///
/// @tparam ObjectID ID of the object.
/// @tparam Object Type of the object.
/// @param parent_id The parent objects ID, to get the target ID from.
/// @param object The object whos ID.
template <mux_object_id_e ObjectID, typename Object>
inline void setId(mux_id_t parent_id, Object *object) {
  object->id = makeId(getTargetId(parent_id), ObjectID);
}

/// @}

namespace detail {
/// @brief Helper to get a Mux object's ID from its type.
///
/// This helper object is used by ::mux::objectIsInvalid() to determine the
/// expected object ID for the given type. The ::mux::detail::get_object_id_t
/// struct must be specialized for new Mux objects.
///
/// @tparam Object Type of the object to get the ID from.
template <typename Object>
struct get_object_id_t;

/// @brief Helper macro to stamp out specializations of get_object_id_t.
///
/// @param OBJECT_TYPE The object type, ensure use of the `mux_<name>_s` type.
/// @param OBJECT_ID The object ID from `::mux_object_id_e`.
#define STAMP_GET_OBJECT_ID_T(OBJECT_TYPE, OBJECT_ID) \
  template <>                                         \
  struct get_object_id_t<OBJECT_TYPE> {               \
    static const mux_object_id_e id = OBJECT_ID;      \
  }

// NOTE: To enable object validity checking for new objects add the new call to
// the STAMP_GET_OBJECT_ID_T macro to the list below providing its type and its
// object ID.
STAMP_GET_OBJECT_ID_T(mux_device_info_s, mux_object_id_device);
STAMP_GET_OBJECT_ID_T(mux_device_s, mux_object_id_device);
STAMP_GET_OBJECT_ID_T(mux_memory_s, mux_object_id_memory);
STAMP_GET_OBJECT_ID_T(mux_buffer_s, mux_object_id_buffer);
STAMP_GET_OBJECT_ID_T(mux_image_s, mux_object_id_image);
STAMP_GET_OBJECT_ID_T(mux_sampler_s, mux_object_id_sampler);
STAMP_GET_OBJECT_ID_T(mux_queue_s, mux_object_id_queue);
STAMP_GET_OBJECT_ID_T(mux_command_buffer_s, mux_object_id_command_buffer);
STAMP_GET_OBJECT_ID_T(mux_semaphore_s, mux_object_id_semaphore);
STAMP_GET_OBJECT_ID_T(mux_executable_s, mux_object_id_executable);
STAMP_GET_OBJECT_ID_T(mux_kernel_s, mux_object_id_kernel);
STAMP_GET_OBJECT_ID_T(mux_query_pool_s, mux_object_id_query_pool);
STAMP_GET_OBJECT_ID_T(mux_sync_point_s, mux_object_id_sync_point);
STAMP_GET_OBJECT_ID_T(mux_fence_s, mux_object_id_fence);

#undef STAMP_GET_OBJECT_ID_T
}  // namespace detail

/// @addtogroup mux_utils
/// @{

/// @brief Check if an object is null or has an invalid object ID.
///
/// @tparam Object Type of the object to validate.
/// @param object Object to validate.
///
/// @return Returns true if the object is not valid, false otherwise.
template <typename Object>
inline bool objectIsInvalid(Object *object) {
  return nullptr == object ||
         detail::get_object_id_t<Object>::id != getObjectId(object->id);
}

template <>
inline bool objectIsInvalid(mux_device_t object) {
  return nullptr == object ||
         detail::get_object_id_t<mux_device_s>::id != getObjectId(object->id);
}

/// @brief Check if allocator info is malformed.
///
/// mux_allocator_info_t is malformed and therefore invalid to use if its alloc
/// or free fields are NULL.
///
/// @param allocator_info Allocator info to check.
/// @return Return true if the allocator info is not valid to use, otherwise
/// false.
inline bool allocatorInfoIsInvalid(mux_allocator_info_t allocator_info) {
  return (nullptr == allocator_info.alloc) || (nullptr == allocator_info.free);
}

/// @}
}  // namespace mux

#endif  // MUX_UTILS_H_INCLUDED
