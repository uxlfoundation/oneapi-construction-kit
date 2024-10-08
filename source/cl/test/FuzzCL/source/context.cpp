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

#include "FuzzCL/context.h"

#include <cmath>

void fuzzcl::fuzz_from_input(const uint8_t *data, size_t size,
                             const unsigned char **kernel_binaries,
                             const std::vector<size_t> &kernel_binary_sizes,
                             const fuzzcl::options_t options) {
  fuzzcl::context_t fc(kernel_binaries, kernel_binary_sizes, options);

  std::vector<std::thread> running_threads;
  running_threads.reserve(MAX_NUM_THREADS);
  for (int i = 0; i < MAX_NUM_THREADS; i++) {
    // only export code from one thread since they all execute the same input
    bool should_export = false;
    if (i == 0) {
      should_export = true;
    }

    const fuzzcl::input_t input(data, size, should_export);
    running_threads.emplace_back(
        std::thread(fuzzcl::run_input, std::ref(fc), input));
  }

  for (std::thread &thread : running_threads) {
    thread.join();
  }
}

void fuzzcl::run_input(fuzzcl::context_t &fc, fuzzcl::input_t input) {
  while (input.index < input.data.size()) {
    switch (input.next(0, 18)) {
      case 0:
        fuzzcl::enqueueReadBuffer(fc, input);
        break;
      case 1:
        fuzzcl::enqueueWriteBuffer(fc, input);
        break;
      case 2:
        fuzzcl::enqueueReadBufferRect(fc, input);
        break;
      case 3:
        fuzzcl::enqueueWriteBufferRect(fc, input);
        break;
      case 4:
        fuzzcl::enqueueFillBuffer(fc, input);
        break;
      case 5:
        fuzzcl::enqueueCopyBuffer(fc, input);
        break;
      case 6:
        fuzzcl::enqueueCopyBufferRect(fc, input);
        break;
      case 7:
        fuzzcl::enqueueMapBuffer(fc, input);
        break;
      case 8:
        fuzzcl::enqueueReadImage(fc, input);
        break;
      case 9:
        fuzzcl::enqueueWriteImage(fc, input);
        break;
      case 10:
        fuzzcl::enqueueFillImage(fc, input);
        break;
      case 11:
        fuzzcl::enqueueCopyImage(fc, input);
        break;
      case 12:
        fuzzcl::enqueueCopyImageToBuffer(fc, input);
        break;
      case 13:
        fuzzcl::enqueueCopyBufferToImage(fc, input);
        break;
      case 14:
        fuzzcl::enqueueMapImage(fc, input);
        break;
      case 15:
        fuzzcl::enqueueUnmapMemObject(fc, input);
        break;
      case 16:
        fuzzcl::enqueueNDRangeKernel(fc, input);
        break;
      case 17:
        fuzzcl::enqueueTask(fc, input);
        break;
      case 18:
        // clSetEventCallback will only be run outside of callbacks, if
        // callbacks are enabled
        if (fc.enable_callbacks && !input.callback_id.has_value()) {
          fuzzcl::setEventCallback(fc, input);
        }
        break;
      default:
        break;
    }
  }
  IS_CL_SUCCESS(clFlush(fc.queue));
}

void fuzzcl::enqueueReadBuffer(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_read = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_read));

  // in number of cl_int
  const size_t offset = input.next(0, BUFFER_SIZE / 2);
  const size_t size = input.next(1, BUFFER_SIZE - offset);
  assert(offset + size <= BUFFER_SIZE);

  std::vector<cl_int> *host_buffer_ptr = new std::vector<cl_int>(size);

  // locks the context_t's mutex
  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.host_buffers.emplace_back(host_buffer_ptr);

  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueReadBuffer(
      fc.queue, buffer->m, blocking_read, offset * sizeof(cl_int),
      size * sizeof(cl_int), host_buffer_ptr->data(), num_events_in_wait_list,
      event_wait_list, &event));

  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueReadBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_read_buffer(buffer->id, blocking_read, offset, size,
                            input.callback_id);
  }
}

void fuzzcl::enqueueWriteBuffer(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_write = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_write));

  // in number of cl_int
  const size_t offset = input.next(0, BUFFER_SIZE / 2);
  const size_t size = input.next(1, BUFFER_SIZE - offset);
  assert(offset + size <= BUFFER_SIZE);

  std::vector<cl_int> *host_buffer_ptr = new std::vector<cl_int>(size);

  // locks the context_t's mutex
  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.host_buffers.emplace_back(host_buffer_ptr);

  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueWriteBuffer(
      fc.queue, buffer->m, blocking_write, offset * sizeof(cl_int),
      size * sizeof(cl_int), host_buffer_ptr->data(), num_events_in_wait_list,
      event_wait_list, &event));

  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueWriteBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_write_buffer(buffer->id, blocking_write, offset, size,
                             input.callback_id);
  }
}

void fuzzcl::enqueueReadBufferRect(fuzzcl::context_t &fc,
                                   fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_read = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_read));

  // in number of cl_int
  const std::array<int, 3> buffer_origin = std::array<int, 3>{
      {input.next(0, BUFFER_WIDTH / 2), input.next(0, BUFFER_HEIGHT / 2),
       input.next(0, BUFFER_DEPTH / 2)}};
  const std::array<int, 3> host_origin = std::array<int, 3>{
      {input.next(0, buffer_origin[0]), input.next(0, buffer_origin[1]),
       input.next(0, buffer_origin[2])}};
  const std::array<int, 3> region =
      std::array<int, 3>{{input.next(1, BUFFER_WIDTH - buffer_origin[0]),
                          input.next(1, BUFFER_HEIGHT - buffer_origin[1]),
                          input.next(1, BUFFER_DEPTH - buffer_origin[2])}};

  // in bytes
  std::array<size_t, 3> *buffer_origin_ptr = new std::array<size_t, 3>{
      {buffer_origin[0] * sizeof(cl_int), static_cast<size_t>(buffer_origin[1]),
       static_cast<size_t>(buffer_origin[2])}};
  std::array<size_t, 3> *host_origin_ptr = new std::array<size_t, 3>{
      {host_origin[0] * sizeof(cl_int), static_cast<size_t>(host_origin[1]),
       static_cast<size_t>(host_origin[2])}};
  std::array<size_t, 3> *region_ptr = new std::array<size_t, 3>{
      {region[0] * sizeof(cl_int), static_cast<size_t>(region[1]),
       static_cast<size_t>(region[2])}};

  const size_t buffer_row_pitch = BUFFER_WIDTH * sizeof(cl_int);
  const size_t buffer_slice_pitch = BUFFER_HEIGHT * buffer_row_pitch;
  const size_t host_row_pitch = region[0] * sizeof(cl_int);
  const size_t host_slice_pitch = region[1] * host_row_pitch;

  assert(static_cast<size_t>(buffer_origin[2]) * buffer_slice_pitch +
             static_cast<size_t>(buffer_origin[1]) * buffer_row_pitch +
             static_cast<size_t>(buffer_origin[0]) +
             static_cast<size_t>(region[0]) * static_cast<size_t>(region[1]) *
                 static_cast<size_t>(region[2]) * sizeof(cl_int) <=
         BUFFER_SIZE * sizeof(cl_int));

  std::vector<cl_int> *host_buffer_ptr = new std::vector<cl_int>(BUFFER_SIZE);

  // locks the context_t's mutex
  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.buffer_origins.emplace_back(buffer_origin_ptr);
  fc.host_origins.emplace_back(host_origin_ptr);
  fc.regions.emplace_back(region_ptr);

  fc.host_buffers.emplace_back(host_buffer_ptr);

  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueReadBufferRect(
      fc.queue, buffer->m, blocking_read, buffer_origin_ptr->data(),
      host_origin_ptr->data(), region_ptr->data(), buffer_row_pitch,
      buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      host_buffer_ptr->data(), num_events_in_wait_list, event_wait_list,
      &event));

  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueReadBufferRect" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_read_buffer_rect(
        buffer->id, blocking_read, *buffer_origin_ptr, *host_origin_ptr,
        *region_ptr, buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
        host_slice_pitch, input.callback_id);
  }
}

void fuzzcl::enqueueWriteBufferRect(fuzzcl::context_t &fc,
                                    fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_write = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_write));

  // in number of cl_int
  const std::array<int, 3> buffer_origin = std::array<int, 3>{
      {input.next(0, BUFFER_WIDTH / 2), input.next(0, BUFFER_HEIGHT / 2),
       input.next(0, BUFFER_DEPTH / 2)}};
  const std::array<int, 3> host_origin = std::array<int, 3>{
      {input.next(0, buffer_origin[0]), input.next(0, buffer_origin[1]),
       input.next(0, buffer_origin[2])}};
  const std::array<int, 3> region =
      std::array<int, 3>{{input.next(1, BUFFER_WIDTH - buffer_origin[0]),
                          input.next(1, BUFFER_HEIGHT - buffer_origin[1]),
                          input.next(1, BUFFER_DEPTH - buffer_origin[2])}};

  // in bytes
  std::array<size_t, 3> *buffer_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(buffer_origin[0]) * sizeof(cl_int),
       static_cast<size_t>(buffer_origin[1]),
       static_cast<size_t>(buffer_origin[2])}};
  std::array<size_t, 3> *host_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(host_origin[0]) * sizeof(cl_int),
       static_cast<size_t>(host_origin[1]),
       static_cast<size_t>(host_origin[2])}};
  std::array<size_t, 3> *region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(region[0]) * sizeof(cl_int),
       static_cast<size_t>(region[1]), static_cast<size_t>(region[2])}};

  const size_t buffer_row_pitch = BUFFER_WIDTH * sizeof(cl_int);
  const size_t buffer_slice_pitch = BUFFER_HEIGHT * buffer_row_pitch;
  const size_t host_row_pitch = region[0] * sizeof(cl_int);
  const size_t host_slice_pitch = region[1] * host_row_pitch;

  assert(static_cast<size_t>(buffer_origin[2]) * buffer_slice_pitch +
             static_cast<size_t>(buffer_origin[1]) * buffer_row_pitch +
             static_cast<size_t>(buffer_origin[0]) +
             static_cast<size_t>(region[0]) * static_cast<size_t>(region[1]) *
                 static_cast<size_t>(region[2]) * sizeof(cl_int) <=
         BUFFER_SIZE * sizeof(cl_int));

  std::vector<cl_int> *host_buffer_ptr = new std::vector<cl_int>(BUFFER_SIZE);

  // locks the context_t's mutex
  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.buffer_origins.emplace_back(buffer_origin_ptr);
  fc.host_origins.emplace_back(host_origin_ptr);
  fc.regions.emplace_back(region_ptr);

  fc.host_buffers.emplace_back(host_buffer_ptr);

  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueWriteBufferRect(
      fc.queue, buffer->m, blocking_write, buffer_origin_ptr->data(),
      host_origin_ptr->data(), region_ptr->data(), buffer_row_pitch,
      buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      host_buffer_ptr->data(), num_events_in_wait_list, event_wait_list,
      &event));

  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueWriteBufferRect" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_write_buffer_rect(
        buffer->id, blocking_write, *buffer_origin_ptr, *host_origin_ptr,
        *region_ptr, buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
        host_slice_pitch, input.callback_id);
  }
}

void fuzzcl::enqueueFillBuffer(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  std::vector<cl_int> *pattern_ptr = new std::vector<cl_int>{};
  // pattern's size can only be a power of two
  const size_t pattern_size =
      std::pow(2, input.next(0, MAX_FILL_PATTERN_SIZE)) * sizeof(cl_int);
  for (size_t i = 0; i < pattern_size / sizeof(cl_int); i++) {
    pattern_ptr->push_back(input.next());
  }

  // offset and size need to be a multiple of pattern_size
  const size_t offset =
      input.next(0, BUFFER_SIZE / (2 * pattern_size)) * pattern_size;
  const size_t size =
      input.next(1, (BUFFER_SIZE - offset) / pattern_size) * pattern_size;

  // locks the context_t's mutex
  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.patterns.emplace_back(pattern_ptr);

  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueFillBuffer(
      fc.queue, buffer->m, pattern_ptr->data(), pattern_size, offset, size,
      num_events_in_wait_list, event_wait_list, &event));
  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueFillBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_fill_buffer(buffer->id, *pattern_ptr, pattern_size, offset,
                            size, input.callback_id);
  }
}

void fuzzcl::enqueueCopyBuffer(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *src_buffer = fuzzcl::get_buffer(fc, input);
  mem_object_t *dst_buffer = fuzzcl::get_buffer(fc, input, src_buffer->id);

  const size_t src_offset = input.next(0, BUFFER_SIZE / 2);
  const size_t dst_offset = input.next(0, src_offset);
  const size_t size = input.next(1, BUFFER_SIZE - src_offset);

  std::unique_lock<std::mutex> lock(fc.mutex);
  // If previous calls were made on these buffers, wait for them
  cl_uint num_events_in_wait_list = 0;
  std::vector<cl_event> *event_wait_list_ptr = new std::vector<cl_event>();
  if (src_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(src_buffer->event_stack.top());
  }
  if (dst_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(dst_buffer->event_stack.top());
  }
  fc.event_wait_lists.emplace_back(event_wait_list_ptr);
  cl_event event;

  IS_CL_SUCCESS(clEnqueueCopyBuffer(
      fc.queue, src_buffer->m, dst_buffer->m, src_offset * sizeof(cl_int),
      dst_offset * sizeof(cl_int), size * sizeof(cl_int),
      num_events_in_wait_list, event_wait_list_ptr->data(), &event));

  clRetainEvent(event);
  src_buffer->event_stack.push(event);
  dst_buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueCopyBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_copy_buffer(src_buffer->id, dst_buffer->id, src_offset,
                            dst_offset, size, input.callback_id);
  }
}

void fuzzcl::enqueueCopyBufferRect(fuzzcl::context_t &fc,
                                   fuzzcl::input_t &input) {
  mem_object_t *src_buffer = fuzzcl::get_buffer(fc, input);
  mem_object_t *dst_buffer = fuzzcl::get_buffer(fc, input, src_buffer->id);

  // in number of values
  const std::array<int, 3> src_origin = std::array<int, 3>{
      {input.next(0, BUFFER_WIDTH / 2), input.next(0, BUFFER_HEIGHT / 2),
       input.next(0, BUFFER_DEPTH / 2)}};
  const std::array<int, 3> dst_origin = std::array<int, 3>{
      {input.next(0, src_origin[0]), input.next(0, src_origin[1]),
       input.next(0, src_origin[2])}};
  const std::array<int, 3> region =
      std::array<int, 3>{{input.next(1, BUFFER_WIDTH - src_origin[0]),
                          input.next(1, BUFFER_HEIGHT - src_origin[1]),
                          input.next(1, BUFFER_DEPTH - src_origin[2])}};

  // in bytes
  std::array<size_t, 3> *src_origin_ptr = new std::array<size_t, 3>{
      {src_origin[0] * sizeof(cl_int), static_cast<size_t>(src_origin[1]),
       static_cast<size_t>(src_origin[2])}};
  std::array<size_t, 3> *dst_origin_ptr = new std::array<size_t, 3>{
      {dst_origin[0] * sizeof(cl_int), static_cast<size_t>(dst_origin[1]),
       static_cast<size_t>(dst_origin[2])}};
  std::array<size_t, 3> *region_ptr = new std::array<size_t, 3>{
      {region[0] * sizeof(cl_int), static_cast<size_t>(region[1]),
       static_cast<size_t>(region[2])}};

  const size_t src_row_pitch = BUFFER_WIDTH * sizeof(cl_int);
  const size_t src_slice_pitch = BUFFER_HEIGHT * src_row_pitch;
  const size_t dst_row_pitch = src_row_pitch;
  const size_t dst_slice_pitch = src_slice_pitch;

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.src_origins.emplace_back(src_origin_ptr);
  fc.dst_origins.emplace_back(dst_origin_ptr);
  fc.regions.emplace_back(region_ptr);

  // If previous calls were made on these buffers, wait for them
  cl_uint num_events_in_wait_list = 0;
  std::vector<cl_event> *event_wait_list_ptr = new std::vector<cl_event>();
  if (src_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(src_buffer->event_stack.top());
  }
  if (dst_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(dst_buffer->event_stack.top());
  }
  fc.event_wait_lists.emplace_back(event_wait_list_ptr);
  cl_event event;

  IS_CL_SUCCESS(clEnqueueCopyBufferRect(
      fc.queue, src_buffer->m, dst_buffer->m, src_origin_ptr->data(),
      dst_origin_ptr->data(), region_ptr->data(), src_row_pitch,
      src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list,
      event_wait_list_ptr->data(), &event));

  clRetainEvent(event);
  src_buffer->event_stack.push(event);
  dst_buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueCopyBufferRect" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_copy_buffer_rect(src_buffer->id, dst_buffer->id,
                                 *src_origin_ptr, *dst_origin_ptr, *region_ptr,
                                 src_row_pitch, src_slice_pitch, dst_row_pitch,
                                 dst_slice_pitch, input.callback_id);
  }
}

void fuzzcl::enqueueMapBuffer(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *buffer = fuzzcl::get_buffer(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_map = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_map));

  const cl_map_flags map_flag = input.next(0, 1) ? CL_MAP_READ : CL_MAP_WRITE;

  const size_t offset = input.next(0, BUFFER_SIZE / 2);
  const size_t size = input.next(1, BUFFER_SIZE - offset);

  std::unique_lock<std::mutex> lock(fc.mutex);
  // If a previous call was made on this buffer, wait for its event
  const cl_uint num_events_in_wait_list =
      buffer->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
  cl_event event;

  cl_int error_code;
  void *map_ptr = clEnqueueMapBuffer(
      fc.queue, buffer->m, blocking_map, map_flag, offset, size,
      num_events_in_wait_list, event_wait_list, &event, &error_code);
  IS_CL_SUCCESS(error_code);

  fc.map_ptrs.emplace_back(map_ptr_t{buffer, map_ptr});

  buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueMapBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_map_buffer(buffer->id, blocking_map, map_flag, offset, size,
                           input.callback_id);
  }
}

void fuzzcl::enqueueReadImage(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *image = fuzzcl::get_image(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_read = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_read));

  // in pixels
  std::array<size_t, 3> *image_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_origin_ptr->at(1))),
       1}};

  // in bytes, each pixel is composed of INT_PER_PIXEL cl_ints
  const size_t row_pitch = IMAGE_WIDTH * INT_PER_PIXEL * sizeof(cl_int);
  const size_t slice_pitch = 0;

  // the size of the image is defined so that it fits in a buffer
  std::vector<cl_int4> *image_host_buffer_ptr =
      new std::vector<cl_int4>(BUFFER_SIZE);

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_origins.emplace_back(image_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  fc.image_host_buffers.emplace_back(image_host_buffer_ptr);

  // if a previous call was made on this image, wait for its event
  const cl_uint num_events_in_wait_list = image->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(
      clEnqueueReadImage(fc.queue, image->m, blocking_read,
                         image_origin_ptr->data(), image_region_ptr->data(),
                         row_pitch, slice_pitch, image_host_buffer_ptr->data(),
                         num_events_in_wait_list, event_wait_list, &event));

  image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueReadImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_read_image(image->id, blocking_read, *image_origin_ptr,
                           *image_region_ptr, row_pitch, slice_pitch,
                           input.callback_id);
  }
}

void fuzzcl::enqueueWriteImage(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *image = fuzzcl::get_image(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_write = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_write));

  // in pixels
  std::array<size_t, 3> *image_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_origin_ptr->at(1))),
       1}};

  // in bytes, each pixel is composed of INT_PER_PIXEL cl_ints
  const size_t row_pitch = IMAGE_WIDTH * INT_PER_PIXEL * sizeof(cl_int);
  const size_t slice_pitch = 0;

  // the size of the image is defined so that it fits in a buffer
  std::vector<cl_int4> *image_host_buffer_ptr =
      new std::vector<cl_int4>(BUFFER_SIZE);

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_origins.emplace_back(image_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  fc.image_host_buffers.emplace_back(image_host_buffer_ptr);

  // if a previous call was made on this image, wait for its event
  const cl_uint num_events_in_wait_list = image->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(
      clEnqueueWriteImage(fc.queue, image->m, blocking_write,
                          image_origin_ptr->data(), image_region_ptr->data(),
                          row_pitch, slice_pitch, image_host_buffer_ptr->data(),
                          num_events_in_wait_list, event_wait_list, &event));

  image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueWriteImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_write_image(image->id, blocking_write, *image_origin_ptr,
                            *image_region_ptr, row_pitch, slice_pitch,
                            input.callback_id);
  }
}

void fuzzcl::enqueueFillImage(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *image = fuzzcl::get_image(fc, input);

  std::array<cl_int, 4> *image_fill_color_ptr = new std::array<cl_int, 4>{
      {input.next(), input.next(), input.next(), input.next()}};

  // in pixels
  std::array<size_t, 3> *image_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_origin_ptr->at(1))),
       1}};

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_fill_colors.emplace_back(image_fill_color_ptr);

  fc.image_origins.emplace_back(image_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  // if a previous call was made on this image, wait for its event
  const cl_uint num_events_in_wait_list = image->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(
      clEnqueueFillImage(fc.queue, image->m, image_fill_color_ptr->data(),
                         image_origin_ptr->data(), image_region_ptr->data(),
                         num_events_in_wait_list, event_wait_list, &event));

  image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueFillImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_fill_image(image->id, *image_fill_color_ptr, *image_origin_ptr,
                           *image_region_ptr, input.callback_id);
  }
}

void fuzzcl::enqueueCopyImage(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *src_image = fuzzcl::get_image(fc, input);
  mem_object_t *dst_image = fuzzcl::get_image(fc, input, src_image->id);

  // in pixels
  std::array<size_t, 3> *image_src_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_dst_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, image_src_origin_ptr->at(0))),
       static_cast<size_t>(input.next(0, image_src_origin_ptr->at(1))), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_src_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_src_origin_ptr->at(1))),
       1}};

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_src_origins.emplace_back(image_src_origin_ptr);
  fc.image_dst_origins.emplace_back(image_dst_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  // If previous calls were made on these images, wait for their events
  cl_uint num_events_in_wait_list = 0;
  std::vector<cl_event> *event_wait_list_ptr = new std::vector<cl_event>();
  if (src_image->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(src_image->event_stack.top());
  }
  if (dst_image->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(dst_image->event_stack.top());
  }
  fc.event_wait_lists.emplace_back(event_wait_list_ptr);
  cl_event event;

  IS_CL_SUCCESS(clEnqueueCopyImage(
      fc.queue, src_image->m, dst_image->m, image_src_origin_ptr->data(),
      image_dst_origin_ptr->data(), image_region_ptr->data(),
      num_events_in_wait_list, event_wait_list_ptr->data(), &event));

  clRetainEvent(event);
  src_image->event_stack.push(event);
  dst_image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueCopyImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_copy_image(src_image->id, dst_image->id, *image_src_origin_ptr,
                           *image_dst_origin_ptr, *image_region_ptr,
                           input.callback_id);
  }
}

void fuzzcl::enqueueCopyImageToBuffer(context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *src_image = fuzzcl::get_image(fc, input);
  mem_object_t *dst_buffer = fuzzcl::get_buffer(fc, input);

  // in pixels
  std::array<size_t, 3> *image_src_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_src_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_src_origin_ptr->at(1))),
       1}};

  // in bytes
  const size_t dst_offset =
      input.next(0, BUFFER_SIZE - (image_src_origin_ptr->at(0) *
                                   image_src_origin_ptr->at(1))) *
      sizeof(cl_int);

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_src_origins.emplace_back(image_src_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  // If previous calls were made on these images, wait for their events
  cl_uint num_events_in_wait_list = 0;
  std::vector<cl_event> *event_wait_list_ptr = new std::vector<cl_event>();
  if (src_image->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(src_image->event_stack.top());
  }
  if (dst_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(dst_buffer->event_stack.top());
  }
  fc.event_wait_lists.emplace_back(event_wait_list_ptr);
  cl_event event;

  IS_CL_SUCCESS(clEnqueueCopyImageToBuffer(
      fc.queue, src_image->m, dst_buffer->m, image_src_origin_ptr->data(),
      image_region_ptr->data(), dst_offset, num_events_in_wait_list,
      event_wait_list_ptr->data(), &event));

  clRetainEvent(event);
  src_image->event_stack.push(event);
  dst_buffer->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueCopyImageToBuffer" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_copy_image_to_buffer(src_image->id, dst_buffer->id,
                                     *image_src_origin_ptr, *image_region_ptr,
                                     dst_offset, input.callback_id);
  }
}

void fuzzcl::enqueueCopyBufferToImage(fuzzcl::context_t &fc,
                                      fuzzcl::input_t &input) {
  mem_object_t *src_buffer = fuzzcl::get_buffer(fc, input);
  mem_object_t *dst_image = fuzzcl::get_image(fc, input);

  // in pixels
  std::array<size_t, 3> *image_dst_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_dst_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_dst_origin_ptr->at(1))),
       1}};

  // in bytes
  const size_t src_offset =
      input.next(0, BUFFER_SIZE - (image_dst_origin_ptr->at(0) *
                                   image_dst_origin_ptr->at(1))) *
      sizeof(cl_int);

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_dst_origins.emplace_back(image_dst_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  // If previous calls were made on these images, wait for their events
  cl_uint num_events_in_wait_list = 0;
  std::vector<cl_event> *event_wait_list_ptr = new std::vector<cl_event>();
  if (src_buffer->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(src_buffer->event_stack.top());
  }
  if (dst_image->event_stack.size() > 0) {
    num_events_in_wait_list++;
    event_wait_list_ptr->push_back(dst_image->event_stack.top());
  }
  fc.event_wait_lists.emplace_back(event_wait_list_ptr);
  cl_event event;

  IS_CL_SUCCESS(clEnqueueCopyBufferToImage(
      fc.queue, src_buffer->m, dst_image->m, src_offset,
      image_dst_origin_ptr->data(), image_region_ptr->data(),
      num_events_in_wait_list, event_wait_list_ptr->data(), &event));

  clRetainEvent(event);
  src_buffer->event_stack.push(event);
  dst_image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueCopyBufferToImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_copy_buffer_to_image(src_buffer->id, dst_image->id, src_offset,
                                     *image_dst_origin_ptr, *image_region_ptr,
                                     input.callback_id);
  }
}

void fuzzcl::enqueueMapImage(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  mem_object_t *image = fuzzcl::get_image(fc, input);

  // no blocking calls should be made in a callback
  const cl_bool blocking_map = input.next(0, 1) && !fc.enable_callbacks;
  assert(!(fc.enable_callbacks && blocking_map));

  const cl_map_flags map_flags = input.next(0, 1) ? CL_MAP_READ : CL_MAP_WRITE;

  // in pixels
  std::array<size_t, 3> *image_origin_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(input.next(0, IMAGE_WIDTH / 2)),
       static_cast<size_t>(input.next(0, IMAGE_HEIGHT / 2)), 0}};
  std::array<size_t, 3> *image_region_ptr = new std::array<size_t, 3>{
      {static_cast<size_t>(
           input.next(1, IMAGE_WIDTH - image_origin_ptr->at(0))),
       static_cast<size_t>(
           input.next(1, IMAGE_HEIGHT - image_origin_ptr->at(1))),
       1}};

  std::unique_lock<std::mutex> lock(fc.mutex);
  fc.image_origins.emplace_back(image_origin_ptr);
  fc.image_regions.emplace_back(image_region_ptr);

  // If a previous call was made on this image, wait for its event
  const cl_uint num_events_in_wait_list = image->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
  cl_event event;

  cl_int errcode;
  size_t image_row_pitch;
  void *map_ptr = clEnqueueMapImage(
      fc.queue, image->m, blocking_map, map_flags, image_origin_ptr->data(),
      image_region_ptr->data(), &image_row_pitch, NULL, num_events_in_wait_list,
      event_wait_list, &event, &errcode);
  IS_CL_SUCCESS(errcode);

  fc.map_ptrs.emplace_back(map_ptr_t{image, map_ptr, image_row_pitch});

  image->event_stack.push(event);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueMapImage" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_map_image(image->id, blocking_map, map_flags, *image_origin_ptr,
                          *image_region_ptr, input.callback_id);
  }
}

void fuzzcl::enqueueUnmapMemObject(fuzzcl::context_t &fc,
                                   fuzzcl::input_t &input) {
  std::unique_lock<std::mutex> lock(fc.mutex);
  if (fc.map_ptrs.empty()) {
    if (fc.verbose) {
      const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
      std::cout << "There are not map_ptr to unmap\n";
    }
    return;
  }

  const size_t map_ptr_index = input.next(0, fc.map_ptrs.size() - 1);
  const map_ptr_t map_ptr = fc.map_ptrs[map_ptr_index];

  // If a previous call was made on this mem_obj, wait for its event
  const cl_uint num_events_in_wait_list =
      map_ptr.mem_obj->event_stack.size() > 0 ? 1 : 0;
  const cl_event *event_wait_list =
      num_events_in_wait_list == 1 ? &map_ptr.mem_obj->event_stack.top() : NULL;
  cl_event event;

  IS_CL_SUCCESS(clEnqueueUnmapMemObject(fc.queue, map_ptr.mem_obj->m, map_ptr.p,
                                        num_events_in_wait_list,
                                        event_wait_list, &event));

  map_ptr.mem_obj->event_stack.push(event);

  // remove the map_ptr once unmapped
  fc.map_ptrs.erase(fc.map_ptrs.begin() + map_ptr_index);
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueUnmapMemObject" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_unmap_mem_object(map_ptr_index, input.callback_id);
  }
}

void fuzzcl::enqueueNDRangeKernel(fuzzcl::context_t &fc,
                                  fuzzcl::input_t &input) {
  std::unique_lock<std::mutex> lock(fc.mutex);
  IS_CL_SUCCESS(clEnqueueNDRangeKernel(
      fc.queue, fc.kernel, fc.work_dim, &fc.global_work_offset,
      &fc.global_work_size, nullptr, 0, NULL, NULL));
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueNDRangeKernel" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_nd_range_kernel(input.callback_id);
  }
}

void fuzzcl::enqueueTask(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  std::unique_lock<std::mutex> lock(fc.mutex);
  IS_CL_SUCCESS(clEnqueueTask(fc.queue, fc.kernel, 0, NULL, NULL));
  lock.unlock();

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clEnqueueTask" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_task(input.callback_id);
  }
}

/// @brief The callback used by fuzzcl::setEventCallback
static void CL_CALLBACK callback(cl_event, cl_int, void *user_data) {
  const fuzzcl::callback_input_data_t callback_input_data =
      *static_cast<fuzzcl::callback_input_data_t *>(user_data);
  run_input(*callback_input_data.fc, callback_input_data.input);
}

void fuzzcl::setEventCallback(fuzzcl::context_t &fc, fuzzcl::input_t &input) {
  // get a mem_object_t to pull an event from
  std::unique_lock<std::mutex> lock(fc.mutex);
  fuzzcl::mem_object_t *mem_obj;
  const cl_bool buffer_or_image = input.next(0, 1);
  if (buffer_or_image) {
    mem_obj = fuzzcl::get_image(fc, input);
  } else {
    mem_obj = fuzzcl::get_buffer(fc, input);
  }

  if (mem_obj->event_stack.empty()) {
    if (fc.verbose) {
      const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
      std::cout << "There is no event to attach a callback to\n";
    }
    return;
  }

  // take the top event
  cl_event event = mem_obj->event_stack.top();
  lock.unlock();

  cl_int command_exec_callback_type;
  switch (input.next(0, 2)) {
    case 0:
      command_exec_callback_type = CL_SUBMITTED;
      break;
    case 1:
      command_exec_callback_type = CL_RUNNING;
      break;
    case 2:
      command_exec_callback_type = CL_COMPLETE;
      break;
    default:
      // not necessary since input.next(0, 2) returns either 0, 1 or 2
      command_exec_callback_type = CL_COMPLETE;
      break;
  }

  // get a part of the input data to be used in the callback
  if (input.data.size() - input.index < 1) {
    if (fc.verbose) {
      const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
      std::cout << "There is not enough data to create a callback\n";
    }
    return;
  }

  const size_t callback_data_size =
      input.next(0, std::min(MAX_CALLBACK_INPUT_SIZE,
                             int(input.data.size() - input.index - 1)));
  std::vector<uint8_t> callback_data;
  callback_data.reserve(callback_data_size);
  for (size_t i = 0; i < callback_data_size; i++) {
    callback_data.emplace_back(input.next());
  }

  size_t callback_id = input.last_callback_id++;
  const fuzzcl::input_t callback_input =
      fuzzcl::input_t(callback_data, input.should_export, callback_id);

  fuzzcl::callback_input_data_t *callback_input_data_ptr =
      new fuzzcl::callback_input_data_t{&fc, callback_input};
  lock.lock();
  fc.callback_input_datas.emplace_back(callback_input_data_ptr);
  lock.unlock();

  IS_CL_SUCCESS(clSetEventCallback(event, command_exec_callback_type, callback,
                                   callback_input_data_ptr));

  // print to stdout
  if (fc.verbose) {
    const std::lock_guard<std::mutex> output_lock(fc.output_mutex);
    std::cout << "clSetEventCallback" << '\n';
  }

  // generate a UnitCL test
  if (input.should_export) {
    fc.cgen.gen_set_event_callback(buffer_or_image, mem_obj->id, callback_id,
                                   command_exec_callback_type);
  }
}

fuzzcl::mem_object_t *fuzzcl::get_buffer(fuzzcl::context_t &fc,
                                         fuzzcl::input_t &input) {
  return fc.buffers[input.next(0, fc.buffers.size() - 1)].get();
}

fuzzcl::mem_object_t *fuzzcl::get_buffer(fuzzcl::context_t &fc,
                                         fuzzcl::input_t &input,
                                         size_t buffer_id) {
  size_t index = input.next(0, fc.buffers.size() - 2);
  if (index == buffer_id) {
    index++;
  }
  return fc.buffers[index].get();
}

fuzzcl::mem_object_t *fuzzcl::get_image(fuzzcl::context_t &fc,
                                        fuzzcl::input_t &input) {
  return fc.images[input.next(0, fc.images.size() - 1)].get();
}

fuzzcl::mem_object_t *fuzzcl::get_image(fuzzcl::context_t &fc,
                                        fuzzcl::input_t &input,
                                        size_t image_id) {
  size_t index = input.next(0, fc.images.size() - 2);
  if (index == image_id) {
    index++;
  }
  return fc.images[index].get();
}

cl_platform_id fuzzcl::context_t::select_platform() {
  cl_platform_id platform;
  IS_CL_SUCCESS(clGetPlatformIDs(1, &platform, NULL));
  return platform;
}

cl_device_id fuzzcl::context_t::select_device(
    cl_platform_id platform, const std::string &specified_device) {
  cl_uint num_devices;
  IS_CL_SUCCESS(
      clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices));

  if (0 == num_devices) {
    std::cerr << "No OpenCL devices found\n";
    exit(1);
  }

  // Get device IDs
  std::vector<cl_device_id> devices(num_devices);
  IS_CL_SUCCESS(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices,
                               devices.data(), NULL));
  device_list = devices;

  // Get device names
  std::vector<std::string> device_names;
  for (size_t i = 0; i < devices.size(); i++) {
    size_t device_name_size;
    IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL,
                                  &device_name_size));
    if (device_name_size > 0) {
      std::string device_name = std::string(device_name_size - 1, '\0');
      IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                    device_name_size, device_name.data(),
                                    NULL));
      device_names.push_back(device_name);
    } else {
      device_names.push_back("Nameless device");
    }
  }

  if (specified_device.empty()) {
    if (num_devices > 1) {
      std::cerr
          << "Multiple OpenCL devices were found but no device was specified\n";
      std::cerr << "Available devices : \n";
      for (size_t i = 0; i < device_names.size(); i++) {
        std::cerr << "  " << i << ": " << device_names[i] << '\n';
      }
      exit(1);
    } else {
      return devices[0];
    }
  } else {
    // Find the specified device
    bool found = false;
    size_t i = 0;
    while (!found && i < device_names.size()) {
      if (device_names[i] == specified_device) {
        found = true;
        continue;
      }
      i++;
    }

    // Device not found
    if (i == device_names.size()) {
      std::cerr << "No device named " << specified_device << " was found\n";
      std::cerr << "Available devices : \n";
      for (size_t i = 0; i < device_names.size(); i++) {
        std::cerr << "  " << i << ": " << device_names[i] << '\n';
      }
      exit(1);
    }

    return devices[i];
  }
}
