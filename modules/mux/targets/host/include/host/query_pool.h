// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Host query pool implementation.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_QUERY_POOL_H_INCLUDED
#define HOST_QUERY_POOL_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/expected.h>
#include <host/host.h>
#include <mux/utils/allocator.h>

#include <cassert>
#include <mutex>

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
#include <pthread.h>
#endif

namespace host {
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
/// @brief Specialized query counter result struct that adds a discriminator for
/// the union.
///
/// We need this for host because we can end up using different members of the
/// union, and in C++ unions do not type pun, it is UB to access an inactive
/// union member.
struct host_query_counter_result_s : mux_query_counter_result_s {
  /// @brief Storage type enum denoting which member of the base class's union
  /// is active.
  mux_query_counter_storage_e storage;
};

/// @brief Struct to track a PAPI event set and the results read out of it.
struct host_papi_event_info_s {
  /// @brief ID used by PAPI to identify the event set.
  int papi_event_set;
  /// @brief Handle to the thread the event set is associated with.
  pid_t thread_id;
  /// @brief Array view the results should be accessed through.
  cargo::array_view<host_query_counter_result_s> results;
  /// @brief Buffer to store the results read from the event set.
  host_query_counter_result_s *result_buffer;
};
#endif

/// @brief Pool of storage for query results.
struct query_pool_s final : mux_query_pool_s {
  /// @brief Create a new query pool object.
  ///
  /// @param query_type Type of results the query pool will store.
  /// @param query_count Number of `uint64_t` query slots to allocate.
  /// @param allocator Mux allocator used for allocations.
  /// @param query_configs Query counter configs, may be null if this is a
  /// duration query pool.
  /// @param queue Queue the query pool is to be used with, may be null if this
  /// is a duration query pool.
  ///
  /// @return Returns a newly constructed query pool on success, or
  /// `mux_error_out_of_memory` on failure.
  static cargo::expected<query_pool_s *, mux_result_t> create(
      mux_query_type_e query_type, uint32_t query_count,
      mux::allocator allocator,
      const mux_query_counter_config_t *query_configs = nullptr,
      mux_queue_t queue = nullptr);

  /// @brief Get the duration query at the given index.
  ///
  /// @param index Index of the query to get.
  ///
  /// @return Returns a pointer to the duration query storage.
  mux_query_duration_result_t getDurationQueryAt(uint32_t index) {
    assert(this->type == mux_query_type_duration &&
           "type must be mux_query_type_duration");
    return static_cast<mux_query_duration_result_t>(this->data) + index;
  }

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  /// @brief Start measuring all the events associated with the pool.
  void startEvents();

  /// @brief Stop measuring events and read the results into `data`.
  void endEvents();

  /// @brief Delete the pool's events and their associated result buffers.
  ///
  /// Should only be used during object teardown.
  ///
  /// @param allocator The allocator used to allocate this query pool's memory.
  void freeEvents(mux::allocator &allocator);

  /// @brief Read results from our papi event set and return in the given
  /// buffer.
  ///
  /// @param results Pointer to array of `result_count`
  /// `mux_query_counter_result_s` structs to return the results in.
  /// @param result_count Number results to read out.
  /// @param query_index Offset into the pool's query slots to start reading
  /// from.
  mux_result_t readPapiResults(mux_query_counter_result_s *results,
                               size_t result_count, size_t query_index);
#endif

  /// @brief Reset the query pool result storage to zeros.
  void reset();

  /// @brief Reset a region of the query pool result storage to zeros.
  ///
  /// @param[in] offset Offset into the query pool's slots.
  /// @param[in] count The number of query slots to reset.
  void reset(size_t offset, size_t count);

  size_t getSize() const { return size; }

  size_t getStride() const { return stride; }

 private:
  /// @brief Private default constructor, use `host::query_poos_s::create()`.
  query_pool_s() = default;
  /// @brief Object is non-copyable.
  query_pool_s(const query_pool_s &) = delete;
  /// @brief Object is non-copyable.
  query_pool_s &operator=(const query_pool_s &) = delete;

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  /// @brief The event sets created for this query pool, one per worker thread.
  cargo::array_view<host_papi_event_info_s> papi_event_infos;
#endif

  /// @brief Pointer to memory used to store query result data.
  void *data;
  /// @brief Size in bytes of memory pointed to by `data`.
  size_t size;
  /// @brief Stride of the values stored in `data`.
  size_t stride;
};
}  // namespace host

#endif  // HOST_QUERY_POOL_H_INCLUDED
