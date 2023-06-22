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

#ifndef VK_PIPELINE_H_INCLUDED
#define VK_PIPELINE_H_INCLUDED

#include <mux/mux.hpp>
#include <spirv-ll/module.h>
#include <vk/allocator.h>
#include <vk/error.h>
#include <vk/small_vector.h>

#include <array>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::shader_module_t
typedef struct shader_module_t *shader_module;

/// @copydoc ::vk::pipeline_cache_t
typedef struct pipeline_cache_t *pipeline_cache;

/// @copydoc ::vk::pipeline_layout_t
typedef struct pipeline_layout_t *pipeline_layout;

/// @brief internal pipeline type
typedef struct pipeline_t final {
  /// @brief Constructor for compiled shader.
  ///
  /// @param compiler_module Compiler module that the kernel object was created
  /// from.
  /// @param compiler_kernel Compiler kernel object to be passed into the
  /// command buffer at bind pipeline.
  /// @param allocator Allocator used to allocate the pipeline's memory
  pipeline_t(std::unique_ptr<compiler::Module> compiler_module,
             compiler::Kernel *compiler_kernel, vk::allocator allocator);

  /// @brief Constructor for cached binary shader.
  ///
  /// @param mux_binary_executable Mux executable that encapsulates a cached
  /// binary shader.
  /// @param mux_binary_kernel Mux kernel object to be passed into the command
  /// buffer at bind pipeline.
  /// @param allocator Allocator used to allocate the pipeline's memory
  pipeline_t(mux::unique_ptr<mux_executable_t> mux_binary_executable,
             mux::unique_ptr<mux_kernel_t> mux_binary_kernel,
             vk::allocator allocator);

  /// @brief Constructor for a derived pipeline created from a base pipeline.
  ///
  /// @param base_pipeline Pipeline to derive from.
  /// @param allocator Allocator used to allocate the pipeline's memory
  pipeline_t(pipeline_t *base_pipeline, vk::allocator allocator);

  /// @brief destructor
  ~pipeline_t();

  /// @brief total size in bytes of the buffer needed for push constants
  uint32_t total_push_constant_size;

  /// @brief Compiler module used to compile the shader. The lifetime of this
  /// object should be greater than `compiler_kernel`.
  std::unique_ptr<compiler::Module> compiler_module;

  /// @brief A reference to the shader stage inside the SPIR-V module compiled
  /// by `compiler_module`.
  compiler::Kernel *compiler_kernel;

  /// @brief Mux executable loaded from a cached binary shader, used to create
  /// `mux_binary_kernel`. If this is a derived pipeline, this will be nullptr.
  mux::unique_ptr<mux_executable_t> mux_binary_executable_storage;

  /// @brief Mux kernel representing the binary shader stage in
  /// `mux_binary_executable`, only used when the pipeline is created from a
  /// cached shader.
  ///
  /// If this is a derived pipeline, then `mux_binary_kernel` will refer to the
  /// kernel owned by the base pipeline, and `mux_binary_kernel_storage` will be
  /// nullptr.
  mux_kernel_t mux_binary_kernel;

  /// @brief Storage for `mux_binary_kernel` if this pipeline owns it. If this
  /// is a derived pipeline, then `mux_binary_kernel_storage` will be nullptr.
  mux::unique_ptr<mux_kernel_t> mux_binary_kernel_storage;

  /// @brief Work group info saved for calculating global size in vkCmdDispatch
  /// and creating derivative pipelines
  std::array<uint32_t, 3> wgs;

  ///@brief List of descriptor set/binding pairs used by the kernel
  vk::small_vector<spirv_ll::DescriptorBinding, 2> descriptor_bindings;
} *pipeline;

/// @brief Internal implementation of vkCreateComputePipelines
///
/// @param device device on which the pipeline is to be created
/// @param pipelineCache optional means to reuse pipeline construction calls,
/// currently unsupported
/// @param createInfoCount length of pCreateInfos, and also the number of
/// pipelines to create
/// @param pCreateInfos array of create info structures
/// @param allocator allocator with which to create the pipelines
/// @param pPipelines return created pipelines
///
/// @return return Vulkan result code
VkResult CreateComputePipelines(vk::device device,
                                vk::pipeline_cache pipelineCache,
                                uint32_t createInfoCount,
                                const VkComputePipelineCreateInfo *pCreateInfos,
                                vk::allocator allocator,
                                VkPipeline *pPipelines);

/// @brief internal implementation of vkDestroyPipeline
///
/// @param device device on which the pipeline was created
/// @param pipeline pipeline to destroy
/// @param allocator allocator the pipeline was created with
void DestroyPipeline(vk::device device, vk::pipeline pipeline,
                     vk::allocator allocator);
}  // namespace vk

#endif  // VK_PIPELINE_H_INCLUDED
