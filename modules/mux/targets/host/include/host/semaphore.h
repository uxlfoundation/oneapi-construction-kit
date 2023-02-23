// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// Host's semaphore interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_SEMAPHORE_H_INCLUDED
#define HOST_SEMAPHORE_H_INCLUDED

#include <host/host.h>
#include <mux/mux.h>
#include <mux/utils/small_vector.h>

#include <mutex>

namespace host {
/// @addtogroup host
/// @{

struct command_buffer_s;
struct queue_s;

struct semaphore_s final : public mux_semaphore_s {
  explicit semaphore_s(mux_device_t device,
                       mux_allocator_info_t allocator_info);

  ~semaphore_s();

  void signal(bool terminate = false);

  mux_result_t addWait(mux_command_buffer_t group);

  void reset();

 private:
  bool signalled;
  bool failed;
  mux::small_vector<mux_command_buffer_t, 8> waitingGroups;
};

/// @}
}  // namespace host

#endif  // HOST_SEMAPHORE_H_INCLUDED
