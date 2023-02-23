// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UR_MEMORY_H_INCLUDED
#define UR_MEMORY_H_INCLUDED

#include <unordered_map>

#include "cargo/expected.h"
#include "cargo/small_vector.h"
#include "mux/mux.h"
#include "ur/base.h"
#include "ur/queue.h"

namespace ur {
/// @brief Helper type for device specific buffer.
struct device_buffer_t {
  /// @brief Target specific buffer.
  mux_buffer_t mux_buffer;
  /// @brief Target specific memory.
  mux_memory_t mux_memory;
};
}  // namespace ur

/// @brief Compute Mux specific implementation of the opaque ur_mem_handle_t_
/// API object.
struct ur_mem_handle_t_ : ur::base {
  ur_mem_handle_t_(ur_context_handle_t context, ur_mem_type_t type,
                   ur_mem_flags_t flags,
                   cargo::small_vector<ur::device_buffer_t, 4> &&buffers,
                   size_t size)
      : context{context},
        type{type},
        flags{flags},
        buffers{std::move(buffers)},
        size{size} {}
  ur_mem_handle_t_(const ur_mem_handle_t_ &) = delete;
  ~ur_mem_handle_t_();

  /// @brief Factory method for creating a buffer object.
  ///
  /// @param[in] hContext Context to which this buffer will belong.
  /// @param[in] flags Flags to create this memory buffer.
  /// @param[in] size Size of the buffer in bytes.
  /// @param[in] hostPtr Optional host pointer to try and use as memory if the
  /// user passed one.
  static cargo::expected<ur_mem_handle_t, ur_result_t> createBuffer(
      ur_context_handle_t hContext, ur_mem_flags_t flags, size_t size,
      void *hostPtr);

  /// @brief Synchronize memory state across devices.
  ///
  /// Buffers may be associated to multiple devices if they are created against
  /// a context containing multiple devices. However, commands that read and
  /// write to memory are enqueued against a command queue that is associated
  /// with a single device. This means we need a way to synchronize memory
  /// across devices after memory read/writes are enqueued to a specific command
  /// queue.
  ///
  /// @param[in] command_queue The command queue to which the memory read/write
  /// was enqueued.
  ///
  /// @return Error code indicated success of memory synchronization.
  ur_result_t sync(ur_queue_handle_t command_queue);

  /// @brief The context to which this memory belongs to.
  ur_context_handle_t context = nullptr;
  /// @brief The type of this memory object.
  const ur_mem_type_t type;
  /// @brief The flags this memory was allocated with..
  const ur_mem_flags_t flags;
  /// @brief The Device specific memory, one for each device in the context.
  /// Tagged union is used here to represent the different memory types in the
  /// same variable.
  /// TODO: Support memory types other than buffers.
  union {
    cargo::small_vector<ur::device_buffer_t, 4> buffers;
  };
  /// @brief The last queue to have modified this command buffer.
  ur_queue_handle_t last_command_queue = nullptr;
  /// @brief Size of the buffer in bytes.
  size_t size;
  /// @brief The base host pointer that will be initialized by a call to
  /// muxMapMemory then reused for subsequent mappings. Rather than making a
  /// call to muxMapMemory for each mapping to the buffer we make one mapping of
  /// the entire buffer on the first mapping, then flush at appropriate sizes
  /// and offset for subsequent mappings, only unmapping the memory when the
  /// final map is no longer in use.
  void *host_base_ptr = nullptr;
  /// @brief The current number of active mappings to this buffer.
  /// TODO: Should this be an atomic to make it thread safe?
  uint32_t map_count = 0;
  /// @brief information about mapped host pointer resulting from
  /// urEnqueueMemBufferMap.
  struct mapping_state_t {
    /// @brief Offset of any host pointer mapped to this memory.
    size_t map_offset;
    /// @brief Size of any host pointer mapped to this memory.
    size_t map_size;
  };
  /// @brief Map from host pointers to the mapping states resulting from write
  /// only map commands. For each mapping we record the size and offset so the
  /// memory can be flushed appropriately.
  std::unordered_map<void *, mapping_state_t> write_mapping_states;
  /// @brief Mutex to lock access to the map count, the mapped base pointer, and
  /// the active write mappings.
  std::mutex mutex;
};

#endif  // UR_MEMORY_H_INCLUDED
