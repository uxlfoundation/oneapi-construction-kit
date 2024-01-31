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

#include <cargo/array_view.h>
#include <cargo/string_view.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_os_ostream.h>
#include <vk/device.h>
#include <vk/error.h>
#include <vk/pipeline.h>
#include <vk/pipeline_cache.h>
#include <vk/pipeline_layout.h>
#include <vk/shader_module.h>
#include <vk/type_traits.h>

#include <cstring>
#include <map>
#include <utility>

namespace vk {
pipeline_t::pipeline_t(std::unique_ptr<compiler::Module> compiler_module,
                       compiler::Kernel *compiler_kernel,
                       vk::allocator allocator)
    : total_push_constant_size(0),
      compiler_module(std::move(compiler_module)),
      compiler_kernel(compiler_kernel),
      mux_binary_executable_storage(nullptr,
                                    {nullptr, {nullptr, nullptr, nullptr}}),
      mux_binary_kernel(nullptr),
      mux_binary_kernel_storage(nullptr,
                                {nullptr, {nullptr, nullptr, nullptr}}),
      descriptor_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

pipeline_t::pipeline_t(mux::unique_ptr<mux_executable_t> mux_binary_executable,
                       mux::unique_ptr<mux_kernel_t> mux_binary_kernel,
                       vk::allocator allocator)
    : total_push_constant_size(0),
      compiler_module(nullptr),
      compiler_kernel(nullptr),
      mux_binary_executable_storage(std::move(mux_binary_executable)),
      mux_binary_kernel(mux_binary_kernel.get()),
      // This will be constructed after `mux_binary_kernel`, so no use after
      // move in the line above.
      mux_binary_kernel_storage(std::move(mux_binary_kernel)),
      descriptor_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

pipeline_t::pipeline_t(pipeline_t *base_pipeline, vk::allocator allocator)
    : total_push_constant_size(0),
      compiler_module(nullptr),
      compiler_kernel(nullptr),
      mux_binary_executable_storage(nullptr,
                                    {nullptr, {nullptr, nullptr, nullptr}}),
      mux_binary_kernel(nullptr),
      mux_binary_kernel_storage(nullptr,
                                {nullptr, {nullptr, nullptr, nullptr}}),
      descriptor_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {
  if (base_pipeline->mux_binary_kernel) {
    mux_binary_kernel = base_pipeline->mux_binary_kernel;
  } else {
    compiler_kernel = base_pipeline->compiler_kernel;
  }
}

pipeline_t::~pipeline_t() {}

VkResult CreateComputePipelines(vk::device device,
                                vk::pipeline_cache pipelineCache,
                                uint32_t createInfoCount,
                                const VkComputePipelineCreateInfo *pCreateInfos,
                                vk::allocator allocator,
                                VkPipeline *pPipelines) {
  VkResult res = VK_SUCCESS;

  for (uint32_t pipelineIndex = 0; pipelineIndex < createInfoCount;
       pipelineIndex++) {
    vk::shader_module shader_module =
        vk::cast<vk::shader_module>(pCreateInfos[pipelineIndex].stage.module);

    vk::pipeline pipeline;

    if (pCreateInfos[pipelineIndex].flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
      // TODO: when providing local workgroup sizes is possible store and reuse
      // the kernel instead of the scheduled_kernel, making this a fast way to
      // switch out local workgroup sizes
      vk::pipeline base_pipeline(nullptr);
      if (pCreateInfos[pipelineIndex].basePipelineHandle != VK_NULL_HANDLE) {
        base_pipeline = vk::cast<vk::pipeline>(
            pCreateInfos[pipelineIndex].basePipelineHandle);
      } else if (pCreateInfos[pipelineIndex].basePipelineIndex >= 0) {
        base_pipeline = vk::cast<vk::pipeline>(
            pPipelines[pCreateInfos[pipelineIndex].basePipelineIndex]);
      }
      VK_ASSERT(nullptr != base_pipeline, "Invalid pipeline state");

      pipeline = allocator.create<vk::pipeline_t>(
          VK_SYSTEM_ALLOCATION_SCOPE_DEVICE, base_pipeline, allocator);
      if (!pipeline) {
        pPipelines[pipelineIndex] = VK_NULL_HANDLE;
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        continue;
      }
    } else {
      const VkSpecializationInfo *spec_info =
          pCreateInfos[pipelineIndex].stage.pSpecializationInfo;

      // Map constant ID to its corresponding offset into spec_data.
      compiler::spirv::SpecializationInfo spvSpecInfo;
      if (spec_info) {
        const uint32_t map_entry_count = spec_info->mapEntryCount;
        size_t dataSize = 0;
        for (uint32_t map_entry_index = 0; map_entry_index < map_entry_count;
             map_entry_index++) {
          const uint32_t id =
              spec_info->pMapEntries[map_entry_index].constantID;
          const uint32_t offset =
              spec_info->pMapEntries[map_entry_index].offset;
          const size_t size = spec_info->pMapEntries[map_entry_index].size;

          spvSpecInfo.entries.insert(std::make_pair(
              id, compiler::spirv::SpecializationInfo::Entry{offset, size}));
          dataSize = (offset + size) > dataSize ? (offset + size) : dataSize;
        }
        spvSpecInfo.data = spec_info->pData;
      }

      mux::unique_ptr<mux_executable_t> mux_binary_executable_ptr(
          nullptr, {nullptr, {nullptr, nullptr, nullptr}});
      mux::unique_ptr<mux_kernel_t> mux_binary_kernel_ptr(
          nullptr, {nullptr, {nullptr, nullptr, nullptr}});

      std::unique_ptr<compiler::Module> compiler_module;
      compiler::Kernel *compiler_kernel;

      std::array<uint32_t, 3> workgroup_size;
      cargo::small_vector<compiler::spirv::DescriptorBinding, 2>
          descriptor_bindings;

      cached_shader *cache_entry_iter = nullptr;

      if (pipelineCache) {
        // Pipeline cache isn't externally synchronized according to the spec.
        {
          const std::lock_guard<std::mutex> lock(pipelineCache->mutex);
          cache_entry_iter = std::find(pipelineCache->cache_entries.begin(),
                                       pipelineCache->cache_entries.end(),
                                       shader_module->module_checksum);
        }
      }

      const cargo::string_view stageName(
          pCreateInfos[pipelineIndex].stage.pName);

      if (cache_entry_iter &&
          cache_entry_iter != pipelineCache->cache_entries.end()) {
        // If the pipeline is cached, create a Mux executable and kernel from
        // the cached binary.
        workgroup_size = cache_entry_iter->workgroup_size;
        if (descriptor_bindings.assign(
                cache_entry_iter->descriptor_bindings.begin(),
                cache_entry_iter->descriptor_bindings.end())) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = VK_ERROR_OUT_OF_HOST_MEMORY;
          continue;
        }

        mux_executable_t mux_binary_executable;
        mux_result_t error = muxCreateExecutable(
            device->mux_device, cache_entry_iter->binary.data(),
            cache_entry_iter->binary.size(), allocator.getMuxAllocator(),
            &mux_binary_executable);

        if (mux_success != error) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = vk::getVkResult(error);
          continue;
        }
        mux_binary_executable_ptr = {
            mux_binary_executable,
            {device->mux_device, allocator.getMuxAllocator()}};

        mux_kernel_t mux_binary_kernel;
        error = muxCreateKernel(
            device->mux_device, mux_binary_executable, stageName.data(),
            stageName.size(), allocator.getMuxAllocator(), &mux_binary_kernel);
        if (mux_success != error) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = vk::getVkResult(error);
          continue;
        }
        mux_binary_kernel_ptr = {
            mux_binary_kernel,
            {device->mux_device, allocator.getMuxAllocator()}};

        pipeline = allocator.create<vk::pipeline_t>(
            VK_SYSTEM_ALLOCATION_SCOPE_DEVICE,
            std::move(mux_binary_executable_ptr),
            std::move(mux_binary_kernel_ptr), allocator);
      } else {
        uint32_t num_errors = 0;
        std::string error_log;
        compiler_module =
            device->compiler_target->createModule(num_errors, error_log);
        if (!compiler_module) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = VK_ERROR_OUT_OF_HOST_MEMORY;
          continue;
        }

        auto compile_result = compiler_module->compileSPIRV(
            {shader_module->code_buffer.data(), shader_module->code_size / 4},
            device->spv_device_info, spvSpecInfo);
        if (!compile_result) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = getVkResult(compile_result.error());
          continue;
        }

        std::vector<builtins::printf::descriptor> printf_calls;
        auto finalize_result = compiler_module->finalize({}, printf_calls);
        if (finalize_result != compiler::Result::SUCCESS) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = getVkResult(finalize_result);
          continue;
        }

        const auto &spirv_module_info = *compile_result;
        if (descriptor_bindings.assign(
                spirv_module_info.used_descriptor_bindings.begin(),
                spirv_module_info.used_descriptor_bindings.end())) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = VK_ERROR_OUT_OF_HOST_MEMORY;
          continue;
        }
        std::sort(descriptor_bindings.begin(), descriptor_bindings.end());
        workgroup_size = spirv_module_info.workgroup_size;

        if (pipelineCache) {
          // we can't use the allocator provided to create the pipeline because
          // this object may outlive the pipeline
          cached_shader shader(device->allocator.getCallbacks(),
                               VK_SYSTEM_ALLOCATION_SCOPE_CACHE);

          shader.source_checksum = shader_module->module_checksum;
          shader.workgroup_size = workgroup_size;
          if (shader.descriptor_bindings.assign(descriptor_bindings.begin(),
                                                descriptor_bindings.end())) {
            pPipelines[pipelineIndex] = VK_NULL_HANDLE;
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            continue;
          }

          cargo::array_view<uint8_t> binary;
          auto binary_result = compiler_module->createBinary(binary);
          if (binary_result != compiler::Result::SUCCESS) {
            pPipelines[pipelineIndex] = VK_NULL_HANDLE;
            res = getVkResult(binary_result);
            continue;
          }

          if (binary.size() > 0) {
            if (cargo::success != shader.binary.resize(binary.size())) {
              pPipelines[pipelineIndex] = VK_NULL_HANDLE;
              res = VK_ERROR_OUT_OF_HOST_MEMORY;
              continue;
            }

            std::memcpy(shader.binary.data(), binary.data(),
                        shader.binary.size());
          }

          // set data size to the combined size of everything that will be
          // copied in the event of a vkGetPipelineCacheData call for later
          // convenience
          shader.data_size =
              sizeof(shader.data_size) + sizeof(shader.source_checksum) +
              // we copy the size into the cache data as well
              sizeof(shader.descriptor_bindings.size()) +
              sizeof(spirv_ll::DescriptorBinding) *
                  shader.descriptor_bindings.size() +
              sizeof(shader.workgroup_size) + sizeof(shader.binary.size()) +
              shader.binary.size();

          {
            const std::lock_guard<std::mutex> lock(pipelineCache->mutex);

            if (std::find(pipelineCache->cache_entries.begin(),
                          pipelineCache->cache_entries.end(),
                          shader) == pipelineCache->cache_entries.end()) {
              if (pipelineCache->cache_entries.push_back(std::move(shader))) {
                pPipelines[pipelineIndex] = VK_NULL_HANDLE;
                res = VK_ERROR_OUT_OF_HOST_MEMORY;
                continue;
              }
            }
          }
        }

        compiler_kernel = compiler_module->getKernel(
            std::string(stageName.data(), stageName.size()));
        if (!compiler_kernel) {
          pPipelines[pipelineIndex] = VK_NULL_HANDLE;
          res = VK_ERROR_INITIALIZATION_FAILED;
          continue;
        }

        // Optimize the kernel for the workgroup size.
        compiler_kernel->precacheLocalSize(workgroup_size[0], workgroup_size[1],
                                           workgroup_size[2]);

        pipeline = allocator.create<vk::pipeline_t>(
            VK_SYSTEM_ALLOCATION_SCOPE_DEVICE, std::move(compiler_module),
            compiler_kernel, allocator);
      }

      if (!pipeline) {
        pPipelines[pipelineIndex] = VK_NULL_HANDLE;
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        continue;
      }

      pipeline->wgs = workgroup_size;

      auto iter = pipeline->descriptor_bindings.insert(
          pipeline->descriptor_bindings.begin(), descriptor_bindings.begin(),
          descriptor_bindings.end());

      if (!iter) {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        continue;
      }
    }

    vk::pipeline_layout pipeline_layout =
        vk::cast<vk::pipeline_layout>(pCreateInfos[pipelineIndex].layout);

    pipeline->total_push_constant_size =
        pipeline_layout->total_push_constant_size;

    pPipelines[pipelineIndex] = reinterpret_cast<VkPipeline>(pipeline);
  }

  return res;
}

void DestroyPipeline(vk::device, vk::pipeline pipeline,
                     vk::allocator allocator) {
  if (pipeline == VK_NULL_HANDLE) {
    return;
  }

  pipeline->mux_binary_kernel_storage.reset();
  pipeline->mux_binary_executable_storage.reset();

  allocator.destroy(pipeline);
}
}  // namespace vk
