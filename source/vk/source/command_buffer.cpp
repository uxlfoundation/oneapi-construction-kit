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

#include <vk/buffer.h>
#include <vk/command_buffer.h>
#include <vk/command_pool.h>
#include <vk/descriptor_set.h>
#include <vk/descriptor_set_layout.h>
#include <vk/device.h>
#include <vk/event.h>
#include <vk/image.h>
#include <vk/pipeline.h>
#include <vk/pipeline_layout.h>
#include <vk/query_pool.h>
#include <vk/type_traits.h>
#include <vk/unique_ptr.h>

#include <cstring>
#include <utility>

namespace vk {
command_buffer_t::command_buffer_t(
    VkCommandPoolCreateFlags command_pool_create_flags, mux_device_t mux_device,
    mux::unique_ptr<mux_command_buffer_t> initial_command_buffer,
    mux::unique_ptr<mux_fence_t> initial_fence,
    mux::unique_ptr<mux_semaphore_t> initial_semaphore, vk::allocator allocator)
    : command_buffer_level(VK_COMMAND_BUFFER_LEVEL_PRIMARY),
      command_pool_create_flags(command_pool_create_flags),
      descriptor_sets(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      state(initial),
      error(VK_SUCCESS),
      allocator(allocator),
      mux_device(mux_device),
      compiler_kernel(nullptr),
      mux_binary_kernel(nullptr),
      push_constant_objects(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      specialized_kernels(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      dispatched_kernels(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      shader_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      commands({allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      main_command_buffer(initial_command_buffer.release()),
      main_fence(initial_fence.release()),
      main_semaphore(initial_semaphore.release()),
      main_command_buffer_stage_flags(0),
      main_command_buffer_event_wait_flags(0),
      main_dispatched(false),
      simultaneous_use_list(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      compute_command_buffer(main_command_buffer),
      compute_stage_flags(&main_command_buffer_stage_flags),
      compute_command_list(&commands),
      transfer_command_buffer(main_command_buffer),
      transfer_stage_flags(&main_command_buffer_stage_flags),
      transfer_command_list(&commands),
      barrier_group_infos(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      wait_events_semaphores(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      push_constant_descriptor_info(),
      descriptor_size_memory_allocs(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      descriptor_size_buffers(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

command_buffer_t::command_buffer_t(
    VkCommandPoolCreateFlags command_pool_create_flags, vk::allocator allocator)
    : command_buffer_level(VK_COMMAND_BUFFER_LEVEL_SECONDARY),
      command_pool_create_flags(command_pool_create_flags),
      descriptor_sets(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      state(initial),
      error(VK_SUCCESS),
      allocator(allocator),
      compiler_kernel(nullptr),
      mux_binary_kernel(nullptr),
      push_constant_objects(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      specialized_kernels(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      dispatched_kernels(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      shader_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      commands({allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      simultaneous_use_list(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      compute_command_list(&commands),
      transfer_command_list(&commands),
      barrier_group_infos(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      wait_events_semaphores(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      push_constant_descriptor_info(),
      descriptor_size_memory_allocs(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      descriptor_size_buffers(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

command_buffer_t::~command_buffer_t() {}

command_buffer_t::recorded_kernel::recorded_kernel(vk::allocator allocator)
    : descriptors(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND}),
      mux_binary_kernel(nullptr),
      specialized_kernel_executable(nullptr,
                                    {nullptr, {nullptr, nullptr, nullptr}}),
      specialized_kernel(nullptr, {nullptr, {nullptr, nullptr, nullptr}}) {}

mux_kernel_t command_buffer_t::recorded_kernel::getMuxKernel() {
  if (mux_binary_kernel) {
    return mux_binary_kernel;
  } else {
    return specialized_kernel.get();
  }
}

mux_ndrange_options_t
command_buffer_t::recorded_kernel::getMuxNDRangeOptions() {
  mux_ndrange_options_t options;
  options.descriptors = descriptors.empty() ? nullptr : descriptors.data();
  options.descriptors_length = descriptors.size();
  options.local_size[0] = local_size[0];
  options.local_size[1] = local_size[1];
  options.local_size[2] = local_size[2];
  options.global_offset = global_offset.data();
  options.global_size = global_size.data();
  options.dimensions = 3;
  return options;
}

VkResult AllocateCommandBuffers(
    vk::device device, const VkCommandBufferAllocateInfo *pAllocateInfo,
    vk::command_buffer *pCommandBuffers) {
  (void)device;

  vk::command_pool command_pool =
      vk::cast<vk::command_pool>(pAllocateInfo->commandPool);

  for (uint32_t commandBufferIndex = 0,
                commandBufferEnd = pAllocateInfo->commandBufferCount;
       commandBufferIndex < commandBufferEnd; commandBufferIndex++) {
    vk::command_buffer command_buffer = nullptr;

    if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pAllocateInfo->level) {
      mux_command_buffer_t initial_command_buffer;

      mux_result_t error = muxCreateCommandBuffer(
          device->mux_device, nullptr,
          command_pool->allocator.getMuxAllocator(), &initial_command_buffer);
      if (mux_success != error) {
        return vk::getVkResult(error);
      }

      mux::unique_ptr<mux_command_buffer_t> initial_command_buffer_ptr(
          initial_command_buffer,
          {device->mux_device, command_pool->allocator.getMuxAllocator()});

      mux_fence_t initial_fence;
      if (auto error = muxCreateFence(device->mux_device,
                                      command_pool->allocator.getMuxAllocator(),
                                      &initial_fence)) {
        return vk::getVkResult(error);
      }

      mux::unique_ptr<mux_fence_t> initial_fence_ptr(
          initial_fence,
          {device->mux_device, command_pool->allocator.getMuxAllocator()});

      mux_semaphore_t initial_semaphore;

      error = muxCreateSemaphore(device->mux_device,
                                 command_pool->allocator.getMuxAllocator(),
                                 &initial_semaphore);
      if (mux_success != error) {
        return vk::getVkResult(error);
      }
      mux::unique_ptr<mux_semaphore_t> initial_semaphore_ptr(
          initial_semaphore,
          {device->mux_device, command_pool->allocator.getMuxAllocator()});

      command_buffer = command_pool->allocator.create<command_buffer_t>(
          VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, command_pool->flags,
          device->mux_device, std::move(initial_command_buffer_ptr),
          std::move(initial_fence_ptr), std::move(initial_semaphore_ptr),
          command_pool->allocator);
      if (!command_buffer) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    } else if (VK_COMMAND_BUFFER_LEVEL_SECONDARY == pAllocateInfo->level) {
      command_buffer = command_pool->allocator.create<command_buffer_t>(
          VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, command_pool->flags,
          command_pool->allocator);
      if (!command_buffer) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }

    if (command_pool->command_buffers.push_back(command_buffer)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    pCommandBuffers[commandBufferIndex] = command_buffer;
  }
  return VK_SUCCESS;
}

void FreeCommandBuffers(vk::device device, vk::command_pool commandPool,
                        uint32_t commandBufferCount,
                        const vk::command_buffer *pCommandBuffers) {
  (void)device;

  vk::small_vector<vk::command_buffer, 4> dead_buffers(
      {commandPool->allocator.getCallbacks(),
       VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

  for (uint32_t commandBufferIndex = 0; commandBufferIndex < commandBufferCount;
       commandBufferIndex++) {
    vk::command_buffer commandBuffer = pCommandBuffers[commandBufferIndex];

    if (commandBuffer == VK_NULL_HANDLE) {
      continue;
    }

    if (VK_COMMAND_BUFFER_LEVEL_PRIMARY ==
        commandBuffer->command_buffer_level) {
      for (auto &barrier_info : commandBuffer->barrier_group_infos) {
        muxDestroyCommandBuffer(device->mux_device,
                                barrier_info->command_buffer,
                                commandPool->allocator.getMuxAllocator());

        muxDestroySemaphore(device->mux_device, barrier_info->semaphore,
                            commandPool->allocator.getMuxAllocator());

        muxDestroyFence(device->mux_device, barrier_info->fence,
                        commandPool->allocator.getMuxAllocator());

        commandBuffer->allocator.destroy(barrier_info);
      }

      for (auto &tuple : commandBuffer->simultaneous_use_list) {
        muxDestroyCommandBuffer(device->mux_device, tuple.command_buffer,
                                commandPool->allocator.getMuxAllocator());

        muxDestroySemaphore(device->mux_device, tuple.semaphore,
                            commandPool->allocator.getMuxAllocator());

        muxDestroyFence(device->mux_device, tuple.fence,
                        commandPool->allocator.getMuxAllocator());
      }

      muxDestroyCommandBuffer(device->mux_device,
                              commandBuffer->main_command_buffer,
                              commandPool->allocator.getMuxAllocator());
      muxDestroyFence(device->mux_device, commandBuffer->main_fence,
                      commandPool->allocator.getMuxAllocator());
      muxDestroySemaphore(device->mux_device, commandBuffer->main_semaphore,
                          commandPool->allocator.getMuxAllocator());

      commandBuffer->dispatched_kernels.clear();
      commandBuffer->specialized_kernels.clear();

      for (auto &buffer_memory_pair : commandBuffer->push_constant_objects) {
        muxFreeMemory(commandBuffer->mux_device, buffer_memory_pair.memory,
                      commandBuffer->allocator.getMuxAllocator());

        muxDestroyBuffer(commandBuffer->mux_device, buffer_memory_pair.buffer,
                         commandBuffer->allocator.getMuxAllocator());
      }

      for (auto &alloc : commandBuffer->descriptor_size_memory_allocs) {
        muxFreeMemory(commandBuffer->mux_device, alloc,
                      commandBuffer->allocator.getMuxAllocator());
      }

      for (auto &buffer : commandBuffer->descriptor_size_buffers) {
        muxDestroyBuffer(commandBuffer->mux_device, buffer,
                         commandBuffer->allocator.getMuxAllocator());
      }
    } else if (commandBuffer->command_buffer_level ==
               VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      // only secondary command buffers record state commands into the command
      // list
      std::for_each(commandBuffer->commands.begin(),
                    commandBuffer->commands.end(),
                    [&commandBuffer](command_info &c) {
                      if (c.type == command_type_bind_descriptorset) {
                        commandBuffer->allocator.free(static_cast<void *>(
                            c.bind_descriptorset_command.pDescriptorSets));
                      }
                    });
    }

    auto cb_iter = std::find(commandPool->command_buffers.begin(),
                             commandPool->command_buffers.end(), commandBuffer);

    if (cb_iter != commandPool->command_buffers.end()) {
      commandPool->allocator.destroy(commandBuffer);
      if (dead_buffers.push_back(*cb_iter)) {
        return;
      }
    } else {
      VK_ABORT("Command buffer was not allocated from provided command pool!")
    }
  }

  for (auto command_buffer : dead_buffers) {
    auto erase_iter =
        std::find(commandPool->command_buffers.begin(),
                  commandPool->command_buffers.end(), command_buffer);
    commandPool->command_buffers.erase(erase_iter);
  }
}

VkResult ResetCommandBuffer(vk::command_buffer commandBuffer,
                            VkCommandBufferResetFlags flags) {
  if (flags & VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT) {
    commandBuffer->descriptor_sets.clear();
    for (auto &alloc : commandBuffer->descriptor_size_memory_allocs) {
      muxFreeMemory(commandBuffer->mux_device, alloc,
                    commandBuffer->allocator.getMuxAllocator());
    }
    commandBuffer->descriptor_size_memory_allocs.clear();

    for (auto &buffer : commandBuffer->descriptor_size_buffers) {
      muxDestroyBuffer(commandBuffer->mux_device, buffer,
                       commandBuffer->allocator.getMuxAllocator());
    }
    commandBuffer->descriptor_size_buffers.clear();
  }

  commandBuffer->error = VK_SUCCESS;
  commandBuffer->compute_command_buffer = commandBuffer->main_command_buffer;
  commandBuffer->transfer_command_buffer = commandBuffer->main_command_buffer;
  commandBuffer->main_command_buffer_stage_flags = 0;
  commandBuffer->main_command_buffer_event_wait_flags = 0;
  commandBuffer->main_dispatched = false;
  commandBuffer->compute_stage_flags =
      &commandBuffer->main_command_buffer_stage_flags;
  commandBuffer->transfer_stage_flags =
      &commandBuffer->main_command_buffer_stage_flags;
  commandBuffer->wgs = {{0, 0, 0}};
  commandBuffer->commands.clear();
  commandBuffer->compiler_kernel = nullptr;
  commandBuffer->mux_binary_kernel = nullptr;
  commandBuffer->push_constant_descriptor_info = {};
  commandBuffer->total_push_constant_size = 0;
  commandBuffer->shader_bindings.clear();
  commandBuffer->state = command_buffer_t::initial;
  commandBuffer->wait_events_semaphores.clear();

  for (auto &command_buffer_info : commandBuffer->barrier_group_infos) {
    muxDestroyCommandBuffer(commandBuffer->mux_device,
                            command_buffer_info->command_buffer,
                            commandBuffer->allocator.getMuxAllocator());
    muxDestroySemaphore(commandBuffer->mux_device,
                        command_buffer_info->semaphore,
                        commandBuffer->allocator.getMuxAllocator());
    muxDestroyFence(commandBuffer->mux_device, command_buffer_info->fence,
                    commandBuffer->allocator.getMuxAllocator());
  }

  commandBuffer->barrier_group_infos.clear();

  // only primary command buffers get mux command buffers/semaphores
  if (commandBuffer->command_buffer_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    if (auto error = muxResetSemaphore(commandBuffer->main_semaphore)) {
      return vk::getVkResult(error);
    }

    if (auto error =
            muxResetCommandBuffer(commandBuffer->main_command_buffer)) {
      return vk::getVkResult(error);
    }
    // while only secondary command buffers record state commands into the
    // command list
  } else if (commandBuffer->command_buffer_level ==
             VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
    std::for_each(commandBuffer->commands.begin(),
                  commandBuffer->commands.end(),
                  [&commandBuffer](command_info &c) {
                    if (c.type == command_type_bind_descriptorset) {
                      commandBuffer->allocator.free(static_cast<void *>(
                          c.bind_descriptorset_command.pDescriptorSets));
                    }
                  });
  }

  commandBuffer->dispatched_kernels.clear();
  commandBuffer->specialized_kernels.clear();
  for (auto &buffer_memory_pair : commandBuffer->push_constant_objects) {
    muxFreeMemory(commandBuffer->mux_device, buffer_memory_pair.memory,
                  commandBuffer->allocator.getMuxAllocator());

    muxDestroyBuffer(commandBuffer->mux_device, buffer_memory_pair.buffer,
                     commandBuffer->allocator.getMuxAllocator());
  }
  commandBuffer->push_constant_objects.clear();

  return VK_SUCCESS;
}

VkResult BeginCommandBuffer(vk::command_buffer commandBuffer,
                            const VkCommandBufferBeginInfo *pBeginInfo) {
  if (commandBuffer->command_pool_create_flags &
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) {
    if (auto error = ResetCommandBuffer(
            commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)) {
      return error;
    }
  }

  commandBuffer->usage_flags = pBeginInfo->flags;

  commandBuffer->state = command_buffer_t::recording;
  // TODO: support command buffer inheritance

  return VK_SUCCESS;
}

VkResult EndCommandBuffer(vk::command_buffer commandBuffer) {
  if (VK_SUCCESS != commandBuffer->error) {
    return commandBuffer->error;
  }

  commandBuffer->state = command_buffer_t::executable;

  return VK_SUCCESS;
}

void CmdCopyBuffer(vk::command_buffer commandBuffer, vk::buffer srcBuffer,
                   vk::buffer dstBuffer, uint32_t regionCount,
                   const VkBufferCopy *pRegions) {
  if (command_buffer_t::pending == commandBuffer->state ||
      command_buffer_t::resolving == commandBuffer->state) {
    vk::small_vector<mux_buffer_region_info_t, 2> muxRegions(
        {commandBuffer->allocator.getCallbacks(),
         VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});
    for (uint32_t regionIndex = 0; regionIndex < regionCount; regionIndex++) {
      // These need casting because the implicit cast from VkDeviceSize ->
      // size_t doesn't work on 32 bit.
      size_t size = static_cast<size_t>(pRegions[regionIndex].size);
      size_t srcOffset = static_cast<size_t>(pRegions[regionIndex].srcOffset);
      size_t dstOffset = static_cast<size_t>(pRegions[regionIndex].dstOffset);
      if (muxRegions.push_back({{size, 1, 1},
                                {srcOffset, 0, 0},
                                {dstOffset, 0, 0},
                                {size, 1},
                                {size, 1}})) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }
    }
    if (auto error = muxCommandCopyBufferRegions(
            commandBuffer->transfer_command_buffer, srcBuffer->mux_buffer,
            dstBuffer->mux_buffer, muxRegions.data(), muxRegions.size(), 0,
            nullptr, nullptr)) {
      commandBuffer->error = vk::getVkResult(error);
      return;
    }

    *commandBuffer->transfer_stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (command_buffer_t::recording == commandBuffer->state ||
             VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level) {
    vk::command_info_copy_buffer command = {};
    command.srcBuffer = srcBuffer;
    command.dstBuffer = dstBuffer;
    command.regionCount = regionCount;
    command.pRegions = pRegions;

    if (commandBuffer->transfer_command_list->push_back(
            vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdUpdateBuffer(vk::command_buffer commandBuffer, vk::buffer dstBuffer,
                     VkDeviceSize dstOffset, VkDeviceSize dataSize,
                     const void *pData) {
  if (command_buffer_t::pending == commandBuffer->state ||
      command_buffer_t::resolving == commandBuffer->state) {
    if (auto error = muxCommandWriteBuffer(
            commandBuffer->transfer_command_buffer, dstBuffer->mux_buffer,
            dstOffset, pData, dataSize, 0, nullptr, nullptr)) {
      commandBuffer->error = vk::getVkResult(error);
    }
    *commandBuffer->transfer_stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (command_buffer_t::recording == commandBuffer->state ||
             VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level) {
    vk::command_info_update_buffer command = {};
    command.dstBuffer = dstBuffer;
    command.dstOffset = dstOffset;
    command.dataSize = dataSize;
    command.pData = pData;

    if (commandBuffer->transfer_command_list->push_back(
            vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdFillBuffer(vk::command_buffer commandBuffer, vk::buffer dstBuffer,
                   VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
  if (VK_WHOLE_SIZE == size) {
    size = dstBuffer->mux_buffer->memory_requirements.size;
  }

  if (command_buffer_t::pending == commandBuffer->state ||
      command_buffer_t::resolving == commandBuffer->state) {
    if (auto error = muxCommandFillBuffer(
            commandBuffer->transfer_command_buffer, dstBuffer->mux_buffer,
            dstOffset, size, static_cast<void *>(&data), 4, 0, nullptr,
            nullptr)) {
      commandBuffer->error = vk::getVkResult(error);
    }
    *commandBuffer->transfer_stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (command_buffer_t::recording == commandBuffer->state ||
             VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level) {
    vk::command_info_fill_buffer command = {};
    command.dstBuffer = dstBuffer;
    command.dstOffset = dstOffset;
    command.size = size;
    command.data = data;

    if (commandBuffer->transfer_command_list->push_back(
            vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdBindPipeline(vk::command_buffer commandBuffer,
                     VkPipelineBindPoint pipelineBindPoint,
                     vk::pipeline pipeline) {
  (void)pipelineBindPoint;
  if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == commandBuffer->command_buffer_level) {
    commandBuffer->compiler_kernel = pipeline->compiler_kernel;
    commandBuffer->mux_binary_kernel = pipeline->mux_binary_kernel;

    commandBuffer->wgs = pipeline->wgs;

    commandBuffer->shader_bindings.clear();

    auto iter = commandBuffer->shader_bindings.insert(
        commandBuffer->shader_bindings.begin(),
        pipeline->descriptor_bindings.begin(),
        pipeline->descriptor_bindings.end());

    if (!iter) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }

    // This controls the size of the mux buffer that gets allocated and copied
    // into during vkCmdDispatch. As only one compute pipeline can be bound at a
    // time, it's safe to simply update the current push constant size.
    commandBuffer->total_push_constant_size =
        pipeline->total_push_constant_size;
  } else if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
             commandBuffer->command_buffer_level) {
    vk::command_info_bind_pipeline command = {};
    command.pipeline = pipeline;
    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdBindDescriptorSets(vk::command_buffer commandBuffer,
                           VkPipelineBindPoint pipelineBindPoint,
                           vk::pipeline_layout layout, uint32_t firstSet,
                           uint32_t descriptorSetCount,
                           const VkDescriptorSet *pDescriptorSets,
                           uint32_t dynamicOffsetCount,
                           const uint32_t *pDynamicOffsets) {
  (void)layout;
  (void)pipelineBindPoint;
  if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == commandBuffer->command_buffer_level) {
    // prevent the resize from truncating some already bound descriptor sets if
    // low sets are bound after high ones (e.g. binding sets 0 and 1 after sets
    // 2 and 3 would result in the size being cut down to 2 and the loss of sets
    // 2 and 3)
    if (firstSet + descriptorSetCount > commandBuffer->descriptor_sets.size()) {
      if (cargo::success != commandBuffer->descriptor_sets.resize(
                                firstSet + descriptorSetCount)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }

    if (pDynamicOffsets) {
      int dynamicOffsetIndex = 0;
      for (uint32_t descriptorSetIndex = 0;
           descriptorSetIndex < descriptorSetCount; descriptorSetIndex++) {
        vk::descriptor_set set =
            vk::cast<vk::descriptor_set>(pDescriptorSets[descriptorSetIndex]);
        for (vk::descriptor_binding &binding : set->descriptor_bindings) {
          if (binding->dynamic) {
            for (uint32_t descriptorIndex = 0;
                 descriptorIndex < binding->descriptor_count;
                 descriptorIndex++) {
              binding->descriptors[descriptorIndex].buffer_descriptor.offset +=
                  pDynamicOffsets[dynamicOffsetIndex];
              dynamicOffsetIndex++;
            }
          }
        }
      }
    }

    for (uint32_t descriptorSetIndex = 0;
         descriptorSetIndex < descriptorSetCount; descriptorSetIndex++) {
      commandBuffer->descriptor_sets[firstSet + descriptorSetIndex] =
          vk::cast<vk::descriptor_set>(pDescriptorSets[descriptorSetIndex]);
    }
  } else if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
             commandBuffer->command_buffer_level) {
    vk::command_info_bind_descriptorset command = {};
    command.layout = layout;
    command.firstSet = firstSet;
    command.descriptorSetCount = descriptorSetCount;
    command.dynamicOffsetCount = dynamicOffsetCount;
    command.pDynamicOffsets = pDynamicOffsets;

    // the validation layers invalidate pDescriptorSets, so we need to copy the
    // list of handles into our command info
    VkDescriptorSet *descriptorSetsCopy =
        static_cast<VkDescriptorSet *>(commandBuffer->allocator.alloc(
            sizeof(VkDescriptorSet) * descriptorSetCount,
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT));

    for (uint32_t dSetIndex = 0; dSetIndex < descriptorSetCount; dSetIndex++) {
      descriptorSetsCopy[dSetIndex] = pDescriptorSets[dSetIndex];
    }

    command.pDescriptorSets = descriptorSetsCopy;

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdDispatch(vk::command_buffer commandBuffer, uint32_t x, uint32_t y,
                 uint32_t z) {
  if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
      commandBuffer->command_buffer_level) {
    const vk::command_info_dispatch command = {x, y, z};
    if (commandBuffer->compute_command_list->push_back(
            vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  } else if (vk::command_buffer_t::recording == commandBuffer->state) {
    vk::small_vector<mux_descriptor_info_t, 4> descriptors(
        {commandBuffer->allocator.getCallbacks(),
         VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

    vk::small_vector<uint32_t, 4> buffer_binding_sizes(
        {commandBuffer->allocator.getCallbacks(),
         VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

    for (auto &set : commandBuffer->shader_bindings) {
      auto &binding = commandBuffer->descriptor_sets[set.set]
                          ->descriptor_bindings[set.binding];
      for (uint32_t descriptorIndex = 0;
           descriptorIndex < binding->descriptor_count; descriptorIndex++) {
        if (binding->descriptors[descriptorIndex].type ==
            mux_descriptor_info_type_buffer) {
          if (buffer_binding_sizes.push_back(
                  binding->descriptors[descriptorIndex]
                      .buffer_descriptor.buffer->memory_requirements.size)) {
            commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
            return;
          }
        }

        if (descriptors.push_back(binding->descriptors[descriptorIndex])) {
          commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
          return;
        }
      }
    }

    if (commandBuffer->total_push_constant_size) {
      // create push constant buffer
      mux_buffer_t push_constant_buffer;
      mux_result_t error = muxCreateBuffer(
          commandBuffer->mux_device, commandBuffer->total_push_constant_size,
          commandBuffer->allocator.getMuxAllocator(), &push_constant_buffer);
      if (error != mux_success) {
        commandBuffer->error = getVkResult(error);
        return;
      }
      mux::unique_ptr<mux_buffer_t> push_constant_buffer_ptr(
          push_constant_buffer, {commandBuffer->mux_device,
                                 commandBuffer->allocator.getMuxAllocator()});

      // our push constant memory needs to be host visible, and we need to know
      // whether future writes to it will need flushing
      uint32_t memory_properties = mux_memory_property_host_visible;

      if (commandBuffer->mux_device->info->allocation_capabilities &
          mux_allocation_capabilities_coherent_host) {
        memory_properties |= mux_memory_property_host_coherent;
      } else if (commandBuffer->mux_device->info->allocation_capabilities &
                 mux_allocation_capabilities_cached_host) {
        memory_properties |= mux_memory_property_host_cached;
      }

      mux_memory_t push_constant_memory;
      error = muxAllocateMemory(
          commandBuffer->mux_device, commandBuffer->total_push_constant_size, 1,
          memory_properties, mux_allocation_type_alloc_device, 0,
          commandBuffer->allocator.getMuxAllocator(), &push_constant_memory);

      if (error != mux_success) {
        commandBuffer->error = getVkResult(error);
        return;
      }
      mux::unique_ptr<mux_memory_t> push_constant_memory_ptr(
          push_constant_memory, {commandBuffer->mux_device,
                                 commandBuffer->allocator.getMuxAllocator()});

      error =
          muxBindBufferMemory(commandBuffer->mux_device, push_constant_memory,
                              push_constant_buffer, 0);
      if (error != mux_success) {
        commandBuffer->error = getVkResult(error);
        return;
      }

      const buffer_memory_pair push_constant_buffer_pair = {
          push_constant_buffer, push_constant_memory};
      if (commandBuffer->push_constant_objects.push_back(
              push_constant_buffer_pair)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }

      // Now that 'commandBuffer->push_constant_objects' is populated, the
      // command buffer is responsible for the lifetime of these objects, so
      // avoid a possible double free by releasing the unique_ptr's.
      (void)push_constant_buffer_ptr.release();
      (void)push_constant_memory_ptr.release();

      // map memory and copy
      void *push_constant_mapped_memory;
      error = muxMapMemory(commandBuffer->mux_device, push_constant_memory, 0,
                           commandBuffer->total_push_constant_size,
                           &push_constant_mapped_memory);
      if (error != mux_success) {
        commandBuffer->error = getVkResult(error);
        return;
      }

      std::memcpy(push_constant_mapped_memory,
                  commandBuffer->push_constants.data(),
                  commandBuffer->total_push_constant_size);

      if (push_constant_memory->properties & mux_memory_property_host_cached) {
        error = muxFlushMappedMemoryToDevice(
            commandBuffer->mux_device, push_constant_memory, 0,
            commandBuffer->total_push_constant_size);
        if (error != mux_success) {
          commandBuffer->error = getVkResult(error);
        }
      }

      error = muxUnmapMemory(commandBuffer->mux_device, push_constant_memory);
      if (error != mux_success) {
        commandBuffer->error = getVkResult(error);
        return;
      }

      // add descriptor
      mux_descriptor_info_s push_constant_descriptor_info = {};
      mux_descriptor_info_buffer_s push_constant_buffer_info = {};

      push_constant_buffer_info.buffer = push_constant_buffer;
      push_constant_buffer_info.offset = 0;

      push_constant_descriptor_info.type = mux_descriptor_info_type_buffer;
      push_constant_descriptor_info.buffer_descriptor =
          push_constant_buffer_info;

      if (descriptors.push_back(push_constant_descriptor_info)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }
    }

    if (!buffer_binding_sizes.empty()) {
      // the allocation needs to be host visible and we need to know whether we
      // should flush
      uint32_t memory_properties = mux_memory_property_host_visible;

      if (commandBuffer->mux_device->info->allocation_capabilities &
          mux_allocation_capabilities_coherent_host) {
        memory_properties |= mux_memory_property_host_coherent;
      } else if (commandBuffer->mux_device->info->allocation_capabilities &
                 mux_allocation_capabilities_cached_host) {
        memory_properties |= mux_memory_property_host_cached;
      }

      mux_memory_t descriptor_size_memory;
      mux_buffer_t descriptor_size_buffer;

      muxAllocateMemory(commandBuffer->mux_device,
                        buffer_binding_sizes.size() * sizeof(uint32_t), 1,
                        memory_properties, mux_allocation_type_alloc_device, 0,
                        commandBuffer->allocator.getMuxAllocator(),
                        &descriptor_size_memory);

      muxCreateBuffer(commandBuffer->mux_device,
                      buffer_binding_sizes.size() * sizeof(uint32_t),
                      commandBuffer->allocator.getMuxAllocator(),
                      &descriptor_size_buffer);

      muxBindBufferMemory(commandBuffer->mux_device, descriptor_size_memory,
                          descriptor_size_buffer, 0);

      // copy the sizes of all the buffers bound in the shader into a buffer
      // in device memory
      void *mapped_memory;

      muxMapMemory(commandBuffer->mux_device, descriptor_size_memory, 0,
                   buffer_binding_sizes.size() * sizeof(uint32_t),
                   &mapped_memory);

      std::memcpy(mapped_memory, buffer_binding_sizes.data(),
                  buffer_binding_sizes.size() * sizeof(uint32_t));

      if (memory_properties & mux_memory_property_host_cached) {
        if (auto error = muxFlushMappedMemoryToDevice(
                commandBuffer->mux_device, descriptor_size_memory, 0,
                buffer_binding_sizes.size() * sizeof(uint32_t))) {
          commandBuffer->error = vk::getVkResult(error);
        }
      }

      muxUnmapMemory(commandBuffer->mux_device, descriptor_size_memory);

      // prepare the mux_descriptor_info for the buffer size array and push
      // it to the descriptor list
      commandBuffer->descriptor_size_descriptor_info.type =
          mux_descriptor_info_type_buffer;
      commandBuffer->descriptor_size_descriptor_info.buffer_descriptor.buffer =
          descriptor_size_buffer;
      commandBuffer->descriptor_size_descriptor_info.buffer_descriptor.offset =
          0;

      if (descriptors.push_back(
              commandBuffer->descriptor_size_descriptor_info)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }

      if (commandBuffer->descriptor_size_memory_allocs.push_back(
              descriptor_size_memory)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }

      if (commandBuffer->descriptor_size_buffers.push_back(
              descriptor_size_buffer)) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }
    }

    command_buffer_t::recorded_kernel recorded_kernel(commandBuffer->allocator);
    recorded_kernel.descriptors = std::move(descriptors);
    recorded_kernel.local_size = {commandBuffer->wgs[0], commandBuffer->wgs[1],
                                  commandBuffer->wgs[2]};
    recorded_kernel.global_offset = {0, 0, 0};
    recorded_kernel.global_size = {(size_t)x * (size_t)commandBuffer->wgs[0],
                                   (size_t)y * (size_t)commandBuffer->wgs[1],
                                   (size_t)z * (size_t)commandBuffer->wgs[2]};
    if (commandBuffer->mux_binary_kernel) {
      recorded_kernel.mux_binary_kernel = commandBuffer->mux_binary_kernel;
    } else {
      auto specialized_kernel =
          commandBuffer->compiler_kernel->createSpecializedKernel(
              recorded_kernel.getMuxNDRangeOptions());
      if (!specialized_kernel) {
        commandBuffer->error = vk::getVkResult(specialized_kernel.error());
        return;
      }

      // Create a mux executable and kernel that contains this specialized
      // binary.
      mux_result_t result;
      mux_executable_t mux_executable;
      mux_kernel_t mux_kernel;
      result = muxCreateExecutable(
          commandBuffer->mux_device, specialized_kernel->data(),
          specialized_kernel->size(),
          commandBuffer->allocator.getMuxAllocator(), &mux_executable);
      if (result != mux_success) {
        if (result == mux_error_out_of_memory) {
          commandBuffer->error =
              vk::getVkResult(compiler::Result::OUT_OF_MEMORY);
        } else {
          commandBuffer->error =
              vk::getVkResult(compiler::Result::FINALIZE_PROGRAM_FAILURE);
        }
        return;
      }

      mux::unique_ptr<mux_executable_t> mux_executable_ptr =
          mux::unique_ptr<mux_executable_t>(
              mux_executable, {commandBuffer->mux_device,
                               commandBuffer->allocator.getMuxAllocator()});

      result = muxCreateKernel(commandBuffer->mux_device, mux_executable,
                               commandBuffer->compiler_kernel->name.data(),
                               commandBuffer->compiler_kernel->name.size(),
                               commandBuffer->allocator.getMuxAllocator(),
                               &mux_kernel);
      if (result != mux_success) {
        if (result == mux_error_out_of_memory) {
          commandBuffer->error =
              vk::getVkResult(compiler::Result::OUT_OF_MEMORY);
        } else {
          commandBuffer->error =
              vk::getVkResult(compiler::Result::FINALIZE_PROGRAM_FAILURE);
        }
        return;
      }

      mux::unique_ptr<mux_kernel_t> mux_kernel_ptr =
          mux::unique_ptr<mux_kernel_t>(
              mux_kernel, {commandBuffer->mux_device,
                           commandBuffer->allocator.getMuxAllocator()});

      recorded_kernel.specialized_kernel_executable =
          std::move(mux_executable_ptr);
      recorded_kernel.specialized_kernel = std::move(mux_kernel_ptr);
    }

    if (commandBuffer->specialized_kernels.push_back(
            std::move(recorded_kernel))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // need to push the command here so that it gets executed in the submit
    const vk::command_info_dispatch command = {x, y, z};
    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  } else {
    // Take the next kernel off the kernel list
    auto &specialized_kernel = *commandBuffer->specialized_kernels.begin();
    if (command_buffer_t::pending == commandBuffer->state) {
      if (auto error = muxCommandNDRange(
              commandBuffer->compute_command_buffer,
              specialized_kernel.getMuxKernel(),
              specialized_kernel.getMuxNDRangeOptions(), 0, nullptr, nullptr)) {
        commandBuffer->error = vk::getVkResult(error);
      }
    } else if (command_buffer_t::resolving == commandBuffer->state) {
      if (auto error = muxCommandNDRange(
              commandBuffer->barrier_group_infos.back()->command_buffer,
              specialized_kernel.getMuxKernel(),
              specialized_kernel.getMuxNDRangeOptions(), 0, nullptr, nullptr)) {
        commandBuffer->error = vk::getVkResult(error);
      }
    }

    *commandBuffer->compute_stage_flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    // now that the range is enqueued add the kernel to the executing kernels
    // list
    if (commandBuffer->dispatched_kernels.push_back(
            std::move(specialized_kernel))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Erase the moved-from kernel.
    commandBuffer->specialized_kernels.erase(
        commandBuffer->specialized_kernels.begin());
  }
}

void ExecuteCommand(vk::command_buffer commandBuffer,
                    const vk::command_info &command_info) {
  switch (command_info.type) {
    case vk::command_type_bind_pipeline:
      vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          command_info.bind_pipeline_command.pipeline);
      break;
    case vk::command_type_bind_descriptorset:
      vk::CmdBindDescriptorSets(
          commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          command_info.bind_descriptorset_command.layout,
          command_info.bind_descriptorset_command.firstSet,
          command_info.bind_descriptorset_command.descriptorSetCount,
          command_info.bind_descriptorset_command.pDescriptorSets,
          command_info.bind_descriptorset_command.dynamicOffsetCount,
          command_info.bind_descriptorset_command.pDynamicOffsets);
      break;
    case vk::command_type_dispatch:
      vk::CmdDispatch(commandBuffer, command_info.dispatch_command.x,
                      command_info.dispatch_command.y,
                      command_info.dispatch_command.z);
      break;
    case vk::command_type_dispatch_indirect:
      break;
    case vk::command_type_copy_buffer:
      vk::CmdCopyBuffer(commandBuffer,
                        command_info.copy_buffer_command.srcBuffer,
                        command_info.copy_buffer_command.dstBuffer,
                        command_info.copy_buffer_command.regionCount,
                        command_info.copy_buffer_command.pRegions);
      break;
    case vk::command_type_update_buffer:
      vk::CmdUpdateBuffer(commandBuffer,
                          command_info.update_buffer_command.dstBuffer,
                          command_info.update_buffer_command.dstOffset,
                          command_info.update_buffer_command.dataSize,
                          command_info.update_buffer_command.pData);
      break;
    case vk::command_type_fill_buffer:
      vk::CmdFillBuffer(commandBuffer,
                        command_info.fill_buffer_command.dstBuffer,
                        command_info.fill_buffer_command.dstOffset,
                        command_info.fill_buffer_command.size,
                        command_info.fill_buffer_command.data);
      break;
    case vk::command_type_set_event:
      vk::CmdSetEvent(commandBuffer, command_info.set_event_command.event,
                      command_info.set_event_command.stageMask);
      break;
    case vk::command_type_reset_event:
      vk::CmdResetEvent(commandBuffer, command_info.reset_event_command.event,
                        command_info.reset_event_command.stageMask);
      break;
    case vk::command_type_wait_events:
      vk::CmdWaitEvents(
          commandBuffer, command_info.wait_events_command.eventCount,
          command_info.wait_events_command.pEvents,
          command_info.wait_events_command.srcStageMask,
          command_info.wait_events_command.dstStageMask,
          command_info.wait_events_command.memoryBarrierCount,
          command_info.wait_events_command.pMemoryBarriers,
          command_info.wait_events_command.bufferMemoryBarrierCount,
          command_info.wait_events_command.pBufferMemoryBarriers,
          command_info.wait_events_command.imageMemoryBarrierCount,
          command_info.wait_events_command.pImageMemoryBarriers);
      break;
    case vk::command_type_push_constants:
      vk::CmdPushConstants(commandBuffer,
                           command_info.push_constants_command.pipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           command_info.push_constants_command.offset,
                           command_info.push_constants_command.size,
                           command_info.push_constants_command.pValues);
      break;
    case vk::command_type_pipeline_barrier:
      vk::CmdPipelineBarrier(
          commandBuffer, command_info.pipeline_barrier_command.srcStageMask,
          command_info.pipeline_barrier_command.dstStageMask,
          command_info.pipeline_barrier_command.dependencyFlags,
          command_info.pipeline_barrier_command.memoryBarrierCount,
          command_info.pipeline_barrier_command.pMemoryBarriers,
          command_info.pipeline_barrier_command.bufferMemoryBarrierCount,
          command_info.pipeline_barrier_command.pBufferMemoryBarriers,
          command_info.pipeline_barrier_command.imageMemoryBarrierCount,
          command_info.pipeline_barrier_command.pImageMemoryBarriers);
      break;
  }
}

void CmdExecuteCommands(vk::command_buffer commandBuffer,
                        uint32_t commandBufferCount,
                        const vk::command_buffer *pCommandBuffers) {
  for (uint32_t commandBufferIndex = 0; commandBufferIndex < commandBufferCount;
       commandBufferIndex++) {
    for (const vk::command_info &command_info :
         pCommandBuffers[commandBufferIndex]->commands) {
      ExecuteCommand(commandBuffer, command_info);
    }
  }
}

inline static barrier_group_info *find_barrier_info(
    mux_command_buffer_t mux_command_buffer,
    vk::command_buffer command_buffer) {
  auto barrier_iter =
      std::find_if(command_buffer->barrier_group_infos.begin(),
                   command_buffer->barrier_group_infos.end(),
                   [&mux_command_buffer](barrier_group_info &info) {
                     return info->command_buffer == mux_command_buffer;
                   });

  VK_ASSERT(barrier_iter != command_buffer->barrier_group_infos.end(),
            "Invalid command buffer when searching for barrier info")

  return barrier_iter;
}

void CmdSetEvent(vk::command_buffer commandBuffer, vk::event event,
                 VkPipelineStageFlags stageMask) {
  if (command_buffer_t::pending == commandBuffer->state ||
      command_buffer_t::resolving == commandBuffer->state) {
    mux_command_buffer_t command_buffer = nullptr;
    VkPipelineStageFlags *stage_flags = nullptr;

    // we need to figure out which mux command buffer to push the barrier and
    // user callback to, and which stage flags to OR with stageMask
    if (commandBuffer->barrier_group_infos.empty()) {
      // if there's never been a barrier there is only one mux command buffer
      command_buffer = commandBuffer->main_command_buffer;
      stage_flags = &commandBuffer->main_command_buffer_event_wait_flags;
    } else if (commandBuffer->transfer_command_buffer ==
               commandBuffer->compute_command_buffer) {
      // if there has been a barrier who's mux command buffer now takes all
      // commands (a common use case) again there is only one choice
      command_buffer = commandBuffer->compute_command_buffer;

      auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);
      stage_flags = &(*barrier_iter)->user_wait_flags;
    } else {
      // otherwise there have been multiple barriers with different stages and
      // figuring this out gets a bit more involved
      if ((stageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
           stageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) ||
          stageMask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        if (commandBuffer->compute_command_buffer ==
                commandBuffer->main_command_buffer ||
            commandBuffer->transfer_command_buffer ==
                commandBuffer->main_command_buffer) {
          // if we're waiting on all stages and one of the groups is still main
          // use the barrier group as it will be dispatched later, and thus able
          // to wait on main
          command_buffer = commandBuffer->compute_command_buffer ==
                                   commandBuffer->main_command_buffer
                               ? commandBuffer->transfer_command_buffer
                               : commandBuffer->compute_command_buffer;

          auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);

          stage_flags = &(*barrier_iter)->user_wait_flags;
        } else {
          // if both mux command buffers belong to different barriers use the
          // one that's latest in the barrier list
          auto compute_barrier_iter = find_barrier_info(
              commandBuffer->compute_command_buffer, commandBuffer);

          auto transfer_barrier_iter = find_barrier_info(
              commandBuffer->transfer_command_buffer, commandBuffer);

          auto barrier_iter =
              std::max(transfer_barrier_iter, compute_barrier_iter);

          command_buffer = (*barrier_iter)->command_buffer;
          stage_flags = &(*barrier_iter)->user_wait_flags;
        }
      } else {
        // we're only waiting for one stage
        if (stageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
          command_buffer = commandBuffer->compute_command_buffer;
        } else if (stageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
          command_buffer = commandBuffer->transfer_command_buffer;
        }

        if (command_buffer == commandBuffer->main_command_buffer) {
          stage_flags = &commandBuffer->main_command_buffer_event_wait_flags;
        } else {
          auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);
          stage_flags = &(*barrier_iter)->user_wait_flags;
        }
      }
    }

    if (auto error = muxCommandUserCallback(command_buffer, setEventCallback,
                                            event, 0, nullptr, nullptr)) {
      commandBuffer->error = getVkResult(error);
      return;
    }

    *stage_flags |= stageMask;
    // set event set stage so future wait events commands know when this event
    // is to be set, and thus whether to inlude it in their wait list
    event->set_stage = stageMask;
  } else if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level ||
             command_buffer_t::recording == commandBuffer->state) {
    vk::command_info_set_event command = {};
    command.event = event;
    command.stageMask = stageMask;

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdResetEvent(vk::command_buffer commandBuffer, vk::event event,
                   VkPipelineStageFlags stageMask) {
  if (command_buffer_t::pending == commandBuffer->state ||
      command_buffer_t::resolving == commandBuffer->state) {
    mux_command_buffer_t command_buffer = nullptr;
    VkPipelineStageFlags *stage_flags = nullptr;

    // we need to figure out which mux command buffer to push the barrier and
    // user callback to, and which stage flags to OR with stageMask
    if (commandBuffer->barrier_group_infos.empty()) {
      // if there's never been a barrier there is only one mux command buffer
      command_buffer = commandBuffer->main_command_buffer;
      stage_flags = &commandBuffer->main_command_buffer_event_wait_flags;
    } else if (commandBuffer->transfer_command_buffer ==
               commandBuffer->compute_command_buffer) {
      // if there has been a barrier who's mux command buffer now takes all
      // commands (a common use case) again there is only one choice
      command_buffer = commandBuffer->compute_command_buffer;

      auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);

      stage_flags = &(*barrier_iter)->user_wait_flags;
    } else {
      // otherwise there have been multiple barriers with different stages and
      // figuring this out gets a bit more involved
      if ((stageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
           stageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) ||
          stageMask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        if (commandBuffer->compute_command_buffer ==
                commandBuffer->main_command_buffer ||
            commandBuffer->transfer_command_buffer ==
                commandBuffer->main_command_buffer) {
          // if we're waiting on all stages and one of the groups is still main
          // use the barrier group as it will be dispatched later, and thus able
          // to wait on main
          command_buffer = commandBuffer->compute_command_buffer ==
                                   commandBuffer->main_command_buffer
                               ? commandBuffer->transfer_command_buffer
                               : commandBuffer->compute_command_buffer;

          auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);

          stage_flags = &(*barrier_iter)->user_wait_flags;
        } else {
          // if both mux command buffers belong to different barriers use the
          // one that's latest in the barrier list
          auto compute_barrier_iter = find_barrier_info(
              commandBuffer->compute_command_buffer, commandBuffer);

          auto transfer_barrier_iter = find_barrier_info(
              commandBuffer->transfer_command_buffer, commandBuffer);

          auto barrier_iter =
              std::max(transfer_barrier_iter, compute_barrier_iter);

          command_buffer = (*barrier_iter)->command_buffer;
          stage_flags = &(*barrier_iter)->user_wait_flags;
        }
      } else {
        // we're only waiting for one stage
        if (stageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
          command_buffer = commandBuffer->compute_command_buffer;
        } else if (stageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
          command_buffer = commandBuffer->transfer_command_buffer;
        }

        if (command_buffer == commandBuffer->main_command_buffer) {
          stage_flags = &commandBuffer->main_command_buffer_event_wait_flags;
        } else {
          auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);
          stage_flags = &(*barrier_iter)->user_wait_flags;
        }
      }
    }

    if (auto error = muxCommandUserCallback(command_buffer, resetEventCallback,
                                            event, 0, nullptr, nullptr)) {
      commandBuffer->error = getVkResult(error);
      return;
    }

    *stage_flags |= stageMask;
  } else if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level ||
             command_buffer_t::recording == commandBuffer->state) {
    vk::command_info_reset_event command = {};
    command.event = event;
    command.stageMask = stageMask;

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdPushConstants(vk::command_buffer commandBuffer,
                      vk::pipeline_layout pipelineLayout,
                      VkShaderStageFlags stageFlags, uint32_t offset,
                      uint32_t size, const void *pValues) {
  (void)pipelineLayout;
  (void)stageFlags;

  std::memcpy(commandBuffer->push_constants.data() + offset, pValues, size);

  if (VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
      commandBuffer->command_buffer_level) {
    vk::command_info_push_constants command = {};
    command.offset = offset;
    command.size = size;
    command.pValues = commandBuffer->push_constants.data();

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdPipelineBarrier(vk::command_buffer commandBuffer,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask, VkDependencyFlags,
                        uint32_t, const VkMemoryBarrier *, uint32_t,
                        const VkBufferMemoryBarrier *, uint32_t,
                        const VkImageMemoryBarrier *) {
  if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == commandBuffer->command_buffer_level &&
      commandBuffer->state == command_buffer_t::recording) {
    // create new mux command buffer
    mux_command_buffer_t new_command_buffer;

    if (auto mux_error = muxCreateCommandBuffer(
            commandBuffer->mux_device, nullptr,
            commandBuffer->allocator.getMuxAllocator(), &new_command_buffer)) {
      commandBuffer->error = vk::getVkResult(mux_error);
      return;
    }

    // create new mux fence
    mux_fence_t new_fence;

    if (auto mux_error = muxCreateFence(
            commandBuffer->mux_device,
            commandBuffer->allocator.getMuxAllocator(), &new_fence)) {
      commandBuffer->error = vk::getVkResult(mux_error);
      return;
    }

    // create new semaphore
    mux_semaphore_t new_semaphore;

    if (auto mux_error = muxCreateSemaphore(
            commandBuffer->mux_device,
            commandBuffer->allocator.getMuxAllocator(), &new_semaphore)) {
      commandBuffer->error = vk::getVkResult(mux_error);
      return;
    }

    barrier_group_info group_info =
        commandBuffer->allocator.create<barrier_group_info_t>(
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT, new_command_buffer, new_fence,
            new_semaphore, srcStageMask, dstStageMask, 0,
            commandBuffer->allocator);

    // add to barrier mux command buffer infos
    if (commandBuffer->barrier_group_infos.push_back(group_info)) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }

    // set mux command buffers to be used for future commands based on dst stage
    if (dstStageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
      commandBuffer->transfer_command_buffer =
          commandBuffer->barrier_group_infos.back()->command_buffer;
      commandBuffer->transfer_stage_flags =
          &commandBuffer->barrier_group_infos.back()->stage_flags;
      commandBuffer->transfer_command_list =
          &commandBuffer->barrier_group_infos.back()->commands;
    }

    if (dstStageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
      commandBuffer->compute_command_buffer =
          commandBuffer->barrier_group_infos.back()->command_buffer;
      commandBuffer->compute_stage_flags =
          &commandBuffer->barrier_group_infos.back()->stage_flags;
      commandBuffer->compute_command_list =
          &commandBuffer->barrier_group_infos.back()->commands;
    }

    // set state to resolving
    commandBuffer->state = command_buffer_t::resolving;

    // go through list of recorded commands, executing each one that matches
    // srcStage and then erasing it
    auto has_stage_flag = [srcStageMask,
                           &commandBuffer](const command_info &info) {
      if (info.stage_flag & srcStageMask) {
        if (!(commandBuffer->usage_flags &
              VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
          ExecuteCommand(commandBuffer, info);
        } else if (commandBuffer->barrier_group_infos.back()
                       ->commands.push_back(info)) {
          commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        return true;
      }
      return false;
    };

    commandBuffer->commands.erase(
        std::remove_if(commandBuffer->commands.begin(),
                       commandBuffer->commands.end(), has_stage_flag),
        commandBuffer->commands.end());

    // set state back to recording
    commandBuffer->state = command_buffer_t::recording;

    // if the simultaneous use bit is set we might have to copy this command
    // buffer, so push the barrier command to the command list so it carries
    // over to any copies we make
    if (commandBuffer->usage_flags &
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
      const vk::command_info_pipeline_barrier command = {};
      if (commandBuffer->barrier_group_infos.back()->commands.push_back(
              vk::command_info(command))) {
        commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
  } else {
    vk::command_info_pipeline_barrier command = {};
    command.srcStageMask = srcStageMask;
    command.dstStageMask = dstStageMask;

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdDispatchIndirect(vk::command_buffer commandBuffer, vk::buffer buffer,
                         VkDeviceSize offset) {
  (void)buffer;
  (void)offset;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdCopyImage(vk::command_buffer commandBuffer, vk::image srcImage,
                  VkImageLayout srcImageLayout, vk::image dstImage,
                  VkImageLayout dstImageLayout, uint32_t regionCount,
                  const VkImageCopy *pRegions) {
  (void)srcImage;
  (void)srcImageLayout;
  (void)dstImage;
  (void)dstImageLayout;
  (void)regionCount;
  (void)pRegions;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdCopyBufferToImage(vk::command_buffer commandBuffer,
                          vk::buffer srcBuffer, vk::image dstImage,
                          VkImageLayout dstImageLayout, uint32_t regionCount,
                          const VkBufferImageCopy *pRegions) {
  (void)srcBuffer;
  (void)dstImage;
  (void)dstImageLayout;
  (void)regionCount;
  (void)pRegions;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdCopyImageToBuffer(vk::command_buffer commandBuffer, vk::image srcImage,
                          VkImageLayout srcImageLayout, vk::buffer dstBuffer,
                          uint32_t regionCount,
                          const VkBufferImageCopy *pRegions) {
  (void)srcImage;
  (void)srcImageLayout;
  (void)dstBuffer;
  (void)regionCount;
  (void)pRegions;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdClearColorImage(vk::command_buffer commandBuffer, vk::image image,
                        VkImageLayout imageLayout,
                        const VkClearColorValue *pColor, uint32_t rangeCount,
                        const VkImageSubresourceRange *pRanges) {
  (void)image;
  (void)imageLayout;
  (void)pColor;
  (void)rangeCount;
  (void)pRanges;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdWaitEvents(vk::command_buffer commandBuffer, uint32_t eventCount,
                   const VkEvent *pEvents, VkPipelineStageFlags srcStageMask,
                   VkPipelineStageFlags dstStageMask, uint32_t,
                   const VkMemoryBarrier *, uint32_t,
                   const VkBufferMemoryBarrier *, uint32_t,
                   const VkImageMemoryBarrier *) {
  if (vk::command_buffer_t::pending == commandBuffer->state ||
      vk::command_buffer_t::resolving == commandBuffer->state) {
    auto wait_info = commandBuffer->allocator.create<wait_callback_data_s>(
        VK_SYSTEM_ALLOCATION_SCOPE_COMMAND, &commandBuffer->allocator);
    if (!wait_info) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }
    vk::unique_ptr<wait_callback_data> wait_info_ptr(wait_info,
                                                     commandBuffer->allocator);
    const std::lock_guard<std::mutex> infoLock(wait_info_ptr->mutex);
    // list of locks obtained on each unsignaled event so they can't be signaled
    // while the command is still going
    vk::small_vector<std::unique_lock<std::mutex>, 2> eventLocks(
        {commandBuffer->allocator.getCallbacks(),
         VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});
    uint32_t unsignaled_events = eventCount;
    for (uint32_t eventIndex = 0; eventIndex < eventCount; eventIndex++) {
      vk::event event = reinterpret_cast<vk::event>(pEvents[eventIndex]);
      // we only need to wait for events that haven't been signaled yet and
      // whose set event stage mask matches this wait's `srcStageMask`
      // also, an event to be signaled from host wont have a `set_stage` set
      // so we add events that have no `set_stage` if host bit is set
      if ((srcStageMask &
               (event->set_stage | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) ||
           (VK_PIPELINE_STAGE_HOST_BIT & srcStageMask && !event->set_stage))) {
        std::unique_lock<std::mutex> lock(event->mutex, std::defer_lock);
        // GCOVR_EXCL_START non-deterministically executed
        if (lock.try_lock() && !event->signaled) {
          if (eventLocks.push_back(std::move(lock))) {
            commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
            return;
          }
          // add the wait event info to the event
          if (event->wait_infos.push_back(wait_info_ptr.get())) {
            commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
            return;
          }
          // if we couldn't obtain a lock then the event is already in the
          // process of being set
        } else {
          unsignaled_events--;
        }
        // GCOVR_EXCL_STOP
      } else {
        unsignaled_events--;
      }
    }
    wait_info_ptr->event_count = unsignaled_events;

    // if we didn't find any events that haven't been signaled yet this can
    // just be a no-op
    if (wait_info_ptr->event_count == 0) {
      return;  // GCOVR_EXCL_LINE non-deterministically executed
    }

    for (auto &lock : eventLocks) {
      lock.unlock();
    }

    // figure out which mux command buffer/semaphore need to be used
    mux_command_buffer_t command_buffer = nullptr;
    mux_semaphore_t semaphore = nullptr;

    // if there haven't been any barriers just used main
    if (commandBuffer->barrier_group_infos.empty()) {
      command_buffer = commandBuffer->main_command_buffer;
      semaphore = commandBuffer->main_semaphore;
    } else if (commandBuffer->compute_command_buffer ==
               commandBuffer->transfer_command_buffer) {
      // if there has been a barrier that replaced both mux command buffers we
      // can just use that
      auto barrier_info = find_barrier_info(
          commandBuffer->compute_command_buffer, commandBuffer);

      command_buffer = (*barrier_info)->command_buffer;
      semaphore = (*barrier_info)->semaphore;
    } else {
      // otherwise we'll need to do a bit of work to figure out which command
      // group should get the command
      if (((dstStageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) &&
           (dstStageMask & VK_PIPELINE_STAGE_TRANSFER_BIT)) ||
          dstStageMask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        // if we need all stages to wait for the events figure out which of the
        // barrier mux command buffers will be submitted first (which one is
        // earlier in the list), use its mux command buffer and alter its
        // dstStage to make later barrier mux command buffers wait for it
        auto compute_barrier_iter = find_barrier_info(
            commandBuffer->compute_command_buffer, commandBuffer);

        auto transfer_barrier_iter = find_barrier_info(
            commandBuffer->transfer_command_buffer, commandBuffer);

        auto first_barrier =
            std::min(compute_barrier_iter, transfer_barrier_iter);

        command_buffer = (*first_barrier)->command_buffer;
        semaphore = (*first_barrier)->semaphore;
        (*first_barrier)->dst_mask |= dstStageMask;
      } else {
        // finally, if we're only waiting for one stage things are a bit simpler
        if (dstStageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
          command_buffer = commandBuffer->compute_command_buffer;
        } else if (dstStageMask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
          command_buffer = commandBuffer->transfer_command_buffer;
        } else if (srcStageMask & VK_PIPELINE_STAGE_HOST_BIT) {
          // if we're only waiting on a host set just use main mux command
          // buffer
          command_buffer = commandBuffer->main_command_buffer;
        }

        if (command_buffer == commandBuffer->main_command_buffer) {
          semaphore = commandBuffer->main_semaphore;
        } else {
          auto barrier_iter = find_barrier_info(command_buffer, commandBuffer);
          semaphore = (*barrier_iter)->semaphore;
        }
      }
    }

    if (auto error = muxCommandUserCallback(command_buffer, waitEventCallback,
                                            wait_info_ptr.release(), 0, nullptr,
                                            nullptr)) {
      commandBuffer->error = getVkResult(error);
      return;
    }

    if (commandBuffer->wait_events_semaphores.push_back(
            {semaphore, dstStageMask})) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  } else if (vk::command_buffer_t::recording == commandBuffer->state ||
             VK_COMMAND_BUFFER_LEVEL_SECONDARY ==
                 commandBuffer->command_buffer_level) {
    // as with bind descriptor sets the unique object layer replaces pEvents
    // here with a local version which then invalidates so we need to copy
    // events into our own allocated list
    VkEvent *events =
        reinterpret_cast<VkEvent *>(commandBuffer->allocator.alloc(
            eventCount * sizeof(VkEvent), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT));

    for (uint32_t eventIndex = 0; eventIndex < eventCount; eventIndex++) {
      events[eventIndex] = pEvents[eventIndex];
    }

    command_info_wait_events command = {};
    command.eventCount = eventCount;
    command.pEvents = events;
    command.srcStageMask = srcStageMask;
    command.dstStageMask = dstStageMask;

    if (commandBuffer->commands.push_back(vk::command_info(command))) {
      commandBuffer->error = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
}

void CmdBeginQuery(vk::command_buffer commandBuffer, vk::query_pool queryPool,
                   uint32_t query, VkQueryControlFlags flags) {
  (void)queryPool;
  (void)query;
  (void)flags;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdEndQuery(vk::command_buffer commandBuffer, vk::query_pool queryPool,
                 uint32_t query) {
  (void)queryPool;
  (void)query;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdResetQueryPool(vk::command_buffer commandBuffer,
                       vk::query_pool queryPool, uint32_t firstQuery,
                       uint32_t queryCount) {
  (void)queryPool;
  (void)firstQuery;
  (void)queryCount;
  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdWriteTimestamp(vk::command_buffer commandBuffer,
                       VkPipelineStageFlagBits pipelineStage,
                       vk::query_pool queryPool, uint32_t query) {
  (void)pipelineStage;
  (void)queryPool;
  (void)query;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}

void CmdCopyQueryPoolResults(vk::command_buffer commandBuffer,
                             vk::query_pool queryPool, uint32_t firstQuery,
                             uint32_t queryCount, VkBuffer dstBuffer,
                             VkDeviceSize dstOffset, VkDeviceSize stride,
                             VkQueryResultFlags flags) {
  (void)queryPool;
  (void)firstQuery;
  (void)queryCount;
  (void)dstBuffer;
  (void)dstOffset;
  (void)stride;
  (void)flags;

  commandBuffer->error = VK_ERROR_FEATURE_NOT_PRESENT;
}
}  // namespace vk
