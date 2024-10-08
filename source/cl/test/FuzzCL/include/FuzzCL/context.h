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

#ifndef FUZZCL_CONTEXT_H_INCLUDED
#define FUZZCL_CONTEXT_H_INCLUDED

#include <CL/cl.h>
#include <cargo/optional.h>

#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <thread>
#include <vector>

#include "FuzzCL/error.h"

const size_t BUFFER_WIDTH = 16;
const size_t BUFFER_HEIGHT = 16;
const size_t BUFFER_DEPTH = 16;
const size_t BUFFER_SIZE = BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_DEPTH;

const size_t INT_PER_PIXEL = 4;
const size_t IMAGE_WIDTH = BUFFER_WIDTH / INT_PER_PIXEL;
const size_t IMAGE_HEIGHT = BUFFER_HEIGHT;

#define STR_EXPAND(...) #__VA_ARGS__
#define STR(s) STR_EXPAND(s)

#define WORK_DIM 1
#define GLOBAL_WORK_OFFSET 0
#define GLOBAL_WORK_SIZE 1

#define MAX_NUM_THREADS 2
#define MAX_NUM_BUFFERS 2
#define MAX_FILL_PATTERN_SIZE 5
#define MAX_NUM_IMAGES 2

#define MAX_CALLBACK_INPUT_SIZE 10

#define IS_CL_SUCCESS(X)                                                     \
  {                                                                          \
    const cl_int ret_val = X;                                                \
    if (CL_SUCCESS != ret_val) {                                             \
      std::cerr << "OpenCL error occured: " << #X << " returned " << ret_val \
                << " : " << fuzzcl::cl_error_code_to_name_map.at(ret_val)    \
                << '\n';                                                     \
      exit(1);                                                               \
    }                                                                        \
  }

#include "FuzzCL/generator.h"

namespace fuzzcl {

/// @brief Store fuzzing options
struct options_t {
  bool enable_callbacks;
  bool verbose;
  std::string device;
  std::string output;

  /// @brief Constructor
  ///
  /// @param enable_callbacks Should callbacks be enabled
  /// @param verbose Should the program output to stdout
  /// @param device A prefered OpenCL device to run the fuzzing on
  /// @param output Path to generated a UnitCL test
  options_t(bool enable_callbacks = false, bool verbose = false,
            std::string device = "", std::string output = "")
      : enable_callbacks(enable_callbacks),
        verbose(verbose),
        device(device),
        output(output) {}
};

/// @brief Type for handling thread specific input
struct input_t {
  std::vector<uint8_t> data;
  size_t index = 0;

  const bool should_export;

  const cargo::optional<size_t> callback_id = cargo::nullopt;

  // stores the last callback id generated from this input
  size_t last_callback_id = 0;

  /// @brief Constructor
  ///
  /// @param data Array containing the input data
  /// @param size Size of the data array
  /// @param should_export Should this input be converted to UnitCL test
  /// @param callback_id The id of the callback
  input_t(const uint8_t *data, size_t size, bool should_export = false,
          cargo::optional<size_t> callback_id = cargo::nullopt)
      : data(data, data + size),
        should_export(should_export),
        callback_id(callback_id) {}

  /// @brief Constructor
  ///
  /// @param data Array containing the input data
  /// @param should_export Should this input be converted to UnitCL test
  /// @param callback_id The id of the callback
  input_t(std::vector<uint8_t> data, bool should_export = false,
          cargo::optional<size_t> callback_id = cargo::nullopt)
      : data(data), should_export(should_export), callback_id(callback_id) {}

  /// @brief Map next input value between [min,max]
  ///
  /// @param[in] min Minimum value that can be returned
  /// @param[in] max Maximum value that can be returned
  ///
  /// @return Returns the next input value, mapped between [min,max]
  int next(int min, int max) {
    assert(min <= max);
    // Return min if there is not enough data left
    if (index < data.size()) {
      return min + (data[index++] % (max - min + 1));
    } else {
      return min;
    }
  }

  /// @brief Get next input value
  ///
  /// @return Returns the next input value
  int next() {
    if (index < data.size()) {
      return data[index++];
    } else {
      return 0;
    }
  }
};

/// @brief Declaration of context_t defined below
struct context_t;
/// @brief Type for handling callback input data
struct callback_input_data_t {
  fuzzcl::context_t *fc;
  fuzzcl::input_t input;
};

/// @brief FuzzCL wrapper of an OpenCL mem object
struct mem_object_t {
  size_t id;
  cl_mem m;
  std::stack<cl_event> event_stack;
};

/// @brief FuzzCL wrapper of an OpenCL mapped ptr
struct map_ptr_t {
  mem_object_t *mem_obj;
  void *p;
  size_t image_row_pitch;

  /// @brief Constructor
  ///
  /// @param mem_obj A mem_object_t assotiated to the map ptr
  /// @param ptr OpenCL map ptr
  /// @param image_row_pitch Row pitch returned by clEnqueueMapImage
  map_ptr_t(mem_object_t *mem_obj, void *ptr, size_t image_row_pitch = 0)
      : mem_obj(mem_obj), p(ptr), image_row_pitch(image_row_pitch) {}
};

/// @brief RAII type for handling the fuzzing
struct context_t {
  std::mutex mutex;
  std::mutex output_mutex;

  const bool verbose;
  const bool enable_callbacks;

  code_generator_t cgen;

  cl_platform_id platform;
  cl_device_id device;
  std::vector<cl_device_id> device_list;

  cl_context context;
  cl_command_queue queue;

  std::vector<std::unique_ptr<mem_object_t>> buffers;
  std::vector<std::unique_ptr<std::vector<cl_int>>> host_buffers;

  std::vector<std::unique_ptr<std::array<size_t, 3>>> buffer_origins;
  std::vector<std::unique_ptr<std::array<size_t, 3>>> host_origins;
  std::vector<std::unique_ptr<std::array<size_t, 3>>> regions;

  std::vector<std::unique_ptr<std::vector<cl_int>>> patterns;

  std::vector<std::unique_ptr<std::array<size_t, 3>>> src_origins;
  std::vector<std::unique_ptr<std::array<size_t, 3>>> dst_origins;

  const cl_image_format image_format = {CL_RGBA, CL_SIGNED_INT32};
  const cl_image_desc image_desc = []() {
    cl_image_desc image_desc;
    image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    image_desc.image_width = IMAGE_WIDTH;
    image_desc.image_height = IMAGE_HEIGHT;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 0;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;
    return image_desc;
  }();

  std::vector<std::unique_ptr<mem_object_t>> images;
  std::vector<std::unique_ptr<std::vector<cl_int4>>> image_host_buffers;

  std::vector<std::unique_ptr<std::array<size_t, 3>>> image_origins;
  std::vector<std::unique_ptr<std::array<size_t, 3>>> image_regions;

  std::vector<std::unique_ptr<std::array<cl_int, 4>>> image_fill_colors;

  std::vector<std::unique_ptr<std::array<size_t, 3>>> image_src_origins;
  std::vector<std::unique_ptr<std::array<size_t, 3>>> image_dst_origins;

  std::vector<map_ptr_t> map_ptrs;

  cl_program program;
  cl_kernel kernel;
  const cl_uint work_dim = WORK_DIM;
  const size_t global_work_offset = GLOBAL_WORK_OFFSET;
  const size_t global_work_size = GLOBAL_WORK_SIZE;

  std::vector<std::unique_ptr<std::vector<cl_event>>> event_wait_lists;

  std::vector<std::unique_ptr<fuzzcl::callback_input_data_t>>
      callback_input_datas;

  /// @brief Constructor
  ///
  /// @param kernel_binaries An array of kernel binaries
  /// @param kernel_binary_sizes A vector containing kernel binary sizes
  /// @param options Fuzzing options
  context_t(const unsigned char **kernel_binaries,
            const std::vector<size_t> &kernel_binary_sizes,
            fuzzcl::options_t options)
      : verbose(options.verbose),
        enable_callbacks(options.enable_callbacks),
        cgen(options.output) {
    platform = select_platform();
    device = select_device(platform, options.device);

    // Create context
    cl_int errcode;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &errcode);
    IS_CL_SUCCESS(errcode);

    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &errcode);
    IS_CL_SUCCESS(errcode);

    // if offline compilation is disabled
    if (kernel_binary_sizes.empty()) {
      const char *source = "void kernel foo() {}";
      program =
          clCreateProgramWithSource(context, 1, &source, nullptr, &errcode);
      IS_CL_SUCCESS(errcode);
    } else {
      cl_int binary_status;
      program = clCreateProgramWithBinary(
          context, device_list.size(), device_list.data(),
          kernel_binary_sizes.data(), kernel_binaries, &binary_status,
          &errcode);
      IS_CL_SUCCESS(binary_status);
      IS_CL_SUCCESS(errcode);
    }

    IS_CL_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &errcode);
    IS_CL_SUCCESS(errcode);

    // Create every buffers
    for (size_t i = 0; i < MAX_NUM_BUFFERS; i++) {
      cl_int errcode;
      cl_mem mem_obj =
          clCreateBuffer(context, CL_MEM_READ_WRITE,
                         sizeof(cl_int) * BUFFER_SIZE, NULL, &errcode);
      IS_CL_SUCCESS(errcode);

      buffers.emplace_back(
          new mem_object_t{buffers.size(), mem_obj, std::stack<cl_event>()});
    }
    // Create every images
    for (size_t i = 0; i < MAX_NUM_IMAGES; i++) {
      cl_int errcode;
      cl_mem mem_obj = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                                     &image_desc, NULL, &errcode);
      IS_CL_SUCCESS(errcode);

      images.emplace_back(
          new mem_object_t{images.size(), mem_obj, std::stack<cl_event>()});
    }
  }

  /// @brief Destructor
  ~context_t() {
    // Make sure everything is done before we unmap. Without this it's possible
    // for event callbacks to enqueue operations that use a memory object at the
    // same time we're enqueueing the unmap operation.
    clFinish(queue);

    // Unmap previously mapped ptrs
    for (const map_ptr_t &map_ptr : map_ptrs) {
      const cl_uint num_events_in_wait_list =
          map_ptr.mem_obj->event_stack.size() > 0 ? 1 : 0;
      const cl_event *event_wait_list =
          num_events_in_wait_list == 1 ? &map_ptr.mem_obj->event_stack.top()
                                       : NULL;
      cl_event event;

      IS_CL_SUCCESS(clEnqueueUnmapMemObject(queue, map_ptr.mem_obj->m,
                                            map_ptr.p, num_events_in_wait_list,
                                            event_wait_list, &event));

      map_ptr.mem_obj->event_stack.push(event);
    }

    clFinish(queue);
    IS_CL_SUCCESS(clReleaseCommandQueue(queue));

    // Release buffers and events related to them
    for (size_t i = 0; i < buffers.size(); i++) {
      IS_CL_SUCCESS(clReleaseMemObject(buffers[i]->m));
      while (!buffers[i]->event_stack.empty()) {
        IS_CL_SUCCESS(clReleaseEvent(buffers[i]->event_stack.top()));
        buffers[i]->event_stack.pop();
      }
    }

    // Release images and events related to them
    for (size_t i = 0; i < images.size(); i++) {
      IS_CL_SUCCESS(clReleaseMemObject(images[i]->m));
      while (!images[i]->event_stack.empty()) {
        IS_CL_SUCCESS(clReleaseEvent(images[i]->event_stack.top()));
        images[i]->event_stack.pop();
      }
    }

    IS_CL_SUCCESS(clReleaseKernel(kernel));
    IS_CL_SUCCESS(clReleaseProgram(program));

    IS_CL_SUCCESS(clReleaseContext(context));
  }

  /// @brief Select an OpenCL platform
  ///
  /// @return Returns the selected platform
  cl_platform_id select_platform();

  /// @brief Select an OpenCL device
  ///
  /// @param[in] platform An OpenCL platform devices belong to
  /// @param[in] specified_device OpenCL device name from the command line
  ///
  /// @return Returns the selected device
  cl_device_id select_device(cl_platform_id platform,
                             const std::string &specified_device);
};

/// @brief Create a buffer_t object
///
/// @param[in, out] fc Fuzz context buffers belong to
///
/// @return Returns the created buffer
mem_object_t *create_buffer(context_t &fc);

/// @brief Get or Create a buffer_t object
///
/// @param[in, out] fc Fuzz context buffers belong to
/// @param[in, out] input Input data
///
/// @return Returns the selected or created buffer
mem_object_t *get_buffer(context_t &fc, input_t &input);

/// @brief Get or Create a buffer_t object
///
/// @param[in, out] fc Fuzz context buffers belong to
/// @param[in, out] input Input data
/// @param[in] buffer_id ID of the Buffer to exclude from the available choices
///
/// @return Returns the selected or created buffer
mem_object_t *get_buffer(context_t &fc, input_t &input, size_t buffer_id);

/// @brief Create an image_t object
///
/// @param fc Fuzz context images belong to
///
/// @return Returns the created image
mem_object_t *create_image(context_t &fc);

/// @brief Get or Create an image_t object
///
/// @param fc Fuzz context images belong to
/// @param[in, out] input Input data
///
/// @return Returns the selected or created image
mem_object_t *get_image(context_t &fc, input_t &input);

/// @brief Get or Create an image object
///
/// @param fc Fuzz context images belong to
/// @param[in, out] input Input data
/// @param image_id ID of the image to exclude from the available choices
///
/// @return Returns the selected or created image
mem_object_t *get_image(context_t &fc, input_t &input, size_t image_id);

/// @brief Decode and run an input
///
/// @param fc Fuzz context
/// @param input The input data
void run_input(context_t &fc, input_t input);

/// @brief Fuzz the OpenCL runtime
///
/// @param data Input data array
/// @param size Size of the input array
/// @param kernel_binaries A pointer to an array of kernel binaries
/// @param kernel_binary_sizes A vector containing the kernel binary sizes
/// @param arguments Fuzzing options
void fuzz_from_input(const uint8_t *data, size_t size,
                     const unsigned char **kernel_binaries,
                     const std::vector<size_t> &kernel_binary_sizes,
                     const fuzzcl::options_t arguments);

/// @brief Call clEnqueueReadBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueReadBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueWriteBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueWriteBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueReadBufferRect with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueReadBufferRect(context_t &fc, input_t &input);

/// @brief Call clEnqueueWriteBufferRect with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueWriteBufferRect(context_t &fc, input_t &input);

/// @brief Call clEnqueueFillBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueFillBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueCopyBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueCopyBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueCopyBufferRect with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueCopyBufferRect(context_t &fc, input_t &input);

/// @brief Call clEnqueueMapBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueMapBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueReadImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueReadImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueWriteImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueWriteImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueFillImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueFillImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueCopyImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueCopyImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueCopyImageToBuffer with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueCopyImageToBuffer(context_t &fc, input_t &input);

/// @brief Call clEnqueueCopyBufferToImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueCopyBufferToImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueMapImage with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueMapImage(context_t &fc, input_t &input);

/// @brief Call clEnqueueUnmapMemObject with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueUnmapMemObject(context_t &fc, input_t &input);

/// @brief Call clEnqueueNDRangeKernel with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueNDRangeKernel(context_t &fc, input_t &input);

/// @brief Call clEnqueueTask with random parameters
///
/// @param[in, out] fc Fuzz context
/// @param[in, out] input Input data
void enqueueTask(context_t &fc, input_t &input);

/// @brief Call clSetEventCallback with random parameters
///
/// @param fc[in, out] Fuzz context
/// @param input[in, out] Input data
void setEventCallback(context_t &fc, input_t &input);
}  // namespace fuzzcl
#endif
