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

#ifndef FUZZCL_GENERATOR_H_INCLUDED
#define FUZZCL_GENERATOR_H_INCLUDED

#include <fstream>

#include "FuzzCL/context.h"

namespace fuzzcl {
/// @brief Type containing test execution parameters
struct exec_params_t {
  std::vector<std::string> commands;

  std::vector<std::string> buffer_ids;

  std::vector<std::string> blockings;

  std::vector<std::string> offsets;
  std::vector<std::string> sizes;

  std::vector<std::string> buffer_origins;
  std::vector<std::string> host_origins;
  std::vector<std::string> regions;
  std::vector<std::string> buffer_row_pitchs;
  std::vector<std::string> buffer_slice_pitchs;
  std::vector<std::string> host_row_pitchs;
  std::vector<std::string> host_slice_pitchs;

  std::vector<std::string> patterns;
  std::vector<std::string> pattern_sizes;

  std::vector<std::string> src_buffer_ids;
  std::vector<std::string> dst_buffer_ids;
  std::vector<std::string> src_offsets;
  std::vector<std::string> dst_offsets;

  std::vector<std::string> src_origins;
  std::vector<std::string> dst_origins;
  std::vector<std::string> src_row_pitchs;
  std::vector<std::string> src_slice_pitchs;
  std::vector<std::string> dst_row_pitchs;
  std::vector<std::string> dst_slice_pitchs;

  std::vector<std::string> image_ids;
  std::vector<std::string> image_origins;
  std::vector<std::string> image_regions;
  std::vector<std::string> image_row_pitchs;
  std::vector<std::string> image_slice_pitchs;

  std::vector<std::string> image_fill_colors;

  std::vector<std::string> src_image_ids;
  std::vector<std::string> dst_image_ids;
  std::vector<std::string> image_src_origins;
  std::vector<std::string> image_dst_origins;

  std::vector<std::string> map_flags;
  std::vector<std::string> map_ptr_indexs;

  std::vector<std::string> buffer_or_images;
  std::vector<std::string> mem_obj_ids;
  std::vector<std::string> callback_ids;
  std::vector<std::string> command_exec_callback_types;
};

/// @brief Type for handling UnitCL code generation
struct code_generator_t {
  std::string path = "";
  std::string code = "";

  std::mutex mutex;

  /// @brief Constructor
  ///
  /// @param[in] export_path Path to export the UnitCL test to
  code_generator_t(std::string export_path = "") : path(export_path) {}

  /// @brief Destructor, saves the code to file
  ~code_generator_t() {
    if (!path.empty()) {
      gen_unitcl_context();
      gen_test();

      code += "}";

      std::ofstream fs;
      fs.open(path);
      fs << code;
      fs.close();
    }
  }

  /// @brief Add clEnqueueReadBuffer to the generated code
  ///
  /// @param[in] buffer_id ID of the buffer to be used
  /// @param[in] blocking_read Blocking or non blocking call
  /// @param[in] offset Offset of the reading zone in bytes
  /// @param[in] size Size of the reading zone in bytes
  /// @param[in] callback_id ID of the callback
  void gen_read_buffer(size_t buffer_id, cl_bool blocking_read, size_t offset,
                       size_t size, cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("READ_BUFFER");

    p->buffer_ids.push_back(std::to_string(buffer_id));
    p->blockings.push_back(std::to_string(blocking_read));
    p->offsets.push_back(std::to_string(offset));
    p->sizes.push_back(std::to_string(size));
  }

  /// @brief Add clEnqueueWriteBuffer to the generated code
  ///
  /// @param[in] buffer_id ID of the buffer to be used
  /// @param[in] blocking_write Blocking or non blocking call
  /// @param[in] offset Offset of the writing zone in bytes
  /// @param[in] size Size of the writing zone in bytes
  /// @param[in] callback_id ID of the callback
  void gen_write_buffer(size_t buffer_id, cl_bool blocking_write, size_t offset,
                        size_t size, cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("WRITE_BUFFER");

    p->buffer_ids.push_back(std::to_string(buffer_id));
    p->blockings.push_back(std::to_string(blocking_write));
    p->offsets.push_back(std::to_string(offset));
    p->sizes.push_back(std::to_string(size));
  }

  /// @brief Add clEnqueueReadBuffer to the generated code
  ///
  /// @param buffer_id ID of the buffer to be used
  /// @param blocking_read Blocking or non blocking call
  /// @param buffer_origin Origin of the buffer zone in bytes
  /// @param host_origin Origin of the host zone in bytes
  /// @param region Region of the buffer zone in bytes
  /// @param buffer_row_pitch Row pitch of the buffer in bytes
  /// @param buffer_slice_pitch Slice pitch of the buffer in bytes
  /// @param host_row_pitch Row pitch of the host buffer in bytes
  /// @param host_slice_pitch Slice pitch of the host buffer in bytes
  /// @param[in] callback_id ID of the callback
  void gen_read_buffer_rect(size_t buffer_id, cl_bool blocking_read,
                            std::array<size_t, 3> buffer_origin,
                            std::array<size_t, 3> host_origin,
                            std::array<size_t, 3> region,
                            size_t buffer_row_pitch, size_t buffer_slice_pitch,
                            size_t host_row_pitch, size_t host_slice_pitch,
                            cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("READ_BUFFER_RECT");

    p->buffer_ids.push_back(std::to_string(buffer_id));
    p->blockings.push_back(std::to_string(blocking_read));

    p->buffer_origins.push_back("{" + std::to_string(buffer_origin[0]) + ", " +
                                std::to_string(buffer_origin[1]) + ", " +
                                std::to_string(buffer_origin[2]) + "}");
    p->host_origins.push_back("{" + std::to_string(host_origin[0]) + ", " +
                              std::to_string(host_origin[1]) + ", " +
                              std::to_string(host_origin[2]) + "}");
    p->regions.push_back("{" + std::to_string(region[0]) + ", " +
                         std::to_string(region[1]) + ", " +
                         std::to_string(region[2]) + "}");
    p->buffer_row_pitchs.push_back(std::to_string(buffer_row_pitch));
    p->buffer_slice_pitchs.push_back(std::to_string(buffer_slice_pitch));
    p->host_row_pitchs.push_back(std::to_string(host_row_pitch));
    p->host_slice_pitchs.push_back(std::to_string(host_slice_pitch));
  }

  /// @brief Add clEnqueueWriteBuffer to the generated code
  ///
  /// @param buffer_id ID of the buffer to be used
  /// @param blocking_write Blocking or non blocking call
  /// @param buffer_origin Origin of the buffer zone in bytes
  /// @param host_origin Origin of the host zone in bytes
  /// @param region Region of the buffer zone in bytes
  /// @param buffer_row_pitch Row pitch of the buffer in bytes
  /// @param buffer_slice_pitch Slice pitch of the buffer in bytes
  /// @param host_row_pitch Row pitch of the host buffer in bytes
  /// @param host_slice_pitch Slice pitch of the host buffer in bytes
  /// @param[in] callback_id ID of the callback
  void gen_write_buffer_rect(size_t buffer_id, cl_bool blocking_write,
                             std::array<size_t, 3> buffer_origin,
                             std::array<size_t, 3> host_origin,
                             std::array<size_t, 3> region,
                             size_t buffer_row_pitch, size_t buffer_slice_pitch,
                             size_t host_row_pitch, size_t host_slice_pitch,
                             cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("WRITE_BUFFER_RECT");

    p->buffer_ids.push_back(std::to_string(buffer_id));
    p->blockings.push_back(std::to_string(blocking_write));

    p->buffer_origins.push_back("{" + std::to_string(buffer_origin[0]) + ", " +
                                std::to_string(buffer_origin[1]) + ", " +
                                std::to_string(buffer_origin[2]) + "}");
    p->host_origins.push_back("{" + std::to_string(host_origin[0]) + ", " +
                              std::to_string(host_origin[1]) + ", " +
                              std::to_string(host_origin[2]) + "}");
    p->regions.push_back("{" + std::to_string(region[0]) + ", " +
                         std::to_string(region[1]) + ", " +
                         std::to_string(region[2]) + "}");
    p->buffer_row_pitchs.push_back(std::to_string(buffer_row_pitch));
    p->buffer_slice_pitchs.push_back(std::to_string(buffer_slice_pitch));
    p->host_row_pitchs.push_back(std::to_string(host_row_pitch));
    p->host_slice_pitchs.push_back(std::to_string(host_slice_pitch));
  }

  /// @brief Add clEnqueueFillBuffer to the generated code
  ///
  /// @param buffer_id ID of the buffer to be used
  /// @param pattern Pattern to fill the buffer with
  /// @param pattern_size Size of the pattern
  /// @param offset Offset of the region to be filled
  /// @param size Size of the region to be filled
  /// @param[in] callback_id ID of the callback
  void gen_fill_buffer(size_t buffer_id, std::vector<cl_int> pattern,
                       size_t pattern_size, size_t offset, size_t size,
                       cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("FILL_BUFFER");

    p->buffer_ids.push_back(std::to_string(buffer_id));

    std::string pattern_str = "{";
    for (size_t i = 0; i < pattern.size() - 1; i++) {
      pattern_str += std::to_string(pattern[i]) + ", ";
    }
    pattern_str += std::to_string(pattern[pattern.size() - 1]) + '}';
    p->patterns.push_back(pattern_str);

    p->pattern_sizes.push_back(std::to_string(pattern_size));

    p->offsets.push_back(std::to_string(offset));
    p->sizes.push_back(std::to_string(size));
  }

  /// @brief Add clEnqueueCopyBuffer to the generated code
  ///
  /// @param src_buffer_id ID of the src buffer
  /// @param dst_buffer_id ID of the dest buffer
  /// @param src_offset Offset of the copy zone in the src buffer
  /// @param dst_offset Offset of the copy zone in the dst buffer
  /// @param size Size of the copy zone
  /// @param[in] callback_id ID of the callback
  void gen_copy_buffer(size_t src_buffer_id, size_t dst_buffer_id,
                       size_t src_offset, size_t dst_offset, size_t size,
                       cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("COPY_BUFFER");

    p->src_buffer_ids.push_back(std::to_string(src_buffer_id));
    p->dst_buffer_ids.push_back(std::to_string(dst_buffer_id));

    p->src_offsets.push_back(std::to_string(src_offset));
    p->dst_offsets.push_back(std::to_string(dst_offset));

    p->sizes.push_back(std::to_string(size));
  }

  /// @brief Add clEnqueueCopyBufferRect to the generated code
  ///
  /// @param src_buffer_id ID of the src buffer
  /// @param dst_buffer_id ID of the dest buffer
  /// @param src_origin Origin of the src buffer zocne
  /// @param dst_origin Origin of the dst buffer zone
  /// @param region Size of the src buffer zone to copy
  /// @param src_row_pitch Row pitch of the src buffer
  /// @param src_slice_pitch Slice pitch of the src buffer
  /// @param dst_row_pitch Row pitch of the dst buffer
  /// @param dst_slice_pitch Slice pitch of the dst buffer
  /// @param[in] callback_id ID of the callback
  void gen_copy_buffer_rect(size_t src_buffer_id, size_t dst_buffer_id,
                            std::array<size_t, 3> src_origin,
                            std::array<size_t, 3> dst_origin,
                            std::array<size_t, 3> region, size_t src_row_pitch,
                            size_t src_slice_pitch, size_t dst_row_pitch,
                            size_t dst_slice_pitch,
                            cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("COPY_BUFFER_RECT");

    p->src_buffer_ids.push_back(std::to_string(src_buffer_id));
    p->dst_buffer_ids.push_back(std::to_string(dst_buffer_id));

    p->src_origins.push_back("{" + std::to_string(src_origin[0]) + ", " +
                             std::to_string(src_origin[1]) + ", " +
                             std::to_string(src_origin[2]) + "}");
    p->dst_origins.push_back("{" + std::to_string(dst_origin[0]) + ", " +
                             std::to_string(dst_origin[1]) + ", " +
                             std::to_string(dst_origin[2]) + "}");
    p->regions.push_back("{" + std::to_string(region[0]) + ", " +
                         std::to_string(region[1]) + ", " +
                         std::to_string(region[2]) + "}");

    p->src_row_pitchs.push_back(std::to_string(src_row_pitch));
    p->src_slice_pitchs.push_back(std::to_string(src_slice_pitch));
    p->dst_row_pitchs.push_back(std::to_string(dst_row_pitch));
    p->dst_slice_pitchs.push_back(std::to_string(dst_slice_pitch));
  }

  /// @brief Add clEnqueueMapBuffer to the generated code
  ///
  /// @param buffer_id ID of the buffer
  /// @param blocking_map Blocking or non blocking call
  /// @param flag Map flag
  /// @param offset Offset of the buffer zone to map
  /// @param size Size of the buffer zone to map
  /// @param[in] callback_id ID of the callback
  void gen_map_buffer(size_t buffer_id, cl_bool blocking_map,
                      cl_map_flags map_flag, size_t offset, size_t size,
                      cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("MAP_BUFFER");

    p->buffer_ids.push_back(std::to_string(buffer_id));

    p->blockings.push_back(std::to_string(blocking_map));

    switch (map_flag) {
      case CL_MAP_READ:
        p->map_flags.push_back("CL_MAP_READ");
        break;
      case CL_MAP_WRITE:
        p->map_flags.push_back("CL_MAP_WRITE");
        break;
    }

    p->offsets.push_back(std::to_string(offset));
    p->sizes.push_back(std::to_string(size));
  }

  /// @brief Add clEnqueueReadImage to the generated code
  ///
  /// @param image_id ID of the image
  /// @param blocking_read Blocking or non blocking call
  /// @param origin Origin of the image zone
  /// @param region Size of the image zone
  /// @param row_pitch Row pitch of the host buffer
  /// @param slice_pitch Slice pitch of the host buffer
  /// @param[in] callback_id ID of the callback
  void gen_read_image(size_t image_id, cl_bool blocking_read,
                      std::array<size_t, 3> origin,
                      std::array<size_t, 3> region, size_t row_pitch,
                      size_t slice_pitch, cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("READ_IMAGE");

    p->image_ids.push_back(std::to_string(image_id));

    p->blockings.push_back(std::to_string(blocking_read));

    p->image_origins.push_back('{' + std::to_string(origin[0]) + ", " +
                               std::to_string(origin[1]) + ", " +
                               std::to_string(origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');

    p->image_row_pitchs.push_back(std::to_string(row_pitch));
    p->image_slice_pitchs.push_back(std::to_string(slice_pitch));
  }

  /// @brief Add clEnqueueWriteImage to the generated code
  ///
  /// @param image_id ID of the image
  /// @param blocking_write Blocking or non blocking call
  /// @param origin Origin of the image zone
  /// @param region Size of the image zone
  /// @param row_pitch Row pitch of the host buffer
  /// @param slice_pitch Slice pitch of the host buffer
  /// @param[in] callback_id ID of the callback
  void gen_write_image(size_t image_id, cl_bool blocking_write,
                       std::array<size_t, 3> origin,
                       std::array<size_t, 3> region, size_t row_pitch,
                       size_t slice_pitch,
                       cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("WRITE_IMAGE");
    p->image_ids.push_back(std::to_string(image_id));

    p->blockings.push_back(std::to_string(blocking_write));

    p->image_origins.push_back('{' + std::to_string(origin[0]) + ", " +
                               std::to_string(origin[1]) + ", " +
                               std::to_string(origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');

    p->image_row_pitchs.push_back(std::to_string(row_pitch));
    p->image_slice_pitchs.push_back(std::to_string(slice_pitch));
  }

  /// @brief Add clEnqueueFillImage to the generated code
  ///
  /// @param image_id ID of the image
  /// @param fill_color Color to fill the image with
  /// @param origin Origin of the fill zone
  /// @param region Size of the fill zone
  /// @param[in] callback_id ID of the callback
  void gen_fill_image(size_t image_id, std::array<cl_int, 4> fill_color,
                      std::array<size_t, 3> origin,
                      std::array<size_t, 3> region,
                      cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("FILL_IMAGE");

    p->image_ids.push_back(std::to_string(image_id));

    p->image_fill_colors.push_back('{' + std::to_string(fill_color[0]) + ", " +
                                   std::to_string(fill_color[1]) + ", " +
                                   std::to_string(fill_color[2]) +
                                   std::to_string(fill_color[3]) + '}');
    p->image_origins.push_back('{' + std::to_string(origin[0]) + ", " +
                               std::to_string(origin[1]) + ", " +
                               std::to_string(origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');
  }

  /// @brief Add clEnqueueCopyImage to the generated code
  ///
  /// @param src_image_id ID of the src image
  /// @param dst_image_id ID of the dst image
  /// @param src_origin Origin of the src image zone
  /// @param dst_origin Origin of the dst image zone
  /// @param region Size of the copy zone
  /// @param[in] callback_id ID of the callback
  void gen_copy_image(size_t src_image_id, size_t dst_image_id,
                      std::array<size_t, 3> src_origin,
                      std::array<size_t, 3> dst_origin,
                      std::array<size_t, 3> region,
                      cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("COPY_IMAGE");

    p->src_image_ids.push_back(std::to_string(src_image_id));
    p->dst_image_ids.push_back(std::to_string(dst_image_id));

    p->image_src_origins.push_back('{' + std::to_string(src_origin[0]) + ", " +
                                   std::to_string(src_origin[1]) + ", " +
                                   std::to_string(src_origin[2]) + '}');
    p->image_dst_origins.push_back('{' + std::to_string(dst_origin[0]) + ", " +
                                   std::to_string(dst_origin[1]) + ", " +
                                   std::to_string(dst_origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');
  }

  /// @brief Add clEnqueueCopyImageToBuffer to the generated code
  ///
  /// @param src_image_id ID of the src image
  /// @param dst_buffer_id ID of the dst buffer
  /// @param src_image_origin Origin of the image copy zone
  /// @param image_region Size of the image copy zone
  /// @param dst_buffer Offset of the buffer zone
  /// @param[in] callback_id ID of the callback
  void gen_copy_image_to_buffer(size_t src_image_id, size_t dst_buffer_id,
                                std::array<size_t, 3> src_origin,
                                std::array<size_t, 3> region, size_t dst_offset,
                                cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("COPY_IMAGE_TO_BUFFER");

    p->src_image_ids.push_back(std::to_string(src_image_id));
    p->dst_buffer_ids.push_back(std::to_string(dst_buffer_id));

    p->image_src_origins.push_back('{' + std::to_string(src_origin[0]) + ", " +
                                   std::to_string(src_origin[1]) + ", " +
                                   std::to_string(src_origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');
    p->dst_offsets.push_back(std::to_string(dst_offset));
  }

  /// @brief Add clEnqueueCopyBufferToImage to the generated code
  ///
  /// @param src_buffer_id ID of the src buffer
  /// @param dst_image_id ID of the dst image
  /// @param src_offset Offset of the buffer zone
  /// @param dst_origin Origin of the image zone
  /// @param region Size of the copy zone
  /// @param[in] callback_id ID of the callback
  void gen_copy_buffer_to_image(size_t src_buffer_id, size_t dst_image_id,
                                size_t src_offset,
                                std::array<size_t, 3> dst_origin,
                                std::array<size_t, 3> region,
                                cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("COPY_BUFFER_TO_IMAGE");

    p->src_buffer_ids.push_back(std::to_string(src_buffer_id));
    p->dst_image_ids.push_back(std::to_string(dst_image_id));

    p->src_offsets.push_back(std::to_string(src_offset));

    p->image_dst_origins.push_back('{' + std::to_string(dst_origin[0]) + ", " +
                                   std::to_string(dst_origin[1]) + ", " +
                                   std::to_string(dst_origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');
  }

  /// @brief Add clEnqueueMapImage to the generated code
  ///
  /// @param image_id ID of the image
  /// @param blocking_map Blocking or non blocking call
  /// @param map_flag Read of Write map
  /// @param origin Origin of the mapped region
  /// @param region Size of the mapped zone
  /// @param[in] callback_id ID of the callback
  void gen_map_image(size_t image_id, cl_bool blocking_map,
                     cl_map_flags map_flag, std::array<size_t, 3> origin,
                     std::array<size_t, 3> region,
                     cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("MAP_IMAGE");

    p->image_ids.push_back(std::to_string(image_id));
    p->blockings.push_back(std::to_string(blocking_map));

    switch (map_flag) {
      case CL_MAP_READ:
        p->map_flags.push_back("CL_MAP_READ");
        break;
      case CL_MAP_WRITE:
        p->map_flags.push_back("CL_MAP_WRITE");
        break;
    }

    p->image_origins.push_back('{' + std::to_string(origin[0]) + ", " +
                               std::to_string(origin[1]) + ", " +
                               std::to_string(origin[2]) + '}');
    p->image_regions.push_back('{' + std::to_string(region[0]) + ", " +
                               std::to_string(region[1]) + ", " +
                               std::to_string(region[2]) + '}');
  }

  /// @brief Add clEnqueueUnmapMemObject to the generated code
  ///
  /// @param mapped_ptr_index Index of the mapped ptr
  /// @param[in] callback_id ID of the callback
  void gen_unmap_mem_object(size_t map_ptr_index,
                            cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("UNMAP_MEM_OBJECT");

    p->map_ptr_indexs.push_back(std::to_string(map_ptr_index));
  }

  /// @brief Add clEnqueueNDRangeKernel to the generated code
  ///
  /// @param[in] is_callback Is this called from a callback
  /// @param[in] callback_id ID of the callback
  void gen_nd_range_kernel(cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("ND_RANGE_KERNEL");
  }

  /// @brief Add clEnqueueTask to the generated code
  ///
  /// @param[in] callback_id ID of the callback
  void gen_task(cargo::optional<size_t> callback_id) {
    const std::lock_guard<std::mutex> lock(mutex);
    fuzzcl::exec_params_t *p = get_exec_params(callback_id);
    p->commands.push_back("TASK");
  }

  /// @brief Add clSetEventCallback to the generated code
  ///
  /// @param buffer_or_image Is the event associated to a buffer or an image
  /// @param mem_obj_id ID of the mem_obj the event is associated to
  /// @param callback_id ID of the new callback
  /// @param command_exec_callback_type Type of the new callback
  void gen_set_event_callback(bool buffer_or_image, size_t mem_obj_id,
                              size_t callback_id,
                              cl_int command_exec_callback_type) {
    std::unique_lock<std::mutex> lock(mutex);
    // clSetEventCallback can only be called from the main execution
    main_exec_params.commands.push_back("SET_EVENT_CALLBACK");
    lock.unlock();
    // we can release the lock here since these arrays can only be modified by
    // one thread.

    main_exec_params.buffer_or_images.push_back(
        std::to_string(buffer_or_image));
    main_exec_params.mem_obj_ids.push_back(std::to_string(mem_obj_id));
    main_exec_params.callback_ids.push_back(std::to_string(callback_id));
    main_exec_params.command_exec_callback_types.push_back(
        std::to_string(command_exec_callback_type));

    // create an exec_params_t for this callback
    callback_exec_params.emplace_back(fuzzcl::exec_params_t{});
  }

 private:
  fuzzcl::exec_params_t main_exec_params;
  std::vector<fuzzcl::exec_params_t> callback_exec_params;

  /// @brief Get an exec_params_t
  ///
  /// @param callback_id ID of the callback
  ///
  /// @return Returns the exec_params_t
  fuzzcl::exec_params_t *get_exec_params(cargo::optional<size_t> callback_id) {
    fuzzcl::exec_params_t *p = &main_exec_params;
    if (callback_id.has_value() &&
        callback_id.value() < callback_exec_params.size()) {
      p = &callback_exec_params[callback_id.value()];
    }
    return p;
  }

  /// @brief Add a UnitCL test general structure
  void gen_unitcl_context() {
    code += R"(#include "Common.h"
#include <array>
#include <mutex>
#include <stack>
#include <thread>

class FuzzTest;
namespace {
struct mem_object_t {
  cl_mem m;
  std::stack<cl_event> event_stack;
};
struct map_ptr_t {
  mem_object_t *mem_obj;
  void *p;
  size_t image_row_pitch;

  map_ptr_t(mem_object_t *mem_obj, void *ptr, size_t image_row_pitch = 0)
      : mem_obj(mem_obj), p(ptr), image_row_pitch(image_row_pitch) {}
};
enum command_t {
  READ_BUFFER,
  WRITE_BUFFER,
  READ_BUFFER_RECT,
  WRITE_BUFFER_RECT,
  FILL_BUFFER,
  COPY_BUFFER,
  COPY_BUFFER_RECT,
  MAP_BUFFER,
  READ_IMAGE,
  WRITE_IMAGE,
  FILL_IMAGE,
  COPY_IMAGE,
  COPY_IMAGE_TO_BUFFER,
  COPY_BUFFER_TO_IMAGE,
  MAP_IMAGE,
  UNMAP_MEM_OBJECT,
  ND_RANGE_KERNEL,
  TASK,
  SET_EVENT_CALLBACK
};
template <class T>
struct param_t {
  const std::vector<T> values;

  param_t(const std::vector<T> values) : values(values) {}

  T next() {
    if (index >= values.size()) {
      std::cerr << "Failed to get the next param value\n";
      exit(1);
    }
    return values[index++];
  }

  size_t size() { return values.size(); }

 private:
  size_t index = 0;
};
struct exec_params_t {
  const size_t max_num_threads;
  const size_t max_num_buffers;
  const size_t max_num_images;

  const size_t buffer_width;
  const size_t buffer_height;
  const size_t buffer_size;

  const cl_image_format image_format;
  const cl_image_desc image_desc;

  const cl_uint work_dim;
  const size_t global_work_offset;
  const size_t global_work_size;

  param_t<command_t> commands;

  param_t<size_t> buffer_ids;

  param_t<cl_bool> blockings;

  param_t<size_t> offsets;
  param_t<size_t> sizes;

  param_t<std::array<size_t, 3>> buffer_origins;
  param_t<std::array<size_t, 3>> host_origins;
  param_t<std::array<size_t, 3>> regions;
  param_t<size_t> buffer_row_pitchs;
  param_t<size_t> buffer_slice_pitchs;
  param_t<size_t> host_row_pitchs;
  param_t<size_t> host_slice_pitchs;

  param_t<std::vector<cl_int>> patterns;
  param_t<size_t> pattern_sizes;

  param_t<size_t> src_buffer_ids;
  param_t<size_t> dst_buffer_ids;
  param_t<size_t> src_offsets;
  param_t<size_t> dst_offsets;

  param_t<std::array<size_t, 3>> src_origins;
  param_t<std::array<size_t, 3>> dst_origins;
  param_t<size_t> src_row_pitchs;
  param_t<size_t> src_slice_pitchs;
  param_t<size_t> dst_row_pitchs;
  param_t<size_t> dst_slice_pitchs;

  param_t<size_t> image_ids;
  param_t<std::array<size_t, 3>> image_origins;
  param_t<std::array<size_t, 3>> image_regions;
  param_t<size_t> image_row_pitchs;
  param_t<size_t> image_slice_pitchs;

  param_t<std::array<cl_int, 4>> image_fill_colors;

  param_t<size_t> src_image_ids;
  param_t<size_t> dst_image_ids;
  param_t<std::array<size_t, 3>> image_src_origins;
  param_t<std::array<size_t, 3>> image_dst_origins;

  param_t<cl_map_flags> map_flags;
  param_t<size_t> map_ptr_indexs;

  param_t<bool> buffer_or_images;
  param_t<size_t> mem_obj_ids;
  param_t<size_t> callback_ids;
  param_t<cl_int> command_exec_callback_types;
};
struct callback_data_t {
  FuzzTest * t;
  exec_params_t params;
};
}  // namespace

class FuzzTest : public CodeplayTestWrapper {
 protected:
  void SetUp() override {
    context = clCreateContext(nullptr, UCL::getNumDevices(), UCL::getDevices(),
                              nullptr, nullptr, &error_code);
    EXPECT_TRUE(context);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

    queue = clCreateCommandQueue(context, UCL::getDevices()[0], 0, &error_code);
    EXPECT_TRUE(queue);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

    const char *source = "void kernel foo() {}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &error_code);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &error_code);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);
  }

  void TearDown() override {
    for (map_ptr_t &map_ptr : map_ptrs) {
      const cl_uint num_events_in_wait_list =
          map_ptr.mem_obj->event_stack.size() > 0 ? 1 : 0;
      const cl_event *event_wait_list =
          num_events_in_wait_list == 1 ? &map_ptr.mem_obj->event_stack.top()
                                       : NULL;
      cl_event event;

      ASSERT_SUCCESS(clEnqueueUnmapMemObject(queue, map_ptr.mem_obj->m,
                                             map_ptr.p, num_events_in_wait_list,
                                             event_wait_list, &event));

      map_ptr.mem_obj->event_stack.push(event);
    }

    ASSERT_SUCCESS(clFinish(queue));

    for (size_t i = 0; i < buffers.size(); i++) {
      ASSERT_SUCCESS(clReleaseMemObject(buffers[i]->m));
      while (!buffers[i]->event_stack.empty()) {
        ASSERT_SUCCESS(clReleaseEvent(buffers[i]->event_stack.top()));
        buffers[i]->event_stack.pop();
      }
    }

    for (size_t i = 0; i < images.size(); i++) {
      ASSERT_SUCCESS(clReleaseMemObject(images[i]->m));
      while (!images[i]->event_stack.empty()) {
        ASSERT_SUCCESS(clReleaseEvent(images[i]->event_stack.top()));
        images[i]->event_stack.pop();
      }
    }

    EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    EXPECT_SUCCESS(clReleaseContext(context));
  }

  cl_context context;
  cl_command_queue queue;

  cl_program program;
  cl_kernel kernel;

  std::vector<std::unique_ptr<mem_object_t>> buffers;
  std::vector<std::unique_ptr<std::vector<cl_int>>> host_buffers;

  std::vector<std::unique_ptr<mem_object_t>> images;
  std::vector<std::unique_ptr<std::vector<cl_int4>>> image_host_buffers;

  std::vector<map_ptr_t> map_ptrs;

  std::vector<std::unique_ptr<std::vector<cl_event>>> event_wait_lists;

  cl_int error_code;

  std::mutex mutex;

  std::vector<std::unique_ptr<callback_data_t>> callback_datas;

  void CL_CALLBACK callback(cl_event, cl_int, void *user_data) {
    exec_params_t test_data = *static_cast<exec_params_t *>(user_data);
    run_test(test_data);
  }

  void run_test(exec_params_t params,
                std::vector<exec_params_t> callback_params = {}) {
    // This could have been used in the constructor, but different tests might
    // have different numbers of images and buffers
    {
      std::lock_guard<std::mutex> lock(mutex);
      while (buffers.size() < params.max_num_buffers) {
        cl_mem mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        params.buffer_size * sizeof(cl_int),
                                        NULL, &error_code);
        ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

        buffers.emplace_back(new mem_object_t{mem_obj, std::stack<cl_event>()});
      }
      while (images.size() < params.max_num_images) {
        cl_mem mem_obj =
            clCreateImage(context, CL_MEM_READ_WRITE, &params.image_format,
                          &params.image_desc, NULL, &error_code);
        ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

        images.emplace_back(new mem_object_t{mem_obj, std::stack<cl_event>()});
      }
    }

    for (size_t i = 0; i < params.commands.size(); i++) {
      switch (params.commands.next()) {
)";
    code += R"(case READ_BUFFER: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const cl_bool blocking = params.blockings.next();
          const size_t offset = params.offsets.next();
          const size_t size = params.sizes.next();
          
          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          host_buffers.emplace_back(new std::vector<cl_int>(size));

          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueReadBuffer(
              queue, buffer->m, blocking, offset * sizeof(cl_int),
              size * sizeof(cl_int), host_buffers.back()->data(),
              num_events_in_wait_list, event_wait_list, &event));
          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case WRITE_BUFFER: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const cl_bool blocking = params.blockings.next();
          const size_t offset = params.offsets.next();
          const size_t size = params.sizes.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          host_buffers.emplace_back(new std::vector<cl_int>(size));

          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueWriteBuffer(
              queue, buffer->m, blocking, offset * sizeof(cl_int),
              size * sizeof(cl_int), host_buffers.back()->data(),
              num_events_in_wait_list, event_wait_list, &event));
          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case READ_BUFFER_RECT: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const cl_bool blocking = params.blockings.next();

          const size_t buffer_row_pitch = params.buffer_row_pitchs.next();
          const size_t buffer_slice_pitch = params.buffer_slice_pitchs.next();
          const size_t host_row_pitch = params.host_row_pitchs.next();
          const size_t host_slice_pitch = params.host_slice_pitchs.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          host_buffers.emplace_back(
              new std::vector<cl_int>(params.buffer_size));

          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueReadBufferRect(
              queue, buffer->m, blocking, params.buffer_origins.next().data(),
              params.host_origins.next().data(), params.regions.next().data(),
              buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
              host_slice_pitch, host_buffers.back()->data(),
              num_events_in_wait_list, event_wait_list, &event));
          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case WRITE_BUFFER_RECT: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const cl_bool blocking = params.blockings.next();

          const size_t buffer_row_pitch = params.buffer_row_pitchs.next();
          const size_t buffer_slice_pitch = params.buffer_slice_pitchs.next();
          const size_t host_row_pitch = params.host_row_pitchs.next();
          const size_t host_slice_pitch = params.host_slice_pitchs.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          host_buffers.emplace_back(
              new std::vector<cl_int>(params.buffer_size));

          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueWriteBufferRect(
              queue, buffer->m, blocking, params.buffer_origins.next().data(),
              params.host_origins.next().data(), params.regions.next().data(),
              buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
              host_slice_pitch, host_buffers.back()->data(),
              num_events_in_wait_list, event_wait_list, &event));
          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case FILL_BUFFER: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const size_t pattern_size = params.pattern_sizes.next();

          const size_t offset = params.offsets.next();
          const size_t size = params.sizes.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueFillBuffer(
              queue, buffer->m, params.patterns.next().data(), pattern_size,
              offset, size, num_events_in_wait_list, event_wait_list, &event));
          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case COPY_BUFFER: {
          mem_object_t *src_buffer =
              buffers[params.src_buffer_ids.next()].get();
          mem_object_t *dst_buffer =
              buffers[params.dst_buffer_ids.next()].get();

          const size_t src_offset = params.src_offsets.next();
          const size_t dst_offset = params.dst_offsets.next();
          const size_t size = params.sizes.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          cl_int num_events_in_wait_list = 0;
          event_wait_lists.emplace_back(new std::vector<cl_event>());
          if (src_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(src_buffer->event_stack.top());
          }
          if (dst_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(dst_buffer->event_stack.top());
          }
          cl_event event;

          ASSERT_SUCCESS(clEnqueueCopyBuffer(
              queue, src_buffer->m, dst_buffer->m, src_offset, dst_offset, size,
              num_events_in_wait_list, event_wait_lists.back()->data(),
              &event));

          clRetainEvent(event);
          src_buffer->event_stack.push(event);
          dst_buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case COPY_BUFFER_RECT: {
          mem_object_t *src_buffer =
              buffers[params.src_buffer_ids.next()].get();
          mem_object_t *dst_buffer =
              buffers[params.dst_buffer_ids.next()].get();

          const size_t src_row_pitch = params.src_row_pitchs.next();
          const size_t src_slice_pitch = params.src_slice_pitchs.next();
          const size_t dst_row_pitch = params.dst_row_pitchs.next();
          const size_t dst_slice_pitch = params.dst_slice_pitchs.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          cl_int num_events_in_wait_list = 0;
          event_wait_lists.emplace_back(new std::vector<cl_event>());
          if (src_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(src_buffer->event_stack.top());
          }
          if (dst_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(dst_buffer->event_stack.top());
          }
          cl_event event;

          ASSERT_SUCCESS(clEnqueueCopyBufferRect(
              queue, src_buffer->m, dst_buffer->m,
              params.src_origins.next().data(),
              params.dst_origins.next().data(), params.regions.next().data(),
              src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
              num_events_in_wait_list, event_wait_lists.back()->data(),
              &event));

          clRetainEvent(event);
          src_buffer->event_stack.push(event);
          dst_buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case MAP_BUFFER: {
          mem_object_t *buffer = buffers[params.buffer_ids.next()].get();

          const cl_bool blocking_map = params.blockings.next();

          const cl_map_flags map_flag = params.map_flags.next();

          const size_t offset = params.offsets.next();
          const size_t size = params.sizes.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          const cl_uint num_events_in_wait_list =
              buffer->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &buffer->event_stack.top() : NULL;
          cl_event event;

          void *map_ptr = clEnqueueMapBuffer(
              queue, buffer->m, blocking_map, map_flag, offset, size,
              num_events_in_wait_list, event_wait_list, &event, &error_code);
          ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

          map_ptrs.emplace_back(map_ptr_t{buffer, map_ptr});

          buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case READ_IMAGE: {
          mem_object_t *image = images[params.image_ids.next()].get();

          const cl_bool blocking_read = params.blockings.next();

          const size_t row_pitch = params.image_row_pitchs.next();
          const size_t slice_pitch = params.image_slice_pitchs.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          image_host_buffers.emplace_back(
              new std::vector<cl_int4>(params.buffer_size));

          const cl_uint num_events_in_wait_list =
              image->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueReadImage(
              queue, image->m, blocking_read,
              params.image_origins.next().data(),
              params.image_regions.next().data(), row_pitch, slice_pitch,
              image_host_buffers.back()->data(), num_events_in_wait_list,
              event_wait_list, &event));

          image->event_stack.push(event);
          break;
        }
)";
    code += R"(case WRITE_IMAGE: {
          mem_object_t *image = images[params.image_ids.next()].get();

          const cl_bool blocking_write = params.blockings.next();

          const size_t row_pitch = params.image_row_pitchs.next();
          const size_t slice_pitch = params.image_slice_pitchs.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          image_host_buffers.emplace_back(
              new std::vector<cl_int4>(params.buffer_size));

          const cl_uint num_events_in_wait_list =
              image->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueWriteImage(
              queue, image->m, blocking_write,
              params.image_origins.next().data(),
              params.image_regions.next().data(), row_pitch, slice_pitch,
              image_host_buffers.back()->data(), num_events_in_wait_list,
              event_wait_list, &event));

          image->event_stack.push(event);
          break;
        }
)";
    code += R"(case FILL_IMAGE: {
          mem_object_t *image = images[params.image_ids.next()].get();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          const cl_uint num_events_in_wait_list =
              image->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueFillImage(
              queue, image->m, params.image_fill_colors.next().data(),
              params.image_origins.next().data(),
              params.image_regions.next().data(), num_events_in_wait_list,
              event_wait_list, &event));

          image->event_stack.push(event);
          break;
        }
)";
    code += R"(case COPY_IMAGE: {
          mem_object_t *src_image = images[params.src_image_ids.next()].get();
          mem_object_t *dst_image = images[params.dst_image_ids.next()].get();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          cl_int num_events_in_wait_list = 0;
          event_wait_lists.emplace_back(new std::vector<cl_event>());
          if (src_image->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(src_image->event_stack.top());
          }
          if (dst_image->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(dst_image->event_stack.top());
          }
          cl_event event;

          ASSERT_SUCCESS(clEnqueueCopyImage(
              queue, src_image->m, dst_image->m,
              params.image_src_origins.next().data(),
              params.image_dst_origins.next().data(),
              params.image_regions.next().data(), num_events_in_wait_list,
              event_wait_lists.back()->data(), &event));

          clRetainEvent(event);
          src_image->event_stack.push(event);
          dst_image->event_stack.push(event);
          break;
        }
)";
    code += R"(case COPY_IMAGE_TO_BUFFER: {
          mem_object_t *src_image = images[params.src_image_ids.next()].get();
          mem_object_t *dst_buffer =
              buffers[params.dst_buffer_ids.next()].get();

          const size_t dst_offset = params.dst_offsets.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          cl_int num_events_in_wait_list = 0;
          event_wait_lists.emplace_back(new std::vector<cl_event>());
          if (src_image->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(src_image->event_stack.top());
          }
          if (dst_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(dst_buffer->event_stack.top());
          }
          cl_event event;

          ASSERT_SUCCESS(clEnqueueCopyImageToBuffer(
              queue, src_image->m, dst_buffer->m,
              params.image_src_origins.next().data(),
              params.image_regions.next().data(), dst_offset,
              num_events_in_wait_list, event_wait_lists.back()->data(),
              &event));

          clRetainEvent(event);
          src_image->event_stack.push(event);
          dst_buffer->event_stack.push(event);
          break;
        }
)";
    code += R"(case COPY_BUFFER_TO_IMAGE: {
          mem_object_t *src_buffer =
              buffers[params.src_buffer_ids.next()].get();
          mem_object_t *dst_image = images[params.dst_image_ids.next()].get();

          const size_t src_offset = params.src_offsets.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          cl_int num_events_in_wait_list = 0;
          event_wait_lists.emplace_back(new std::vector<cl_event>());
          if (src_buffer->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(src_buffer->event_stack.top());
          }
          if (dst_image->event_stack.size() > 0) {
            num_events_in_wait_list++;
            event_wait_lists.back()->push_back(dst_image->event_stack.top());
          }
          cl_event event;

          ASSERT_SUCCESS(clEnqueueCopyBufferToImage(
              queue, src_buffer->m, dst_image->m, src_offset,
              params.image_dst_origins.next().data(),
              params.image_regions.next().data(), num_events_in_wait_list,
              event_wait_lists.back()->data(), &event));

          clRetainEvent(event);
          src_buffer->event_stack.push(event);
          dst_image->event_stack.push(event);
          break;
        }
)";
    code += R"(case MAP_IMAGE: {
          mem_object_t *image = images[params.image_ids.next()].get();

          const cl_bool blocking_map = params.blockings.next();
          const cl_map_flags map_flag = params.map_flags.next();

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          const cl_uint num_events_in_wait_list =
              image->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &image->event_stack.top() : NULL;
          cl_event event;

          size_t image_row_pitch;
          void *map_ptr = clEnqueueMapImage(
              queue, image->m, blocking_map, map_flag,
              params.image_origins.next().data(),
              params.image_regions.next().data(), &image_row_pitch, NULL,
              num_events_in_wait_list, event_wait_list, &event, &error_code);
          ASSERT_EQ_ERRCODE(CL_SUCCESS, error_code);

          map_ptrs.emplace_back(map_ptr_t{image, map_ptr, image_row_pitch});

          image->event_stack.push(event);
          break;
        }
)";
    code += R"(case UNMAP_MEM_OBJECT: {
          const size_t map_ptr_index = params.map_ptr_indexs.next();
          map_ptr_t map_ptr = map_ptrs[map_ptr_index];

          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          const cl_uint num_events_in_wait_list =
              map_ptr.mem_obj->event_stack.size() > 0 ? 1 : 0;
          const cl_event *event_wait_list =
              num_events_in_wait_list == 1 ? &map_ptr.mem_obj->event_stack.top()
                                           : NULL;
          cl_event event;

          ASSERT_SUCCESS(clEnqueueUnmapMemObject(
              queue, map_ptr.mem_obj->m, map_ptr.p, num_events_in_wait_list,
              event_wait_list, &event));

          map_ptr.mem_obj->event_stack.push(event);

          map_ptrs.erase(map_ptrs.begin() + map_ptr_index);
          break;
        }
)";
    code += R"(case ND_RANGE_KERNEL: {
          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          ASSERT_SUCCESS(clEnqueueNDRangeKernel(
              queue, kernel, params.work_dim, &params.global_work_offset,
              &params.global_work_size, nullptr, 0, NULL, NULL));
          break;
        }
)";
    code += R"(case TASK: {
          // the following block should be run without losing the lock
          std::lock_guard<std::mutex> lock(mutex);
          ASSERT_SUCCESS(clEnqueueTask(queue, kernel, 0, NULL, NULL));
          break;
        }
)";
    code += R"(case SET_EVENT_CALLBACK: {
          std::lock_guard<std::mutex> lock(mutex);
          cl_event event;
          if (params.buffer_or_images.next()) {
            event = buffers[params.mem_obj_ids.next()]->event_stack.top();
          } else {
            event = images[params.mem_obj_ids.next()]->event_stack.top();
          }

          const cl_int command_exec_callback_type =
              params.command_exec_callback_types.next();

          callback_datas.emplace_back(new callback_data_t{
              this, callback_params[params.callback_ids.next()]});
          ASSERT_SUCCESS(clSetEventCallback(
              event, command_exec_callback_type,
              [](cl_event, cl_int, void* user_data) {
                callback_data_t callback_data =
                    *static_cast<callback_data_t*>(user_data);
                callback_data.t->run_test(callback_data.params);
              },
              callback_datas.back().get()));
        }
)";
    code += R"(}
    }
    ASSERT_SUCCESS(clFlush(queue));
  }
};

TEST_F(FuzzTest, Default) {
)";
    code +=
        "const size_t max_num_threads = " + std::to_string(MAX_NUM_THREADS) +
        ";\n";
    code +=
        "const size_t max_num_buffers = " + std::to_string(MAX_NUM_BUFFERS) +
        ";\n";
    code += "const size_t max_num_images = " + std::to_string(MAX_NUM_IMAGES) +
            ";\n\n";
    code +=
        "const size_t buffer_width = " + std::to_string(BUFFER_WIDTH) + ";\n";
    code +=
        "const size_t buffer_height = " + std::to_string(BUFFER_HEIGHT) + ";\n";
    code +=
        "const size_t buffer_size = " + std::to_string(BUFFER_SIZE) + ";\n\n";
    code += "const cl_image_format image_format = " +
            std::string(STR(IMAGE_FORMAT)) + ";\n";
    code += "const cl_image_desc image_desc = " + std::string(STR(IMAGE_DESC)) +
            ";\n\n";
    code += "const cl_uint work_dim = " + std::to_string(WORK_DIM) + ";\n";
    code += "const size_t global_work_offset = " +
            std::to_string(GLOBAL_WORK_OFFSET) + ";\n";
    code +=
        "const size_t global_work_size = " + std::to_string(GLOBAL_WORK_SIZE) +
        ";\n\n";
  }

  /// @brief Add array to the generated code
  ///
  /// @param[in] type The type of the array
  /// @param[in] name The name of the array
  /// @param[in] array The values to assign to the array
  void gen_array(std::string type, std::string name,
                 const std::vector<std::string> &array) {
    code += "param_t<" + type + ">{{\n";
    for (size_t i = 0; i < array.size(); i++) {
      code += array[i];
      if (i != array.size() - 1) {
        code += ", ";
      }
    }
    code += "}}, // " + name + "\n";
  }

  /// @brief Add an exec_params_t to the generated code
  ///
  /// @param p The exec_params_t to add
  void gen_exec_params(exec_params_t *p) {
    code += "exec_params_t{\n";

    code += R"(max_num_threads,
      max_num_buffers,
      max_num_images,

      buffer_width,
      buffer_height,
      buffer_size,

      image_format,
      image_desc,

      work_dim,
      global_work_offset,
      global_work_size,

    )";

    gen_array("command_t", "commands", p->commands);

    gen_array("size_t", "buffer_ids", p->buffer_ids);

    gen_array("cl_bool", "blockings", p->blockings);

    gen_array("size_t", "offsets", p->offsets);
    gen_array("size_t", "sizes", p->sizes);

    gen_array("std::array<size_t, 3>", "buffer_origins", p->buffer_origins);
    gen_array("std::array<size_t, 3>", "host_origins", p->host_origins);
    gen_array("std::array<size_t, 3>", "regions", p->regions);
    gen_array("size_t", "buffer_row_pitchs", p->buffer_row_pitchs);
    gen_array("size_t", "buffer_slice_pitchs", p->buffer_slice_pitchs);
    gen_array("size_t", "host_row_pitchs", p->host_row_pitchs);
    gen_array("size_t", "host_slice_pitchs", p->host_slice_pitchs);

    gen_array("std::vector<cl_int>", "patterns", p->patterns);
    gen_array("size_t", "pattern_sizes", p->pattern_sizes);

    gen_array("size_t", "src_buffer_ids", p->src_buffer_ids);
    gen_array("size_t", "dst_buffer_ids", p->dst_buffer_ids);
    gen_array("size_t", "src_offsets", p->src_offsets);
    gen_array("size_t", "dst_offsets", p->dst_offsets);

    gen_array("std::array<size_t, 3>", "src_origins", p->src_origins);
    gen_array("std::array<size_t, 3>", "dst_origins", p->dst_origins);
    gen_array("size_t", "src_row_pitchs", p->src_row_pitchs);
    gen_array("size_t", "src_slice_pitchs", p->src_slice_pitchs);
    gen_array("size_t", "dst_row_pitchs", p->dst_row_pitchs);
    gen_array("size_t", "dst_slice_pitchs", p->dst_slice_pitchs);

    gen_array("size_t", "image_ids", p->image_ids);
    gen_array("std::array<size_t, 3>", "image_origins", p->image_origins);
    gen_array("std::array<size_t, 3>", "image_regions", p->image_regions);
    gen_array("size_t", "image_row_pitchs", p->image_row_pitchs);
    gen_array("size_t", "image_slice_pitchs", p->image_slice_pitchs);

    gen_array("std::array<cl_int, 4>", "image_fill_colors",
              p->image_fill_colors);

    gen_array("size_t", "src_image_ids", p->src_image_ids);
    gen_array("size_t", "dst_image_ids", p->dst_image_ids);
    gen_array("std::array<size_t, 3>", "image_src_origins",
              p->image_src_origins);
    gen_array("std::array<size_t, 3>", "image_dst_origins",
              p->image_dst_origins);

    gen_array("cl_map_flags", "map_flags", p->map_flags);
    gen_array("size_t", "map_ptr_index", p->map_ptr_indexs);

    gen_array("bool", "buffer_or_images", p->buffer_or_images);
    gen_array("size_t", "mem_obj_ids", p->mem_obj_ids);
    gen_array("size_t", "callback_ids", p->callback_ids);
    gen_array("cl_int", "command_exec_callback_types",
              p->command_exec_callback_types);
    code += "}";
  }

  /// @brief Add every callback exec_params_t to the generated code
  void gen_callback_exec_params() {
    code += "std::vector<exec_params_t> callback_exec_params;\n";
    for (size_t i = 0; i < callback_exec_params.size(); i++) {
      exec_params_t *p = &callback_exec_params[i];
      code += "callback_exec_params.emplace_back(";
      gen_exec_params(p);
      code += ");\n";
    }
    code += "\n\n";
  }

  /// @brief Add the main exec_params_t to the generated code
  void gen_main_exec_params() {
    code += "exec_params_t main_exec_params = ";
    gen_exec_params(&main_exec_params);
    code += ";\n";
  }

  /// @brief Generated a UnitCL test
  void gen_test() {
    gen_callback_exec_params();
    gen_main_exec_params();
    code += R"(
  std::vector<std::thread> running_threads;
  running_threads.reserve(max_num_threads);
  for (size_t i = 0; i < max_num_threads; i++) {
    running_threads.emplace_back(std::thread(
        [this](exec_params_t main_exec_params,
               std::vector<exec_params_t> callback_exec_params) {
          run_test(main_exec_params, callback_exec_params);
        },
        main_exec_params, callback_exec_params));
  }

  for (std::thread &thread : running_threads) {
    thread.join();
  }
)";
  }
};  // struct code_generator_t
}  // namespace fuzzcl

#endif
