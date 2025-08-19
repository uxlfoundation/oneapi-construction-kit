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

#ifndef _CLIK_RUNTIME_CLIK_COMMAND_QUEUE_H
#define _CLIK_RUNTIME_CLIK_COMMAND_QUEUE_H

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "clik_async_api.h"

struct clik_command;

struct read_buffer_args;
struct write_buffer_args;
struct copy_buffer_args;
struct run_kernel_args;

// Holds the state needed to asynchronously execute commands on the device.
// Commands are executed in-order.
struct clik_command_queue {
  // Device used to execute commands.
  clik_device *device;
  // List of commands, in the order they have been enqueued. Commands are
  // removed from the list once they have been executed.
  std::list<clik_command *> commands;
  // Used to wake up the worker thread when commands have been dispatched as
  // well as to wake up the main thread waiting for commands to be executed.
  std::condition_variable dispatched;
  // Used to wake up the main thread waiting for commands to be executed.
  std::condition_variable executed;
  // Thread that executes commands.
  std::thread *worker_thread;
  // Timestamp for the next command to be enqueued.
  uint64_t next_command_timestamp;
  // When this is set, commands with a smaller timestamp will be executed by
  // the worker thread.
  uint64_t dispatched_timestamp;
  // Largest timestamp of an executed command. Since the queue is in-order, this
  // is the timestamp of the most recently executed command or zero if no
  // commands have been executed.
  uint64_t executed_timestamp;
  // The queue is shutting down and the worker thread will exit once all
  // commands have been executed.
  bool shutdown;
};

// Create a command queue for the device.
clik_command_queue *clik_create_command_queue(clik_device *device);

// Mark the queue as shutting down, wait for the worker thread to exit and free
// the resources used by the command queue object.
void clik_release_command_queue(clik_command_queue *queue);

// Execute the given command on the device. Return true if the command was
// successfully executed.
bool clik_execute_command(clik_device *device, clik_command *command);

// Execute a 'read buffer' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_read_buffer(clik_device *device, read_buffer_args &args);

// Execute a 'write buffer' command on the device. Return true if the command
// was successfully executed.
bool clik_execute_write_buffer(clik_device *device, write_buffer_args &args);

// Execute a 'copy buffer' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_copy_buffer(clik_device *device, copy_buffer_args &args);

// Execute a 'run kernel' command on the device. Return true if the command was
// successfully executed.
bool clik_execute_run_kernel(clik_device *device, run_kernel_args &args);

#endif
