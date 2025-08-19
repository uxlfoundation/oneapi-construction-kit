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

#include "async/clik_command_queue.h"

#include <vector>

#include "async/clik_objects.h"
#include "hal.h"

static void clik_worker_thread(clik_command_queue *queue);

// Create a command queue for the device.
clik_command_queue *clik_create_command_queue(clik_device *device) {
  if (!device) {
    return nullptr;
  }
  clik_command_queue *queue = new clik_command_queue();
  std::lock_guard<std::mutex> locker(device->lock);
  queue->device = device;
  queue->executed_timestamp = queue->dispatched_timestamp = 0;
  queue->next_command_timestamp = queue->executed_timestamp + 1;
  queue->worker_thread = new std::thread(clik_worker_thread, queue);
  device->queue = queue;
  return queue;
}

// Mark the queue as shutting down, wait for the worker thread to exit and free
// the resources used by the command queue object.
void clik_release_command_queue(clik_command_queue *queue) {
  if (!queue) {
    return;
  }

  // Dispatch any commands that have been enqueued.
  clik_dispatch(queue);

  // Shut down the queue and wait for the worker thread to be done.
  std::thread *worker_thread = nullptr;
  {
    std::lock_guard<std::mutex> locker(queue->device->lock);
    worker_thread = queue->worker_thread;

    // Wake up the worker thread.
    queue->shutdown = true;
    queue->dispatched.notify_all();
  }
  queue->worker_thread->join();

  // Free resources allocated for the queue.
  {
    std::lock_guard<std::mutex> locker(queue->device->lock);
    delete queue->worker_thread;
    delete queue;
  }
}

// Get the command queue exposed by the device.
clik_command_queue *clik_get_device_queue(clik_device *device) {
  if (!device) {
    return nullptr;
  }
  std::unique_lock<std::mutex> locker(device->lock);
  return device->queue;
}

// Entry point for the command queue's worker thread. This thread is responsible
// for executing commands on the device once they have been dispatched.
void clik_worker_thread(clik_command_queue *queue) {
  std::unique_lock<std::mutex> locker(queue->device->lock);
  clik_device *device = queue->device;
  while (true) {
    if (queue->shutdown) {
      break;
    }

    uint64_t previous_executed = queue->executed_timestamp;
    uint64_t current = queue->dispatched_timestamp;
    for (auto I = queue->commands.begin(); I != queue->commands.end();) {
      clik_command *cmd = *I;
      if (cmd->timestamp > current) {
        // The timestamp of that command indicates that it has not been
        // dispatched yet.
        break;
      }

      // Execute the command, without the lock held. This would prevent the main
      // thread from enqueueing new commands until this command has finished.
      locker.unlock();
      bool success = clik_execute_command(device, cmd);
      if (!success) {
        fprintf(stderr, "error: command with type %d failed\n", (int)cmd->type);
      }
      locker.lock();
      queue->executed_timestamp = cmd->timestamp;

      // Remove the command from the queue.
      clik_release_command(cmd);
      I = queue->commands.erase(I);
    }

    // Notify clik_wait that commands have been executed.
    if (queue->executed_timestamp > previous_executed) {
      queue->executed.notify_all();
    }

    // Wait for something to happen:
    //   1) Commands have been dispatched
    //   2) The queue is shutting down
    queue->dispatched.wait(locker);
  }
}

// Execute the given command on the device. Return true if the command was
// successfully executed.
bool clik_execute_command(clik_device *device, clik_command *command) {
  switch (command->type) {
    default:
      return false;
    case clik_command_type::read_buffer:
      return clik_execute_read_buffer(device, command->read_buffer);
    case clik_command_type::write_buffer:
      return clik_execute_write_buffer(device, command->write_buffer);
    case clik_command_type::run_kernel:
      return clik_execute_run_kernel(device, command->run_kernel);
    case clik_command_type::copy_buffer:
      return clik_execute_copy_buffer(device, command->copy_buffer);
  }
}

// Execute a 'read buffer' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_read_buffer(clik_device *device, read_buffer_args &args) {
  return device->hal_device->mem_read(
      args.dst, args.buffer->device_addr + args.offset, args.size);
}

// Execute a 'write buffer' command on the device. Return true if the command
// was successfully executed.
bool clik_execute_write_buffer(clik_device *device, write_buffer_args &args) {
  return device->hal_device->mem_write(args.buffer->device_addr + args.offset,
                                       args.src, args.size);
}

// Execute a 'copy buffer' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_copy_buffer(clik_device *device, copy_buffer_args &args) {
  uint64_t dst_addr = args.dst_buffer->device_addr + args.dst_offset;
  uint64_t src_addr = args.src_buffer->device_addr + args.src_offset;
  return device->hal_device->mem_copy(dst_addr, src_addr, args.size);
}

// Execute a 'run kernel' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_run_kernel(clik_device *device, run_kernel_args &args) {
  hal::hal_device_t *hal_device = device->hal_device;

  // Copy scheduling information.
  const clik_ndrange &nd_range(args.kernel->nd_range);
  hal::hal_ndrange_t ndrange;
  hal::hal_size_t work_group_size = 1;
  for (uint32_t i = 0; i < nd_range.max_dimensions; i++) {
    ndrange.offset[i] = (i < nd_range.dims) ? nd_range.offset[i] : 0;
    ndrange.local[i] = (i < nd_range.dims) ? nd_range.local[i] : 1;
    ndrange.global[i] = (i < nd_range.dims) ? nd_range.global[i] : 1;
    work_group_size *= ndrange.local[i];
  }
  if (work_group_size == 0) {
    // Do not allow a local size of zero in any dimension.
    return false;
  }

  // Translate clik arguments to HAL arguments.
  std::vector<hal::hal_arg_t> hal_args;
  for (uint32_t i = 0; i < args.kernel->num_args; i++) {
    const clik_argument &arg(args.kernel->args[i]);
    hal::hal_arg_t hal_arg;
    switch (arg.type) {
      default:
        return false;
      case clik_argument_type::buffer:
        hal_arg.kind = hal::hal_arg_address;
        hal_arg.space = hal::hal_space_global;
        hal_arg.size = 0;
        hal_arg.address = arg.buffer->device_addr;
        break;
      case clik_argument_type::byval:
        hal_arg.kind = hal::hal_arg_value;
        hal_arg.space = hal::hal_space_global;
        hal_arg.size = arg.size;
        hal_arg.pod_data = arg.contents;
        break;
      case clik_argument_type::local:
        hal_arg.kind = hal::hal_arg_address;
        hal_arg.space = hal::hal_space_local;
        hal_arg.size = arg.size;
        hal_arg.address = hal::hal_nullptr;
        break;
    }
    hal_args.push_back(hal_arg);
  }

  if (!hal_device->kernel_exec(
          args.kernel->program->elf, args.kernel->function_addr, &ndrange,
          hal_args.data(), hal_args.size(), nd_range.dims)) {
    return false;
  }
  return true;
}

// Start executing enqueued commands on the device.
//
// Returns true if any commands have been dispatched by this call.
bool clik_dispatch(clik_command_queue *queue) {
  if (!queue) {
    return false;
  }
  std::lock_guard<std::mutex> locker(queue->device->lock);
  uint64_t most_recent_timestamp = 0;
  for (clik_command *cmd : queue->commands) {
    most_recent_timestamp = std::max(most_recent_timestamp, cmd->timestamp);
  }
  if (most_recent_timestamp <= queue->dispatched_timestamp) {
    return false;
  }
  queue->dispatched_timestamp = most_recent_timestamp;
  // Wake up the worker thread.
  queue->dispatched.notify_all();
  return true;
}

// Wait until enqueued commands have finished executing on the device.
//
// clik_dispatch must have been called previously or this function will return
// without waiting.
bool clik_wait(clik_command_queue *queue) {
  if (!queue) {
    return false;
  }
  std::unique_lock<std::mutex> locker(queue->device->lock);
  // Wait until the worker thread has executed all commands enqueued before the
  // most recent call to clik_dispatch.
  uint64_t current_timestamp = queue->dispatched_timestamp;
  while (queue->executed_timestamp < current_timestamp) {
    queue->executed.wait(locker);
  }
  return true;
}

// Enqueue a command to read the contents of a buffer back to host memory.
bool clik_enqueue_read_buffer(clik_command_queue *queue, void *dst,
                              clik_buffer *src, uint64_t src_offset,
                              uint64_t size) {
  if (!queue || !src || !dst) {
    return false;
  }
  std::lock_guard<std::mutex> locker(queue->device->lock);
  if ((src_offset + size) > src->size) {
    return false;
  }
  clik_command *cmd =
      clik_create_command(queue, clik_command_type::read_buffer);
  if (!cmd) {
    return false;
  }
  cmd->read_buffer.buffer = src;
  cmd->read_buffer.offset = src_offset;
  cmd->read_buffer.size = size;
  cmd->read_buffer.dst = dst;
  queue->commands.push_back(cmd);
  return true;
}

// Enqueue a command to write host data to device memory.
bool clik_enqueue_write_buffer(clik_command_queue *queue, clik_buffer *dst,
                               uint64_t dst_offset, const void *src,
                               uint64_t size) {
  if (!queue || !src || !dst) {
    return false;
  }
  std::lock_guard<std::mutex> locker(queue->device->lock);
  if ((dst_offset + size) > dst->size) {
    return false;
  }
  clik_command *cmd =
      clik_create_command(queue, clik_command_type::write_buffer);
  if (!cmd) {
    return false;
  }
  cmd->write_buffer.buffer = dst;
  cmd->write_buffer.offset = dst_offset;
  cmd->write_buffer.size = size;
  cmd->write_buffer.src = src;
  queue->commands.push_back(cmd);
  return true;
}

// Enqueue a command to copy data from one buffer to another buffer.
bool clik_enqueue_copy_buffer(clik_command_queue *queue, clik_buffer *dst,
                              uint64_t dst_offset, clik_buffer *src,
                              uint64_t src_offset, uint64_t size) {
  if (!queue || !src || !dst) {
    return false;
  }
  std::lock_guard<std::mutex> locker(queue->device->lock);
  if ((dst_offset + size) > dst->size) {
    return false;
  } else if ((src_offset + size) > src->size) {
    return false;
  }
  clik_command *cmd =
      clik_create_command(queue, clik_command_type::copy_buffer);
  if (!cmd) {
    return false;
  }
  cmd->copy_buffer.dst_buffer = dst;
  cmd->copy_buffer.dst_offset = dst_offset;
  cmd->copy_buffer.src_buffer = src;
  cmd->copy_buffer.src_offset = src_offset;
  cmd->copy_buffer.size = size;
  queue->commands.push_back(cmd);
  return true;
}

// Enqueue a command to execute a kernel on the device.
bool clik_enqueue_kernel(clik_command_queue *queue, clik_kernel *kernel) {
  if (!queue || !kernel) {
    return false;
  }
  std::lock_guard<std::mutex> locker(queue->device->lock);
  clik_command *cmd = clik_create_command(queue, clik_command_type::run_kernel);
  if (!cmd) {
    return false;
  }
  cmd->run_kernel.kernel = kernel;
  queue->commands.push_back(cmd);
  return true;
}
