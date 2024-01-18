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

#include <vk/command_buffer.h>
#include <vk/device.h>
#include <vk/fence.h>
#include <vk/queue.h>
#include <vk/semaphore.h>
#include <vk/type_traits.h>

namespace vk {

queue_t::queue_t(mux_queue_t mux_queue, vk::allocator allocator)
    : mux_queue(mux_queue),
      allocator(allocator),
      fence_command_buffer(nullptr),
      fence_command_buffers(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      fence_submitted(false) {}

queue_t::~queue_t() {
  if (fence_command_buffer) {
    mux_device_t mux_device = fence_command_buffer->device;

    muxDestroyCommandBuffer(mux_device, fence_command_buffer,
                            allocator.getMuxAllocator());
    for (auto &command_buffer : fence_command_buffers) {
      muxDestroyCommandBuffer(mux_device, command_buffer,
                              allocator.getMuxAllocator());
    }
  }
}

void GetDeviceQueue(vk::device device, uint32_t queueFamilyIndex,
                    uint32_t queueIndex, vk::queue *pQueue) {
  if (queueFamilyIndex == 0 && queueIndex == 0) {
    *pQueue = (device->queue);
  }
}

VkResult QueueWaitIdle(vk::queue queue) {
  const mux_result_t mux_error = muxWaitAll(queue->mux_queue);
  if (mux_success != mux_error) {
    return vk::getVkResult(mux_error);
  }
  return VK_SUCCESS;
}

static void semaphore_callback(mux_command_buffer_t command_buffer,
                               mux_result_t, void *user_data) {
  auto semaphore = reinterpret_cast<vk::semaphore>(user_data);

  mux_semaphore_t mux_semaphore;
  vk::queue queue = VK_NULL_HANDLE;

  {
    const std::lock_guard<std::mutex> lock(semaphore->mutex);
    queue = semaphore->queue;

    // first we need to find the matching semaphore for the mux command buffer
    // that called the callback GCOVR_EXCL_START non-deterministically executed
    if (command_buffer == semaphore->command_buffer) {
      mux_semaphore = semaphore->mux_semaphore;
    } else {
      auto pair_iter = std::find_if(
          semaphore->semaphore_tuples.begin(),
          semaphore->semaphore_tuples.end(),
          [&command_buffer](semaphore_command_buffer_fence_tuple &tuple) {
            return tuple.command_buffer == command_buffer;
          });
      VK_ASSERT(pair_iter != semaphore->semaphore_tuples.end(),
                "Invalid semaphore in callback");
      mux_semaphore = pair_iter->semaphore;
    }
    // GCOVR_EXCL_STOP
  }

  const std::lock_guard<std::mutex> lock(queue->mutex);

  // now that the semaphore is about to be signaled we can remove it from the
  // lists of user semaphores that new submissions need to wait for
  if (semaphore->wait_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
      semaphore->wait_stage & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
    semaphore->queue->user_compute_waits.erase(mux_semaphore);
  }

  if (semaphore->wait_stage & VK_PIPELINE_STAGE_TRANSFER_BIT ||
      semaphore->wait_stage & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
    semaphore->queue->user_transfer_waits.erase(mux_semaphore);
  }
}

// Callback which is called upon completion of execution of a command_buffer
static void cmd_group_complete_callback(mux_command_buffer_t, mux_result_t,
                                        void *user_data) {
  if (user_data) {
    vk::dispatch_callback_data cb_data =
        reinterpret_cast<vk::dispatch_callback_data>(user_data);
    // Handling change of state:
    if (!(cb_data->commandBuffer->usage_flags &
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
      {
        const std::lock_guard<std::mutex> lock(cb_data->commandBuffer->mutex);
        cb_data->commandBuffer->state = vk::command_buffer_t::executable;
      }
    }

    // Handle signalled semaphores:
    {
      const std::lock_guard<std::mutex> lock(cb_data->queue->mutex);

      // remove this mux command buffer's semaphore from the relevant lists
      // since the mux command buffer is done
      if (cb_data->stage_flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
          cb_data->stage_flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        cb_data->queue->compute_waits.erase(cb_data->semaphore);

        // as well as the regular semaphore list we need to see if this
        // semaphore was added to the user list by an event or barrier
        cb_data->queue->user_compute_waits.erase(cb_data->semaphore);
      }

      if (cb_data->stage_flags & VK_PIPELINE_STAGE_TRANSFER_BIT ||
          cb_data->stage_flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        cb_data->queue->transfer_waits.erase(cb_data->semaphore);

        cb_data->queue->user_transfer_waits.erase(cb_data->semaphore);
      }
    }

    cb_data->queue->allocator.destroy(cb_data);
  }
}

// Processes a single submit info
static VkResult ProcessSubmitInfo(const VkSubmitInfo &submitInfo,
                                  const vk::queue queue) {
  const std::lock_guard<std::mutex> lock(queue->mutex);
  // loop through this submit info's wait semaphores extracting the mux
  // semaphores and add them to appropriate lists based on their dst mask
  for (uint32_t waitSemaphoreIndex = 0;
       waitSemaphoreIndex < submitInfo.waitSemaphoreCount;
       waitSemaphoreIndex++) {
    vk::semaphore wait_semaphore =
        vk::cast<vk::semaphore>(submitInfo.pWaitSemaphores[waitSemaphoreIndex]);

    const std::lock_guard<std::mutex> lock(wait_semaphore->mutex);

    wait_semaphore->wait_stage =
        submitInfo.pWaitDstStageMask[waitSemaphoreIndex];

    // check if we need to create a new semaphore/mux command buffer
    // GCOVR_EXCL_START non-deterministically executed
    if (wait_semaphore->has_dispatched &&
        mux_fence_not_ready ==
            muxTryWait(queue->mux_queue, 0, wait_semaphore->mux_fence)) {
      mux_semaphore_t new_semaphore;
      if (auto error = muxCreateSemaphore(
              wait_semaphore->command_buffer->device,
              queue->allocator.getMuxAllocator(), &new_semaphore)) {
        return vk::getVkResult(error);
      }

      mux_command_buffer_t new_command_buffer;
      if (auto error = muxCreateCommandBuffer(
              wait_semaphore->command_buffer->device, nullptr,
              queue->allocator.getMuxAllocator(), &new_command_buffer)) {
        return vk::getVkResult(error);
      }

      mux_fence_t new_fence;
      if (auto error =
              muxCreateFence(wait_semaphore->command_buffer->device,
                             queue->allocator.getMuxAllocator(), &new_fence)) {
        return vk::getVkResult(error);
      }

      wait_semaphore->mux_semaphore = new_semaphore;
      wait_semaphore->command_buffer = new_command_buffer;
      wait_semaphore->mux_fence = new_fence;

      if (wait_semaphore->semaphore_tuples.push_back(
              {new_semaphore, new_command_buffer, new_fence})) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    } else if (wait_semaphore->has_dispatched) {
      // make sure this is reset if we are re-using a semaphore
      muxResetSemaphore(wait_semaphore->mux_semaphore);
    }
    // GCOVR_EXCL_STOP

    // add this wait semaphore's mux semaphore to the correct lists in queue
    if (wait_semaphore->wait_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
        wait_semaphore->wait_stage & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
      queue->user_compute_waits.insert(wait_semaphore->mux_semaphore);
    }
    if (wait_semaphore->wait_stage & VK_PIPELINE_STAGE_TRANSFER_BIT ||
        wait_semaphore->wait_stage & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
      queue->user_transfer_waits.insert(wait_semaphore->mux_semaphore);
    }

    wait_semaphore->queue = queue;

    mux_semaphore_t *wait_semaphore_waits =
        wait_semaphore->wait_semaphores.empty()
            ? nullptr
            : wait_semaphore->wait_semaphores.data();

    // Finalize the command buffer before dispatch.
    if (auto error = muxFinalizeCommandBuffer(wait_semaphore->command_buffer)) {
      return vk::getVkResult(error);
    }

    if (auto error =
            muxDispatch(queue->mux_queue, wait_semaphore->command_buffer,
                        wait_semaphore->mux_fence, wait_semaphore_waits,
                        wait_semaphore->wait_semaphores.size(),
                        &(wait_semaphore->mux_semaphore), 1, semaphore_callback,
                        wait_semaphore)) {
      return vk::getVkResult(error);
    }

    wait_semaphore->has_dispatched = true;
  }

  const vk::small_vector<mux_semaphore_t, 2> wait_events_semaphores(
      {queue->allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

  // loop through this submit info's command buffers
  for (uint32_t commandBufferIndex = 0;
       commandBufferIndex < submitInfo.commandBufferCount;
       commandBufferIndex++) {
    vk::command_buffer commandBuffer = reinterpret_cast<vk::command_buffer>(
        submitInfo.pCommandBuffers[commandBufferIndex]);
    const std::lock_guard<std::mutex> lockCb(commandBuffer->mutex);
    commandBuffer->state = command_buffer_t::pending;

    if (commandBuffer->usage_flags &
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
      // If the command buffer was started with the simultaneous use bit active
      // we need to check if it is already running. If it is then a simultaneous
      // submission is underway and we need to copy the command buffer so we can
      // dispatch the copy concurrently.
      if (commandBuffer->main_dispatched &&
          mux_fence_not_ready ==
              muxTryWait(queue->mux_queue, 0, commandBuffer->main_fence)) {
        // create new mux command buffer/semaphore
        mux_command_buffer_t new_command_buffer;

        if (auto error = muxCreateCommandBuffer(
                commandBuffer->mux_device, nullptr,
                commandBuffer->allocator.getMuxAllocator(),
                &new_command_buffer)) {
          return getVkResult(error);
        }

        mux_fence_t new_fence;

        if (auto error = muxCreateFence(
                commandBuffer->mux_device,
                commandBuffer->allocator.getMuxAllocator(), &new_fence)) {
          return getVkResult(error);
        }

        mux_semaphore_t new_semaphore;

        if (auto error = muxCreateSemaphore(
                commandBuffer->mux_device,
                commandBuffer->allocator.getMuxAllocator(), &new_semaphore)) {
          return getVkResult(error);
        }

        if (commandBuffer->simultaneous_use_list.push_back(
                {commandBuffer->main_command_buffer,
                 commandBuffer->main_semaphore, commandBuffer->main_fence})) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if (commandBuffer->transfer_command_buffer ==
            commandBuffer->main_command_buffer) {
          commandBuffer->transfer_command_buffer = new_command_buffer;
        }

        if (commandBuffer->compute_command_buffer ==
            commandBuffer->main_command_buffer) {
          commandBuffer->compute_command_buffer = new_command_buffer;
        }

        commandBuffer->main_command_buffer = new_command_buffer;
        commandBuffer->main_semaphore = new_semaphore;
        commandBuffer->main_fence = new_fence;
        // reset dispatched bool to make the commands execute again
        commandBuffer->main_dispatched = false;

        // if there were specialized kernels put them back in the undispatched
        // list so they are picked up on the second run through the dispatch
        // commands
        if (!commandBuffer->dispatched_kernels.empty()) {
          if (!commandBuffer->specialized_kernels.insert(
                  commandBuffer->specialized_kernels.begin(),
                  commandBuffer->dispatched_kernels.begin(),
                  commandBuffer->dispatched_kernels.end())) {
          }
          commandBuffer->dispatched_kernels.clear();
        }
      }

      for (auto &barrier_info : commandBuffer->barrier_group_infos) {
        if (barrier_info->dispatched &&
            mux_fence_not_ready ==
                muxTryWait(queue->mux_queue, 0, barrier_info->fence)) {
          // create new group/semaphore, discard old one
          mux_command_buffer_t new_command_buffer;
          if (auto error = muxCreateCommandBuffer(
                  commandBuffer->mux_device, nullptr,
                  commandBuffer->allocator.getMuxAllocator(),
                  &new_command_buffer)) {
            return getVkResult(error);
          }

          mux_fence_t new_fence;
          if (auto error = muxCreateFence(
                  commandBuffer->mux_device,
                  commandBuffer->allocator.getMuxAllocator(), &new_fence)) {
            return getVkResult(error);
          }

          mux_semaphore_t new_semaphore;
          if (auto error = muxCreateSemaphore(
                  commandBuffer->mux_device,
                  commandBuffer->allocator.getMuxAllocator(), &new_semaphore)) {
            return getVkResult(error);
          }

          if (commandBuffer->simultaneous_use_list.push_back(
                  {barrier_info->command_buffer, barrier_info->semaphore,
                   barrier_info->fence})) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }

          if (commandBuffer->transfer_command_buffer ==
              barrier_info->command_buffer) {
            commandBuffer->transfer_command_buffer = new_command_buffer;
          }

          if (commandBuffer->compute_command_buffer ==
              barrier_info->command_buffer) {
            commandBuffer->compute_command_buffer = new_command_buffer;
          }

          barrier_info->command_buffer = new_command_buffer;
          barrier_info->semaphore = new_semaphore;
          barrier_info->fence = new_fence;
          // reset dispatched bool to make the commands execute again
          barrier_info->dispatched = false;
        }
      }
    }

    // execute the recorded commands if they haven't been already
    if (!commandBuffer->main_dispatched) {
      for (auto &command_info : commandBuffer->commands) {
        ExecuteCommand(commandBuffer, command_info);
        if (commandBuffer->error != VK_SUCCESS) {
          return commandBuffer->error;
        }

        if (command_info.type == command_type_wait_events &&
            !(commandBuffer->usage_flags &
              VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
          commandBuffer->allocator.free(
              command_info.wait_events_command.pEvents);
        }
      }
    }

    // if simultaneous use is a possibility we will need this list to copy the
    // command buffer if it is submitted again, so don't clear it
    if (!(commandBuffer->usage_flags &
          VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
      commandBuffer->commands.clear();
    }

    vk::dispatch_callback_data_s *dispatch_callback_data =
        queue->allocator.create<vk::dispatch_callback_data_s>(
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT, queue, commandBuffer,
            commandBuffer->main_semaphore,
            commandBuffer->main_command_buffer_stage_flags);

    // figure out if main needs to wait on any user semaphores
    vk::small_vector<mux_semaphore_t, 2> main_wait_semaphores(
        {queue->allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});
    if (commandBuffer->main_command_buffer_stage_flags &
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
        !queue->user_compute_waits.empty()) {
      for (auto &semaphore : queue->user_compute_waits) {
        if (main_wait_semaphores.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      // if the mux command buffer executes in both stages we need to take extra
      // care not to add the same semaphore twice
      if (commandBuffer->main_command_buffer_stage_flags &
              VK_PIPELINE_STAGE_TRANSFER_BIT &&
          !queue->user_transfer_waits.empty()) {
        for (auto &semaphore : queue->user_transfer_waits) {
          if (std::find(main_wait_semaphores.begin(),
                        main_wait_semaphores.end(),
                        semaphore) == main_wait_semaphores.end()) {
            if (main_wait_semaphores.push_back(semaphore)) {
              return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
          }
        }
      }
    } else if (commandBuffer->main_command_buffer_stage_flags &
                   VK_PIPELINE_STAGE_TRANSFER_BIT &&
               !queue->user_transfer_waits.empty()) {
      for (auto &semaphore : queue->user_transfer_waits) {
        if (main_wait_semaphores.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }

    // check for wait flags generated by user event commands
    if (commandBuffer->main_command_buffer_event_wait_flags &
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
        commandBuffer->main_command_buffer_event_wait_flags &
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
      for (auto &semaphore : queue->compute_waits) {
        if (main_wait_semaphores.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      if (commandBuffer->main_command_buffer_event_wait_flags &
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
        // if we're looking in both lists for waits we need to ensure we don't
        // add the same semaphore twice
        for (auto &semaphore : queue->transfer_waits) {
          if (std::find(main_wait_semaphores.begin(),
                        main_wait_semaphores.end(),
                        semaphore) == main_wait_semaphores.end()) {
            if (main_wait_semaphores.push_back(semaphore)) {
              return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
          }
        }
      }
    }
    if (commandBuffer->main_command_buffer_event_wait_flags &
        VK_PIPELINE_STAGE_TRANSFER_BIT) {
      for (auto &semaphore : queue->transfer_waits) {
        if (main_wait_semaphores.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }

    mux_semaphore_t *wait_semaphores =
        main_wait_semaphores.empty() ? nullptr : main_wait_semaphores.data();

    // Finalize the command buffer before dispatch (consider if
    // muxFinalizeCommandBuffer should be called earlier, see CA-3155).
    if (auto error =
            muxFinalizeCommandBuffer(commandBuffer->main_command_buffer)) {
      return vk::getVkResult(error);
    }

    // submit the non-barrier mux command buffer first
    if (auto error =
            muxDispatch(queue->mux_queue, commandBuffer->main_command_buffer,
                        commandBuffer->main_fence, wait_semaphores,
                        main_wait_semaphores.size(),
                        &commandBuffer->main_semaphore, 1,
                        cmd_group_complete_callback,
                        dispatch_callback_data) != mux_success) {
      return vk::getVkResult(error);
    }

    // add the now executing main mux command buffer's semaphore to the relevant
    // lists
    if (commandBuffer->main_command_buffer_stage_flags &
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
      queue->compute_waits.insert(commandBuffer->main_semaphore);
    }

    if (commandBuffer->main_command_buffer_stage_flags &
        VK_PIPELINE_STAGE_TRANSFER_BIT) {
      queue->transfer_waits.insert(commandBuffer->main_semaphore);
    }

    commandBuffer->main_dispatched = true;

    // submit the barrier mux command buffers with wait semaphores appropriate
    // to their srcStage flags
    for (auto &barrier_info : commandBuffer->barrier_group_infos) {
      if (!barrier_info->dispatched) {
        for (auto &command : barrier_info->commands) {
          // Pipeline barrier commands take immediate effect, so they should be
          // skipped here.
          if (command.type != command_type_pipeline_barrier) {
            ExecuteCommand(commandBuffer, command);
          }
        }
      }

      if (!(commandBuffer->usage_flags &
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
        barrier_info->commands.clear();
      }

      vk::small_vector<mux_semaphore_t, 4> barrier_wait_semaphores(
          {queue->allocator.getCallbacks(),
           VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

      // first add user submitted wait semaphores according to their flags
      if (barrier_info->stage_flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
          !queue->user_compute_waits.empty()) {
        for (auto &semaphore : queue->user_compute_waits) {
          if (barrier_wait_semaphores.push_back(semaphore)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }
        }

        // if the mux command buffer executes in both stages we need to take
        // extra care not to add the same semaphore twice
        if (barrier_info->stage_flags & VK_PIPELINE_STAGE_TRANSFER_BIT &&
            !queue->user_transfer_waits.empty()) {
          for (auto &semaphore : queue->user_transfer_waits) {
            if (std::find(barrier_wait_semaphores.begin(),
                          barrier_wait_semaphores.end(),
                          semaphore) == barrier_wait_semaphores.end()) {
              if (barrier_wait_semaphores.push_back(semaphore)) {
                return VK_ERROR_OUT_OF_HOST_MEMORY;
              }
            }
          }
        }
      } else if (barrier_info->stage_flags & VK_PIPELINE_STAGE_TRANSFER_BIT &&
                 !queue->user_transfer_waits.empty()) {
        for (auto &semaphore : queue->user_transfer_waits) {
          if (barrier_wait_semaphores.push_back(semaphore)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }
        }
      }

      barrier_info->src_mask |= barrier_info->user_wait_flags;

      // now add the barrier specific wait semaphores
      if (barrier_info->src_mask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
          barrier_info->src_mask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        // don't need to check anything here as our list is empty and the
        // queue list should only contain unique entries
        if (!queue->compute_waits.empty()) {
          for (auto &semaphore : queue->compute_waits) {
            if (barrier_wait_semaphores.push_back(semaphore)) {
              return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
          }
        }
        // if this barrier depends on both compute and transfer we need to
        // make sure we aren't getting any doubles
        if (barrier_info->src_mask & VK_PIPELINE_STAGE_TRANSFER_BIT ||
            barrier_info->src_mask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
          for (auto &transfer_semaphore : queue->transfer_waits) {
            if (std::find(barrier_wait_semaphores.begin(),
                          barrier_wait_semaphores.end(), transfer_semaphore) ==
                barrier_wait_semaphores.end()) {
              if (barrier_wait_semaphores.push_back(transfer_semaphore)) {
                return VK_ERROR_OUT_OF_HOST_MEMORY;
              }
            }
          }
        }
      } else if (barrier_info->src_mask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
        // if the src mask is only transfer we can just repeat the quick
        // method used above for transfer
        if (!queue->transfer_waits.empty()) {
          for (auto &semaphore : queue->transfer_waits) {
            if (barrier_wait_semaphores.push_back(semaphore)) {
              return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
          }
        }
      }

      // if a previous barrier's dst matches this one's src then this one
      // might have some commands pushed to it that the previous one needs to
      // execute before, so we need to make sure this barrier mux command buffer
      // waits for the semaphores of of any previous ones that satisfy this
      // criteria
      for (auto barrier_iter = commandBuffer->barrier_group_infos.begin();
           *barrier_iter != barrier_info; barrier_iter++) {
        if ((*barrier_iter)->dst_mask & barrier_info->src_mask) {
          // there is a chance we just added this semaphore above, so check
          // to make sure we aren't adding it again
          if (std::find(barrier_wait_semaphores.begin(),
                        barrier_wait_semaphores.end(),
                        (*barrier_iter)->semaphore) ==
              barrier_wait_semaphores.end()) {
            if (barrier_wait_semaphores.push_back((*barrier_iter)->semaphore)) {
              return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
          }
        }
      }

      vk::dispatch_callback_data_s *dispatch_callback_data =
          queue->allocator.create<vk::dispatch_callback_data_s>(
              VK_SYSTEM_ALLOCATION_SCOPE_OBJECT, queue, commandBuffer,
              barrier_info->semaphore, barrier_info->stage_flags);

      mux_semaphore_t *wait_semaphores = barrier_wait_semaphores.empty()
                                             ? nullptr
                                             : barrier_wait_semaphores.data();

      if (auto error =
              muxDispatch(queue->mux_queue, barrier_info->command_buffer,
                          barrier_info->fence, wait_semaphores,
                          barrier_wait_semaphores.size(),
                          &barrier_info->semaphore, 1,
                          cmd_group_complete_callback,
                          dispatch_callback_data) != mux_success) {
        return vk::getVkResult(error);
      }

      barrier_info->dispatched = true;

      // add the now executing barrier mux command buffer's semaphore to the
      // relevant lists, first to the general "executing semaphores" lists
      if (barrier_info->stage_flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
        queue->compute_waits.insert(barrier_info->semaphore);
      }
      if (barrier_info->stage_flags & VK_PIPELINE_STAGE_TRANSFER_BIT) {
        queue->transfer_waits.insert(barrier_info->semaphore);
      }

      // then, based on the dst stage, add it to the lists of user generated
      // semaphores that all submissions must wait for to guarantee the second
      // synchronization scope extends to subsequent submissions and not just
      // this command buffer
      if (barrier_info->dst_mask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
          barrier_info->dst_mask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        queue->user_compute_waits.insert(barrier_info->semaphore);
      } else if (barrier_info->dst_mask & VK_PIPELINE_STAGE_TRANSFER_BIT) {
        queue->user_transfer_waits.insert(barrier_info->semaphore);
      }
    }
    // add semaphores signalled by mux command buffers waiting on events to the
    // user semaphore lists
    for (auto &event_pair : commandBuffer->wait_events_semaphores) {
      if (event_pair.flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT ||
          event_pair.flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        queue->user_compute_waits.insert(event_pair.semaphore);
      }
      if (event_pair.flags & VK_PIPELINE_STAGE_TRANSFER_BIT ||
          event_pair.flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        queue->user_transfer_waits.insert(event_pair.semaphore);
      }
    }
  }
  // deal with signal semaphores
  if (submitInfo.signalSemaphoreCount) {
    vk::small_vector<mux_semaphore_t, 4> signal_waits(
        {queue->allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

    // the first synchronisation scope of a signal semaphore consists of all
    // commands earlier in submission order, an easy way to get this is just
    // all the semaphores still in the queue's wait lists, as these are all
    // the commands already submitted that are still in progress
    if (!queue->compute_waits.empty()) {
      for (auto &semaphore : queue->compute_waits) {
        if (signal_waits.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }

    if (!queue->transfer_waits.empty()) {
      for (auto &semaphore : queue->transfer_waits) {
        if (signal_waits.push_back(semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }

    for (uint32_t signalSemaphoreIndex = 0;
         signalSemaphoreIndex < submitInfo.signalSemaphoreCount;
         signalSemaphoreIndex++) {
      {
        vk::semaphore semaphore = vk::cast<vk::semaphore>(
            submitInfo.pSignalSemaphores[signalSemaphoreIndex]);
        semaphore->wait_semaphores.clear();
        // save a snapshot of the semaphores that make up the first
        // synchronization scope for when this semaphore is waited on and thus
        // submitted
        if (!signal_waits.empty()) {
          if (!semaphore->wait_semaphores.insert(
                  semaphore->wait_semaphores.begin(), signal_waits.begin(),
                  signal_waits.end())) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }
        }
      }
    }
  }
  return VK_SUCCESS;
}

VkResult QueueSubmit(vk::queue queue, uint32_t submitCount,
                     const VkSubmitInfo *pSubmits, vk::fence fence) {
  vk::small_vector<mux_semaphore_t, 4> fence_semaphores(
      {queue->allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

  // loop through each submit info
  for (uint32_t submitIndex = 0; submitIndex < submitCount; submitIndex++) {
    if (const VkResult result =
            ProcessSubmitInfo(pSubmits[submitIndex], queue)) {
      return result;
    }

    // if we've been given a fence take every semaphore from each command buffer
    // so that the fence's mux command buffer can wait for them all
    if (fence) {
      for (uint32_t cbIndex = 0;
           cbIndex < pSubmits[submitIndex].commandBufferCount; cbIndex++) {
        vk::command_buffer command_buffer = vk::cast<vk::command_buffer>(
            pSubmits[submitIndex].pCommandBuffers[cbIndex]);
        if (fence_semaphores.push_back(command_buffer->main_semaphore)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        for (auto &group_info : command_buffer->barrier_group_infos) {
          if (fence_semaphores.push_back(group_info->semaphore)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }
        }
      }
    }
  }

  if (fence) {
    mux_semaphore_t *wait_semaphores =
        fence_semaphores.empty() ? nullptr : fence_semaphores.data();

    const uint32_t wait_semaphores_length =
        wait_semaphores ? fence_semaphores.size() : 0;

    if (auto error =
            muxDispatch(queue->mux_queue, fence->command_buffer,
                        fence->mux_fence, wait_semaphores,
                        wait_semaphores_length, nullptr, 0, nullptr, nullptr)) {
      return vk::getVkResult(error);
    }
  }

  return VK_SUCCESS;
}

dispatch_callback_data_s::dispatch_callback_data_s(
    vk::queue queue, vk::command_buffer commandBuffer,
    mux_semaphore_t semaphore, VkPipelineStageFlags stage_flags)
    : queue(queue),
      commandBuffer(commandBuffer),
      semaphore(semaphore),
      stage_flags(stage_flags) {}

VkResult QueueBindSparse(vk::queue queue, uint32_t bindInfoCount,
                         const VkBindSparseInfo *pBindInfo, vk::fence fence) {
  (void)queue;
  (void)bindInfoCount;
  (void)pBindInfo;
  (void)fence;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}
}  // namespace vk
