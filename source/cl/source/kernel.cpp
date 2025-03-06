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

#include <CL/cl.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/mux.h>
#include <cl/platform.h>
#include <cl/printf.h>
#include <cl/program.h>
#include <cl/sampler.h>
#include <cl/validate.h>

#include <array>

#include "cargo/expected.h"
#ifdef OCL_EXTENSION_cl_khr_command_buffer
#include <extension/khr_command_buffer.h>
#endif
#include <tracer/tracer.h>

#include <memory>
#include <mutex>

mux_ndrange_options_t _cl_kernel::createKernelExecutionOptions(
    cl_device_id device, cl_uint device_index, size_t work_dim,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &local_size,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_offset,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_size,
    mux_buffer_t printf_buffer,
    std::unique_ptr<mux_descriptor_info_t[]> &descriptors) {
  (void)device;

  const size_t num_arguments = info->getNumArguments();
  const bool printf = nullptr != printf_buffer;
  descriptors = std::unique_ptr<mux_descriptor_info_t[]>(
      new mux_descriptor_info_t[printf ? num_arguments + 1 : num_arguments]);

  for (size_t i = 0; i < num_arguments; i++) {
    const _cl_kernel::argument &arg = saved_args[i];
    switch (arg.stype) {
      case _cl_kernel::argument::storage_type::local_memory: {
        descriptors[i].type = mux_descriptor_info_type_shared_local_buffer;
        descriptors[i].shared_local_buffer_descriptor.size =
            arg.local_memory_size;
      }
        continue;

      case _cl_kernel::argument::storage_type::memory_buffer:
        switch (arg.type.kind) {
          default:
            OCL_ABORT("Unhandled argument type");
            continue;

          case compiler::ArgumentKind::POINTER: {
            if (arg.memory_buffer) {
              auto buffer = static_cast<cl_mem_buffer>(arg.memory_buffer);
              descriptors[i].type = mux_descriptor_info_type_buffer;
              descriptors[i].buffer_descriptor.buffer =
                  buffer->mux_buffers[device_index];
              descriptors[i].buffer_descriptor.offset = 0;
            } else {
              descriptors[i].type = mux_descriptor_info_type_null_buffer;
            }
          }
            continue;

          case compiler::ArgumentKind::IMAGE1D:         // fall-through
          case compiler::ArgumentKind::IMAGE1D_ARRAY:   // fall-through
          case compiler::ArgumentKind::IMAGE1D_BUFFER:  // fall-through
          case compiler::ArgumentKind::IMAGE2D_ARRAY:   // fall-through
          case compiler::ArgumentKind::IMAGE3D:         // fall-through
          case compiler::ArgumentKind::IMAGE2D: {
            auto image = static_cast<cl_mem_image>(arg.memory_buffer);
            descriptors[i].type = mux_descriptor_info_type_image;
            descriptors[i].image_descriptor.image =
                image->mux_images[device_index];
          }
            continue;
        }

      case _cl_kernel::argument::storage_type::sampler: {
        // HACK(Benie): This code is going away so hack in samplers to
        // Core.cpp because the target device abstraction is going to die and
        // there is no point putting effort into it. These hexidecimal values
        // are taken from the CLK macro definitions used to create samplers in
        // libimg.
        descriptors[i].type = mux_descriptor_info_type_sampler;
        descriptors[i].sampler_descriptor.sampler.normalize_coords =
            0x1 & arg.sampler_value;

        switch (0xE & arg.sampler_value) {
          default:
            descriptors[i].sampler_descriptor.sampler.address_mode =
                mux_address_mode_none;
            break;
          case 0x2:
            descriptors[i].sampler_descriptor.sampler.address_mode =
                mux_address_mode_clamp_edge;
            break;
          case 0x4:
            descriptors[i].sampler_descriptor.sampler.address_mode =
                mux_address_mode_clamp;
            break;
          case 0x6:
            descriptors[i].sampler_descriptor.sampler.address_mode =
                mux_address_mode_repeat;
            break;
          case 0x8:
            descriptors[i].sampler_descriptor.sampler.address_mode =
                mux_address_mode_repeat_mirror;
            break;
        }

        switch (0x30 & arg.sampler_value) {
          default:
          case 0x10:
            descriptors[i].sampler_descriptor.sampler.filter_mode =
                mux_filter_mode_linear;
            continue;
          case 0x20:
            descriptors[i].sampler_descriptor.sampler.filter_mode =
                mux_filter_mode_nearest;
            continue;
        }
      }

      case _cl_kernel::argument::storage_type::value:
        descriptors[i].type = mux_descriptor_info_type_plain_old_data;
        descriptors[i].plain_old_data_descriptor.data = arg.value.data;
        descriptors[i].plain_old_data_descriptor.length = arg.value.size;
        continue;

      case _cl_kernel::argument::storage_type::uninitialized:
        continue;
    }
  }

  // printf buffer argument
  if (printf) {
    descriptors[num_arguments].type = mux_descriptor_info_type_buffer;
    descriptors[num_arguments].buffer_descriptor.buffer = printf_buffer;
    descriptors[num_arguments].buffer_descriptor.offset = 0;
  }

  mux_ndrange_options_t execution_options;
  execution_options.descriptors =
      ((num_arguments == 0) && !printf) ? nullptr : descriptors.get();
  execution_options.descriptors_length =
      printf ? num_arguments + 1 : num_arguments;
  execution_options.local_size[0] = local_size[0];
  execution_options.local_size[1] = local_size[1];
  execution_options.local_size[2] = local_size[2];
  execution_options.global_offset = global_offset.data();
  execution_options.global_size = global_size.data();
  execution_options.dimensions = work_dim;
  return execution_options;
}

cl_int _cl_kernel::retainMems(cl_command_queue command_queue,
                              std::function<bool(cl_mem)> retain) {
  // Collect list of cl_mem's to retain.
  for (size_t i = 0, e = info->getNumArguments(); i < e; i++) {
    if (saved_args[i].stype ==
        _cl_kernel::argument::storage_type::memory_buffer) {
      auto mem = saved_args[i].memory_buffer;

      if (retain(mem)) {
        return CL_OUT_OF_HOST_MEMORY;
      }

      // It is legal for cl_mem's to be NULL (i.e. a clSetKernelArg call was
      // given a NULL pointer as a cl_mem), the expectation in this case is
      // that the nullptr should be preserved through to the kernel call.
      // However, we can skip any memory synchronization between devices for
      // such pointers, as there is nothing to synchronize.
      if (nullptr == mem) {
        continue;
      }

      // Synchronize cl_mem's created with multiple devices in their context.
      switch (mem->type) {
        case CL_MEM_OBJECT_BUFFER: {
          if (auto error =
                  static_cast<cl_mem_buffer>(mem)->synchronize(command_queue)) {
            return error;
          }
        } break;
        case CL_MEM_OBJECT_IMAGE1D:
        case CL_MEM_OBJECT_IMAGE1D_BUFFER:
        case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        case CL_MEM_OBJECT_IMAGE2D:
        case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        case CL_MEM_OBJECT_IMAGE3D: {
          // TODO: Add synchronization of cl_mem_image objects once implemented.
        } break;
        default:
          return CL_INVALID_OPERATION;
      }
    }
  }
  return CL_SUCCESS;
}

namespace {
/// @brief Push kernel execution to the queue.
///
/// @param command_queue OpenCL command queue to enqueue on.
/// @param kernel OpenCL kernel to execute.
/// @param work_dim Dimensions of work to perform.
/// @param global_work_offset Global offset to begin work at.
/// @param global_work_size Global work item count.
/// @param local_work_size Local work item count.
/// @param num_events_in_wait_list Number of events in `event_wait_list`.
/// @param event_wait_list List of evetns to wait on.
/// @param return_event Kernel execution event.
///
/// @return Returns appropriate OpenCL error code.
cl_int PushExecuteKernel(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_work_offset,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_work_size,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &local_work_size,
    const cl_uint num_events_in_wait_list,
    const cl_event *const event_wait_list, cl_event return_event) {
  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());
  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  // We retained the event when creating the command, release it once the
  // command completes.
  //
  // Pass nullptr cl_event so that no command is submitted for profiling, we
  // need to push the kernel execution command before querying its end time.
  if (auto error = command_queue->registerDispatchCallback(
          *mux_command_buffer, nullptr,
          [return_event]() { cl::releaseInternal(return_event); })) {
    return error;
  }
#endif

  cl::retainInternal(kernel);
  cl::release_guard<cl_kernel> kernel_release_guard(
      kernel, cl::ref_count_type::INTERNAL);

  cl_device_id device = command_queue->device;
  mux_device_t mux_device = device->mux_device;

  auto &device_program = kernel->program->programs[command_queue->device];

  const mux_allocator_info_t mux_allocator =
      command_queue->device->mux_allocator;

  // create the printf buffer argument if necessary
  mux_buffer_t printf_buffer = nullptr;
  mux_memory_t printf_memory = nullptr;
  size_t num_groups = 0;
  size_t buffer_group_size = 0;
  if (device_program.printf_calls.size() != 0) {
    const cl_int err = createPrintfBuffer(
        device, local_work_size, global_work_size, num_groups,
        buffer_group_size, printf_memory, printf_buffer);
    if (err) {
      if (nullptr != return_event) {
        return_event->complete(CL_OUT_OF_RESOURCES);
      }
      return err;
    }
  }

  std::unique_ptr<mux_descriptor_info_t[]> descriptor_info_storage;
  const cl_uint device_index = kernel->program->context->getDeviceIndex(device);
  const mux_ndrange_options_t mux_execution_options =
      kernel->createKernelExecutionOptions(
          command_queue->device, device_index, work_dim, local_work_size,
          global_work_offset, global_work_size, printf_buffer,
          descriptor_info_storage);

  mux_kernel_t mux_specialized_kernel = nullptr;
  mux_executable_t mux_specialized_executable = nullptr;
  mux_kernel_t kernel_to_execute = nullptr;
  if (kernel->device_kernel_map[device]->supportsDeferredCompilation()) {
    auto result = kernel->device_kernel_map[device]->createSpecializedKernel(
        mux_execution_options);
    if (!result.has_value()) {
      if (printf_buffer) {
        muxDestroyBuffer(mux_device, printf_buffer, mux_allocator);
      }
      if (printf_memory) {
        muxFreeMemory(mux_device, printf_memory, mux_allocator);
      }
      return cl::getErrorFrom(result.error());
    }

    mux_specialized_kernel = result->mux_kernel.release();
    mux_specialized_executable = result->mux_executable.release();
    kernel_to_execute = mux_specialized_kernel;
  } else {
    // Execute the precompiled kernel.
    kernel_to_execute =
        kernel->device_kernel_map[device]->getPrecompiledKernel();
  }

  mux_result_t mux_error =
      muxCommandNDRange(*mux_command_buffer, kernel_to_execute,
                        mux_execution_options, 0, nullptr, nullptr);
  if (mux_success != mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (nullptr != return_event) {
      return_event->complete(error);
    }
    if (nullptr != printf_buffer) {
      muxDestroyBuffer(mux_device, printf_buffer, mux_allocator);
    }
    if (nullptr != printf_memory) {
      muxFreeMemory(mux_device, printf_memory, mux_allocator);
    }
    return error;
  }

  // enqueue a user callback that reads the printf buffer and print the data
  // out.
  if (device_program.printf_calls.size() != 0) {
    printf_info_t *printf_info =
        new printf_info_t{device,
                          printf_memory,
                          printf_buffer,
                          buffer_group_size,
                          std::vector<uint32_t>(num_groups, 0),
                          device_program.printf_calls};

    mux_error = createPrintfCallback(*mux_command_buffer, printf_info);
    OCL_ASSERT(mux_success == mux_error, "muxCommand failed!");
  }

  // collect list of cl_mem's to retain and store in a list to release later
  std::vector<cl_mem> mems_to_release;
  mems_to_release.reserve(kernel->info->getNumArguments());
  auto retain = [&mems_to_release](cl_mem mem) {
    cl::retainInternal(mem);
    mems_to_release.push_back(mem);
    return CL_SUCCESS;
  };

  if (auto error = kernel->retainMems(command_queue, retain)) {
    return error;
  }

  // don't release the kernel until it has been executed
  kernel_release_guard.dismiss();

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event,
      [kernel, mems_to_release, mux_device, mux_specialized_kernel,
       mux_specialized_executable, mux_allocator]() {
        for (auto mem : mems_to_release) {
          cl::releaseInternal(mem);
        }
        if (mux_specialized_kernel) {
          muxDestroyKernel(mux_device, mux_specialized_kernel, mux_allocator);
        }
        if (mux_specialized_executable) {
          muxDestroyExecutable(mux_device, mux_specialized_executable,
                               mux_allocator);
        }
        cl::releaseInternal(kernel);
      });
}
}  // namespace

MuxKernelWrapper::SpecializedKernel::~SpecializedKernel() {
  // The kernel must be destroyed before the executable.
  mux_kernel.reset();
  mux_executable.reset();
}

MuxKernelWrapper::MuxKernelWrapper(cl_device_id device, mux_kernel_t mux_kernel)
    : preferred_local_size_x(mux_kernel->preferred_local_size_x),
      preferred_local_size_y(mux_kernel->preferred_local_size_y),
      preferred_local_size_z(mux_kernel->preferred_local_size_z),
      local_memory_size(mux_kernel->local_memory_size),
      mux_device(device->mux_device),
      mux_allocator_info(device->mux_allocator),
      precompiled_kernel(mux_kernel),
      deferred_kernel(nullptr) {}

MuxKernelWrapper::MuxKernelWrapper(cl_device_id device,
                                   compiler::Kernel *deferred_kernel)
    : preferred_local_size_x(deferred_kernel->preferred_local_size_x),
      preferred_local_size_y(deferred_kernel->preferred_local_size_y),
      preferred_local_size_z(deferred_kernel->preferred_local_size_z),
      local_memory_size(deferred_kernel->local_memory_size),
      mux_device(device->mux_device),
      mux_allocator_info(device->mux_allocator),
      precompiled_kernel(nullptr),
      deferred_kernel(deferred_kernel) {}

bool MuxKernelWrapper::supportsDeferredCompilation() const {
  return deferred_kernel != nullptr;
}

compiler::Result MuxKernelWrapper::precacheLocalSize(size_t local_size_x,
                                                     size_t local_size_y,
                                                     size_t local_size_z) {
  if (deferred_kernel) {
    return deferred_kernel->precacheLocalSize(local_size_x, local_size_y,
                                              local_size_z);
  }
  return compiler::Result::SUCCESS;
}

uint32_t MuxKernelWrapper::getDynamicWorkWidth(size_t local_size_x,
                                               size_t local_size_y,
                                               size_t local_size_z) {
  // In the case where we don't have a subgroup size to return, we just return
  // 1.
  if (deferred_kernel) {
    return deferred_kernel
        ->getDynamicWorkWidth(local_size_x, local_size_y, local_size_z)
        .value_or(1);
  }

  return 1;
}

cargo::expected<MuxKernelWrapper::SpecializedKernel, compiler::Result>
MuxKernelWrapper::createSpecializedKernel(
    const mux_ndrange_options_t &specialization_options) {
  if (!deferred_kernel) {
    return cargo::make_unexpected(compiler::Result::FAILURE);
  }

  auto specialized_kernel =
      deferred_kernel->createSpecializedKernel(specialization_options);
  if (!specialized_kernel.has_value()) {
    return cargo::make_unexpected(specialized_kernel.error());
  }

  // Create a mux executable and kernel that contains this specialized binary.
  mux_result_t result;
  mux_executable_t mux_executable;
  mux_kernel_t mux_kernel;
  result = muxCreateExecutable(mux_device, specialized_kernel->data(),
                               specialized_kernel->size(), mux_allocator_info,
                               &mux_executable);
  if (result != mux_success) {
    if (result == mux_error_out_of_memory) {
      return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
    } else {
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }
  }

  mux::unique_ptr<mux_executable_t> mux_executable_ptr =
      mux::unique_ptr<mux_executable_t>(mux_executable,
                                        {mux_device, mux_allocator_info});

  result = muxCreateKernel(
      mux_device, mux_executable, deferred_kernel->name.data(),
      deferred_kernel->name.size(), mux_allocator_info, &mux_kernel);
  if (result != mux_success) {
    if (result == mux_error_out_of_memory) {
      return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
    } else {
      return cargo::make_unexpected(compiler::Result::FINALIZE_PROGRAM_FAILURE);
    }
  }

  mux::unique_ptr<mux_kernel_t> mux_kernel_ptr = mux::unique_ptr<mux_kernel_t>(
      mux_kernel, {mux_device, mux_allocator_info});

  return {SpecializedKernel{std::move(mux_executable_ptr),
                            std::move(mux_kernel_ptr)}};
}

mux_kernel_t MuxKernelWrapper::getPrecompiledKernel() const {
  return precompiled_kernel;
}

cargo::expected<size_t, cl_int> MuxKernelWrapper::getSubGroupSizeForLocalSize(
    size_t local_size_x, size_t local_size_y, size_t local_size_z) const {
  if (deferred_kernel) {
    auto expected_sub_group_size =
        deferred_kernel->querySubGroupSizeForLocalSize(
            local_size_x, local_size_y, local_size_z);
    if (!expected_sub_group_size) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
    return *expected_sub_group_size;
  } else {
    size_t sub_group_size;
    auto error = muxQuerySubGroupSizeForLocalSize(
        precompiled_kernel, local_size_x, local_size_y, local_size_z,
        &sub_group_size);
    if (error) {
      return cargo::make_unexpected(cl::getErrorFrom(error));
    }
    return sub_group_size;
  }
}

cargo::expected<size_t, cl_int> MuxKernelWrapper::getSubGroupCountForLocalSize(
    size_t local_size_x, size_t local_size_y, size_t local_size_z) const {
  const auto expected_sub_group_size =
      getSubGroupSizeForLocalSize(local_size_x, local_size_y, local_size_z);
  if (!expected_sub_group_size) {
    return expected_sub_group_size;
  }
  // The OpenCL spec says:
  //
  // All sub-groups must be the same size, while the last
  // subgroup in any work-group (i.e. the subgroup with the maximum index) could
  // be the same or smaller size.
  //
  // Implying that the remaining work items must form 1 sub-group.
  if (*expected_sub_group_size) {
    const auto local_size = local_size_x * local_size_y * local_size_z;
    const auto uniform_sub_group_count = local_size / *expected_sub_group_size;
    const auto remainder_sub_group_count =
        !!(local_size % *expected_sub_group_size);
    return uniform_sub_group_count + remainder_sub_group_count;
  }

  // This is the case that the sub-group size is zero.
  return 0;
}

cargo::expected<std::array<size_t, 3>, cl_int>
MuxKernelWrapper::getLocalSizeForSubGroupCount(size_t sub_group_count) const {
  if (deferred_kernel) {
    auto expected_local_size =
        deferred_kernel->queryLocalSizeForSubGroupCount(sub_group_count);
    if (!expected_local_size) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
    return *expected_local_size;
  } else {
    std::array<size_t, 3> local_size;
    const auto error = muxQueryLocalSizeForSubGroupCount(
        precompiled_kernel, sub_group_count,
        &local_size[0],  // NOLINT(readability-container-data-pointer)
        &local_size[1], &local_size[2]);
    if (error) {
      return cargo::make_unexpected(cl::getErrorFrom(error));
    }
    return local_size;
  }
}

cargo::expected<size_t, cl_int> MuxKernelWrapper::getMaxNumSubGroups() const {
  if (deferred_kernel) {
    auto expected_max_num_sub_groups = deferred_kernel->queryMaxSubGroupCount();
    if (!expected_max_num_sub_groups) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
    return *expected_max_num_sub_groups;
  } else {
    size_t max_sub_group_size;
    const auto error =
        muxQueryMaxNumSubGroups(precompiled_kernel, &max_sub_group_size);
    if (error) {
      return cargo::make_unexpected(cl::getErrorFrom(error));
    }
    return max_sub_group_size;
  }
}

_cl_kernel::_cl_kernel(cl_program program, std::string name,
                       const compiler::KernelInfo *info)
    : base<_cl_kernel>(cl::ref_count_type::EXTERNAL),
      program(program),
      name(name),
      info(info) {
  cl::retainInternal(program);
  program->num_external_kernels++;  // Count implicit retain on creation.
}

_cl_kernel::~_cl_kernel() { cl::releaseInternal(program); }

cargo::expected<cl_kernel, cl_int> _cl_kernel::create(
    cl_program program, std::string name, const compiler::KernelInfo *info) {
  std::unique_ptr<_cl_kernel> kernel{new _cl_kernel{program, name, info}};
  if (!kernel) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  if (kernel->saved_args.alloc(info->getNumArguments()) != cargo::success) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  for (auto device : program->context->devices) {
    auto kernel_wrapper_result =
        program->programs[device].createKernel(device, kernel->name);
    if (!kernel_wrapper_result.has_value()) {
      return cargo::make_unexpected(kernel_wrapper_result.error());
    }
    kernel->device_kernel_map[device] = std::move(*kernel_wrapper_result);
  }
  return kernel.release();
}

cargo::expected<cl_kernel, cl_int> _cl_kernel::clone() const {
  std::unique_ptr<_cl_kernel> kernel{new _cl_kernel{program, name, info}};
  if (!kernel) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  if (kernel->saved_args.alloc(info->getNumArguments()) != cargo::success) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  std::copy(saved_args.begin(), saved_args.end(), kernel->saved_args.begin());
  for (const auto &entry : device_kernel_map) {
    auto *kernel_wrapper_copy = new MuxKernelWrapper(*entry.second);
    if (!kernel_wrapper_copy) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
    kernel->device_kernel_map[entry.first].reset(kernel_wrapper_copy);
  }
  return kernel.release();
}

bool _cl_kernel::GetArgInfo() {
  if (arg_info) {
    return true;
  }

  cl_context context = program->context;
  for (auto device : context->devices) {
    const auto &device_program = program->programs[device];
    if (!device_program.isExecutable() ||
        !device_program.program_info.has_value()) {
      continue;
    }

    // Note: We can't just use `this->info` here, as that instance of
    // `ProgramInfo` may not have `argument_info` populated.
    const compiler::KernelInfo *info =
        device_program.program_info->getKernelByName(name);
    if (nullptr == info) {
      continue;
    }
    if (!info->argument_info) {
      continue;
    }

    auto &arg_info_storage = arg_info.emplace();
    if (arg_info_storage.alloc(info->getNumArguments()) != cargo::success) {
      arg_info.reset();
      continue;
    }
    std::copy_n(info->argument_info->begin(), info->getNumArguments(),
                arg_info_storage.begin());
    return true;
  }

  return false;
}

cargo::expected<const compiler::ArgumentType &, cl_int> _cl_kernel::GetArgType(
    const cl_uint arg_index) const {
  OCL_CHECK(arg_index >= info->getNumArguments(),
            return cargo::make_unexpected(CL_INVALID_ARG_INDEX));
  return info->argument_types[arg_index];
}

_cl_kernel::argument::argument()
    : type(compiler::ArgumentKind::UNKNOWN),
      stype(_cl_kernel::argument::storage_type::uninitialized) {}

_cl_kernel::argument::argument(compiler::ArgumentType arg_type,
                               size_t local_memory_size)
    : type(arg_type),
      local_memory_size(local_memory_size),
      stype(_cl_kernel::argument::storage_type::local_memory) {
  OCL_ASSERT(arg_type.address_space >= compiler::AddressSpace::LOCAL,
             "Trying to create a local memory argument with the wrong type.");
}

_cl_kernel::argument::argument(compiler::ArgumentType arg_type,
                               const cl_sampler sampler)
    : type(arg_type),
      sampler_value(sampler->sampler_value),
      stype(_cl_kernel::argument::storage_type::sampler) {
  OCL_ASSERT(arg_type.kind == compiler::ArgumentKind::SAMPLER,
             "Trying to create a sampler argument with the wrong type.");
}

_cl_kernel::argument::argument(compiler::ArgumentType type, cl_mem mem)
    : type(type),
      memory_buffer(mem),
      stype(_cl_kernel::argument::storage_type::memory_buffer) {
  OCL_ASSERT((compiler::ArgumentKind::POINTER == type.kind &&
              (type.address_space == compiler::AddressSpace::GLOBAL ||
               type.address_space == compiler::AddressSpace::CONSTANT)) ||
                 compiler::ArgumentKind::IMAGE2D == type.kind ||
                 compiler::ArgumentKind::IMAGE3D == type.kind ||
                 compiler::ArgumentKind::IMAGE2D_ARRAY == type.kind ||
                 compiler::ArgumentKind::IMAGE1D == type.kind ||
                 compiler::ArgumentKind::IMAGE1D_ARRAY == type.kind ||
                 compiler::ArgumentKind::IMAGE1D_BUFFER == type.kind,
             "Trying to create a memory argument with a non-memory type.");
}

_cl_kernel::argument::argument(compiler::ArgumentType type, const void *data,
                               size_t size)
    : type(type), stype(_cl_kernel::argument::storage_type::value) {
  value.size = size;
  if (data) {
    value.data = static_cast<void *>(new char[size]);
    std::memcpy(value.data, data, size);
  } else {
    value.data = nullptr;
  }
}

_cl_kernel::argument::argument(const argument &other)
    : type(other.type), stype(other.stype) {
  switch (other.stype) {
    case _cl_kernel::argument::storage_type::local_memory:
      local_memory_size = other.local_memory_size;
      break;
    case _cl_kernel::argument::storage_type::memory_buffer:
      memory_buffer = other.memory_buffer;
      break;
    case _cl_kernel::argument::storage_type::sampler:
      sampler_value = other.sampler_value;
      break;
    case _cl_kernel::argument::storage_type::value:
      value.data = static_cast<void *>(new char[other.value.size]);
      std::memcpy(value.data, other.value.data, other.value.size);
      value.size = other.value.size;
      break;
    case _cl_kernel::argument::storage_type::uninitialized:
      break;
  }
}

_cl_kernel::argument::argument(argument &&other)
    : type(other.type), stype(other.stype) {
  switch (other.stype) {
    case _cl_kernel::argument::storage_type::local_memory:
      local_memory_size = other.local_memory_size;
      break;
    case _cl_kernel::argument::storage_type::memory_buffer:
      memory_buffer = other.memory_buffer;
      break;
    case _cl_kernel::argument::storage_type::sampler:
      sampler_value = other.sampler_value;
      break;
    case _cl_kernel::argument::storage_type::value:
      value.data = other.value.data;
      value.size = other.value.size;
      break;
    case _cl_kernel::argument::storage_type::uninitialized:
      break;
  }
  // Invalidate moved object
  other.stype = _cl_kernel::argument::storage_type::uninitialized;
  other.type = compiler::ArgumentKind::UNKNOWN;
}

_cl_kernel::argument &_cl_kernel::argument::operator=(
    const _cl_kernel::argument &other) {
  if (stype == _cl_kernel::argument::storage_type::value && value.data) {
    delete[] static_cast<char *>(value.data);
  }
  new (this) _cl_kernel::argument(other);
  return *this;
}

_cl_kernel::argument &_cl_kernel::argument::operator=(
    _cl_kernel::argument &&other) {
  if (stype == _cl_kernel::argument::storage_type::value && value.data) {
    delete[] static_cast<char *>(value.data);
  }
  new (this) _cl_kernel::argument(std::move(other));
  return *this;
}

_cl_kernel::argument::~argument() {
  if (stype == _cl_kernel::argument::storage_type::value && value.data) {
    delete[] static_cast<char *>(value.data);
  }
}

CL_API_ENTRY cl_kernel CL_API_CALL cl::CreateKernel(cl_program program,
                                                    const char *kernel_name,
                                                    cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateKernel");
  OCL_CHECK(!program, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROGRAM);
            return nullptr);

  for (auto device : program->context->devices) {
    // if we don't have an finalized executable
    OCL_CHECK(!program->programs[device].isExecutable(),
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROGRAM_EXECUTABLE);
              return nullptr);
  }

  OCL_CHECK(!kernel_name, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  auto kernel_info = program->getKernelInfo(kernel_name);
  OCL_CHECK(!kernel_info,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_KERNEL_NAME);
            return nullptr);

  auto kernel = _cl_kernel::create(program, kernel_name, *kernel_info);
  if (!kernel) {
    OCL_SET_IF_NOT_NULL(errcode_ret, kernel.error());
    return nullptr;
  }

  // If we had any local sizes specified with the -cl-precache-local-sizes flag
  // or the reqd_work_group_size kernel attribute, we can compile the kernel for
  // those sizes here.
  for (auto device_program_entry = program->programs.begin();
       device_program_entry != program->programs.end();
       device_program_entry++) {
    auto device = device_program_entry->first;
    auto &device_program = device_program_entry->second;
    auto &device_kernel = kernel.value()->device_kernel_map[device];
    if (device_kernel->supportsDeferredCompilation()) {
      for (auto &size : device_program.compiler_module.module->getOptions()
                            .precache_local_sizes) {
        auto result =
            device_kernel->precacheLocalSize(size[0], size[1], size[2]);
        if (compiler::Result::SUCCESS != result) {
          OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROGRAM_EXECUTABLE);
          return nullptr;
        }
      }
      if (auto reqd_wg_size = kernel.value()->info->reqd_work_group_size) {
        auto result = device_kernel->precacheLocalSize(
            (*reqd_wg_size)[0], (*reqd_wg_size)[1], (*reqd_wg_size)[2]);
        if (compiler::Result::SUCCESS != result) {
          OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROGRAM_EXECUTABLE);
          return nullptr;
        }
      }
    }
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return kernel.value();
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainKernel(cl_kernel kernel) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainKernel");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  kernel->program->num_external_kernels++;
  return cl::retainExternal(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseKernel(cl_kernel kernel) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseKernel");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  kernel->program->num_external_kernels--;
  return cl::releaseExternal(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL cl::SetKernelArg(cl_kernel kernel,
                                                 cl_uint arg_index,
                                                 size_t arg_size,
                                                 const void *arg_value) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetKernelArg");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  OCL_CHECK(arg_index >= kernel->info->getNumArguments(),
            return CL_INVALID_ARG_INDEX);

  auto arg_type = kernel->GetArgType(arg_index);
  OCL_CHECK(!arg_type, return arg_type.error());

  // Allow extensions to handle kernel arguments first.
  auto error = extension::SetKernelArg(kernel, arg_index, arg_size, arg_value);
  // CL_INVALID_KERNEL is handled specially to signify that the extension was
  // not able to set the kernel argument.
  if (error != CL_INVALID_KERNEL) {
    return error;  // Other return codes are returned to the user as normal.
  }

  switch (arg_type->kind) {
    case compiler::ArgumentKind::POINTER: {
      if (arg_type->address_space == compiler::AddressSpace::GLOBAL ||
          arg_type->address_space == compiler::AddressSpace::CONSTANT) {
        OCL_CHECK(sizeof(cl_mem) != arg_size, return CL_INVALID_ARG_SIZE);

        if (nullptr == arg_value ||
            (nullptr == *static_cast<const cl_mem *>(arg_value))) {
          // If the argument value is null or points to a null value set the
          // buffer argument to be null.
          kernel->saved_args[arg_index] =
              _cl_kernel::argument(*arg_type, (_cl_mem *)nullptr);
        } else {
          cl_mem mem = *static_cast<const cl_mem *>(arg_value);

          OCL_CHECK(CL_MEM_OBJECT_BUFFER != mem->type,
                    return CL_INVALID_ARG_VALUE);
#ifdef CL_VERSION_3_0
          // Arguments can optionally be annoted with a 'dereferenceable'
          // attribute, which indicates how many bytes can be dereferenced. We
          // therefore check if the argument has this value before the checking
          // if it within limits.
          const cargo::optional<uint64_t> deref_bytes =
              kernel->GetArgType(arg_index)->dereferenceable_bytes;
          if (deref_bytes.has_value() && mem->size > deref_bytes.value()) {
            return CL_MAX_SIZE_RESTRICTION_EXCEEDED;
          }
#endif
          kernel->saved_args[arg_index] = _cl_kernel::argument(*arg_type, mem);
        }
      } else if (arg_type->address_space == compiler::AddressSpace::LOCAL) {
        OCL_CHECK(nullptr != arg_value, return CL_INVALID_ARG_VALUE);
        OCL_CHECK(arg_size == 0, return CL_INVALID_ARG_SIZE);
#ifdef CL_VERSION_3_0
        // Arguments can optionally be annoted with a 'dereferenceable'
        // attribute, which indicates how many bytes can be dereferenced. We
        // therefore check if the argument has this value before the checking
        // if it within limits.
        const cargo::optional<uint64_t> deref_bytes =
            kernel->GetArgType(arg_index)->dereferenceable_bytes;
        if (deref_bytes.has_value() && arg_size > deref_bytes.value()) {
          return CL_MAX_SIZE_RESTRICTION_EXCEEDED;
        }
#endif
        kernel->saved_args[arg_index] =
            _cl_kernel::argument(*arg_type, arg_size);
      } else {
        OCL_CHECK(nullptr != arg_value, return CL_INVALID_ARG_VALUE);
        OCL_CHECK(arg_size == 0, return CL_INVALID_ARG_SIZE);

        auto isCustomBufferCapable = [](cl_device_id device) {
          return device->mux_device->info->custom_buffer_capabilities != 0;
        };
        if (std::none_of(kernel->program->context->devices.begin(),
                         kernel->program->context->devices.end(),
                         isCustomBufferCapable)) {
          return CL_INVALID_ARG_VALUE;
        }

        kernel->saved_args[arg_index] =
            _cl_kernel::argument(*arg_type, arg_value, arg_size);
      }
    } break;

    case compiler::ArgumentKind::IMAGE2D:        // fall-through
    case compiler::ArgumentKind::IMAGE3D:        // fall-through
    case compiler::ArgumentKind::IMAGE2D_ARRAY:  // fall-through
    case compiler::ArgumentKind::IMAGE1D:        // fall-through
    case compiler::ArgumentKind::IMAGE1D_ARRAY:  // fall-through
    case compiler::ArgumentKind::IMAGE1D_BUFFER: {
      OCL_CHECK(sizeof(cl_mem) != arg_size, return CL_INVALID_ARG_SIZE);
      if ((nullptr == arg_value) ||
          (nullptr == *static_cast<const cl_mem *>(arg_value))) {
        // If the argument value is null or points to a null value set the
        // buffer argument to be null.
        kernel->saved_args[arg_index] =
            _cl_kernel::argument(*arg_type, (_cl_mem *)nullptr);
      } else {
        cl_mem mem = *static_cast<const cl_mem *>(arg_value);

        switch (mem->type) {
          case CL_MEM_OBJECT_IMAGE2D:
            OCL_CHECK(compiler::ArgumentKind::IMAGE2D != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          case CL_MEM_OBJECT_IMAGE3D:
            OCL_CHECK(compiler::ArgumentKind::IMAGE3D != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          case CL_MEM_OBJECT_IMAGE2D_ARRAY:
            OCL_CHECK(compiler::ArgumentKind::IMAGE2D_ARRAY != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          case CL_MEM_OBJECT_IMAGE1D:
            OCL_CHECK(compiler::ArgumentKind::IMAGE1D != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          case CL_MEM_OBJECT_IMAGE1D_ARRAY:
            OCL_CHECK(compiler::ArgumentKind::IMAGE1D_ARRAY != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          case CL_MEM_OBJECT_IMAGE1D_BUFFER:
            OCL_CHECK(compiler::ArgumentKind::IMAGE1D_BUFFER != arg_type->kind,
                      return CL_INVALID_ARG_VALUE);
            break;
          default:
            return CL_INVALID_ARG_VALUE;
        }

        kernel->saved_args[arg_index] = _cl_kernel::argument(*arg_type, mem);
      }
    } break;

    case compiler::ArgumentKind::SAMPLER: {
      OCL_CHECK(sizeof(cl_sampler) != arg_size, return CL_INVALID_ARG_SIZE);
      OCL_CHECK(nullptr == arg_value, return CL_INVALID_ARG_VALUE);
      kernel->saved_args[arg_index] = _cl_kernel::argument(
          *arg_type, *static_cast<const cl_sampler *>(arg_value));
      break;
    }

    case compiler::ArgumentKind::INT1:
      // Quick and dirty
      kernel->saved_args[arg_index] =
          _cl_kernel::argument(*arg_type, arg_value, arg_size);
      break;

    case compiler::ArgumentKind::INT1_2:
    case compiler::ArgumentKind::INT1_3:
    case compiler::ArgumentKind::INT1_4:
    case compiler::ArgumentKind::INT1_8:
    case compiler::ArgumentKind::INT1_16:
      // It is not valid to pass bool vectors to kernels.
      return CL_INVALID_KERNEL_ARGS;

#define CASE_VALUE_TYPE(arg_type, type)                              \
  case arg_type: {                                                   \
    OCL_CHECK(sizeof(type) != arg_size, return CL_INVALID_ARG_SIZE); \
    OCL_CHECK(!arg_value, return CL_INVALID_ARG_VALUE);              \
    kernel->saved_args[arg_index] =                                  \
        _cl_kernel::argument(arg_type, arg_value, arg_size);         \
  } break

#define CASES_VALUE_VECTOR_TYPE(arg_type, type) \
  CASE_VALUE_TYPE(arg_type, type);              \
  CASE_VALUE_TYPE(arg_type##_2, type##2);       \
  CASE_VALUE_TYPE(arg_type##_3, type##3);       \
  CASE_VALUE_TYPE(arg_type##_4, type##4);       \
  CASE_VALUE_TYPE(arg_type##_8, type##8);       \
  CASE_VALUE_TYPE(arg_type##_16, type##16)

      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT8, cl_char);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT16, cl_short);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT32, cl_int);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT64, cl_long);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::HALF, cl_half);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::FLOAT, cl_float);
      CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::DOUBLE, cl_double);

#undef CASE_VALUE
#undef CASES_VALUE_VECTOR_TYPE

    case compiler::ArgumentKind::STRUCTBYVAL: {
      OCL_CHECK(nullptr == arg_value, return CL_INVALID_ARG_VALUE);
      kernel->saved_args[arg_index] =
          _cl_kernel::argument(*arg_type, arg_value, arg_size);
      break;
    }

    case compiler::ArgumentKind::UNKNOWN:
      return CL_INVALID_KERNEL;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::CreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                           cl_kernel *kernels, cl_uint *num_kernels_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateKernelsInProgram");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  for (auto device : program->context->devices) {
    OCL_CHECK(!program->programs[device].isExecutable(),
              return CL_INVALID_PROGRAM_EXECUTABLE);
  }

  const size_t actual_num_kernels = program->getNumKernels();

  if (kernels) {
    OCL_CHECK(num_kernels < actual_num_kernels, return CL_INVALID_VALUE);

    for (size_t i = 0; i < actual_num_kernels; i++) {
      cl_int errcode;

      const char *kernelName = program->getKernelNameByOffset(i);

      kernels[i] = cl::CreateKernel(program, kernelName, &errcode);

      OCL_CHECK(CL_SUCCESS != errcode, return errcode);
    }
  }

  if (num_kernels_ret) {
    *num_kernels_ret = static_cast<cl_uint>(actual_num_kernels);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetKernelInfo(
    cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetKernelInfo");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

#define KERNEL_INFO_CASE(TYPE, SIZE_RET, POINTER, VALUE)                  \
  case TYPE: {                                                            \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);                  \
    OCL_CHECK(param_value &&param_value_size < SIZE_RET,                  \
              return CL_INVALID_VALUE);                                   \
    OCL_SET_IF_NOT_NULL((reinterpret_cast<POINTER>(param_value)), VALUE); \
  } break

  switch (param_name) {
    case CL_KERNEL_FUNCTION_NAME:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          sizeof(char) * kernel->name.size() + 1);
      OCL_CHECK(param_value &&
                    param_value_size < sizeof(char) * kernel->name.size() + 1,
                return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(reinterpret_cast<char *>(param_value),
                     kernel->name.c_str(), param_value_size);
      }
      break;
      KERNEL_INFO_CASE(CL_KERNEL_NUM_ARGS, sizeof(cl_uint), cl_uint *,
                       kernel->info->getNumArguments());
      KERNEL_INFO_CASE(CL_KERNEL_REFERENCE_COUNT, sizeof(cl_uint), cl_uint *,
                       kernel->refCountExternal());
      KERNEL_INFO_CASE(CL_KERNEL_CONTEXT, sizeof(cl_context), cl_context *,
                       kernel->program->context);
      KERNEL_INFO_CASE(CL_KERNEL_PROGRAM, sizeof(cl_program), cl_program *,
                       kernel->program);
    case CL_KERNEL_ATTRIBUTES: {
      // The OpenCL spec states that:
      // For kernels not created from OpenCL C source and the
      // clCreateProgramWithSource API call the string returned from this query
      // will be empty.
      const char *attributes = "";
      if (cl::program_type::OPENCLC == kernel->program->type) {
        attributes = kernel->info->attributes.c_str();
      }
      // +1 for the string terminator.
      const size_t attributes_length = std::strlen(attributes) + 1;
      OCL_SET_IF_NOT_NULL(param_value_size_ret, attributes_length);
      OCL_CHECK(param_value && param_value_size < attributes_length,
                return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(reinterpret_cast<char *>(param_value), attributes,
                     attributes_length);
      }
    } break;
    default: {
      return extension::GetKernelInfo(kernel, param_name, param_value_size,
                                      param_value, param_value_size_ret);
    }
  }

  return CL_SUCCESS;
}

/// @brief Converts a kernel argument address space from the compiler library to
/// an OpenCL argument address qualifier.
static cl_kernel_arg_address_qualifier convertKernelAddressQualifier(
    compiler::AddressSpace address) {
  switch (address) {
    case compiler::AddressSpace::PRIVATE:
      return CL_KERNEL_ARG_ADDRESS_PRIVATE;
    case compiler::AddressSpace::GLOBAL:
      return CL_KERNEL_ARG_ADDRESS_GLOBAL;
    case compiler::AddressSpace::CONSTANT:
      return CL_KERNEL_ARG_ADDRESS_CONSTANT;
    case compiler::AddressSpace::LOCAL:
      return CL_KERNEL_ARG_ADDRESS_LOCAL;
  }
  return 0;
}

/// @brief Converts a kernel argument access qualifier from the compiler library
/// to an OpenCL argument access qualifier.
static cl_kernel_arg_access_qualifier convertKernelArgAccessQualifier(
    compiler::KernelArgAccess access) {
  switch (access) {
    case compiler::KernelArgAccess::NONE:
      return CL_KERNEL_ARG_ACCESS_NONE;
    case compiler::KernelArgAccess::READ_ONLY:
      return CL_KERNEL_ARG_ACCESS_READ_ONLY;
    case compiler::KernelArgAccess::WRITE_ONLY:
      return CL_KERNEL_ARG_ACCESS_WRITE_ONLY;
    case compiler::KernelArgAccess::READ_WRITE:
      return CL_KERNEL_ARG_ACCESS_READ_WRITE;
  }
  return 0;
}

/// @brief Converts a kernel argument type qualifier from the compiler library
/// to an OpenCL argument type qualifier.
static cl_kernel_arg_type_qualifier convertKernelArgTypeQualifier(
    std::uint32_t type) {
  cl_uint cl_arg_type = 0;
  if (type & compiler::KernelArgType::CONST) {
    cl_arg_type |= CL_KERNEL_ARG_TYPE_CONST;
  }
  if (type & compiler::KernelArgType::RESTRICT) {
    cl_arg_type |= CL_KERNEL_ARG_TYPE_RESTRICT;
  }
  if (type & compiler::KernelArgType::VOLATILE) {
    cl_arg_type |= CL_KERNEL_ARG_TYPE_VOLATILE;
  }
  return cl_arg_type;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetKernelArgInfo(
    cl_kernel kernel, cl_uint arg_indx, cl_kernel_arg_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetKernelArgInfo");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(!kernel->GetArgInfo(), return CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
  OCL_CHECK(arg_indx > kernel->info->getNumArguments(),
            return CL_INVALID_ARG_INDEX);
  OCL_CHECK(param_value && param_value_size == 0, return CL_INVALID_VALUE);

  // Extensions may extend the list of values returned by standard
  // cl_kernel_arg_info values so must be handled first.
  if (CL_SUCCESS == extension::GetKernelArgInfo(kernel, arg_indx, param_name,
                                                param_value_size, param_value,
                                                param_value_size_ret)) {
    return CL_SUCCESS;
  }

  switch (param_name) {
    case CL_KERNEL_ARG_ADDRESS_QUALIFIER:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          sizeof(cl_kernel_arg_address_qualifier));
      OCL_CHECK(param_value &&
                    param_value_size < sizeof(cl_kernel_arg_address_qualifier),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(
          reinterpret_cast<cl_kernel_arg_address_qualifier *>(param_value),

          convertKernelAddressQualifier(
              kernel->arg_info.value()[arg_indx].address_qual));
      break;
    case CL_KERNEL_ARG_ACCESS_QUALIFIER:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          sizeof(cl_kernel_arg_access_qualifier));
      OCL_CHECK(param_value &&
                    param_value_size < sizeof(cl_kernel_arg_access_qualifier),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(
          reinterpret_cast<cl_kernel_arg_access_qualifier *>(param_value),
          convertKernelArgAccessQualifier(
              kernel->arg_info.value()[arg_indx].access_qual));
      break;
    case CL_KERNEL_ARG_TYPE_NAME:
      OCL_SET_IF_NOT_NULL(
          param_value_size_ret,
          kernel->arg_info.value()[arg_indx].type_name.size() + 1);
      OCL_CHECK(param_value &&
                    param_value_size <
                        kernel->arg_info.value()[arg_indx].type_name.size() + 1,
                return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(reinterpret_cast<char *>(param_value),
                     kernel->arg_info.value()[arg_indx].type_name.c_str(),
                     param_value_size);
      }
      break;
    case CL_KERNEL_ARG_TYPE_QUALIFIER:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          sizeof(cl_kernel_arg_type_qualifier));
      OCL_CHECK(param_value &&
                    param_value_size < sizeof(cl_kernel_arg_type_qualifier),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(
          reinterpret_cast<cl_kernel_arg_type_qualifier *>(param_value),
          convertKernelArgTypeQualifier(
              kernel->arg_info.value()[arg_indx].type_qual));
      break;
    case CL_KERNEL_ARG_NAME:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          kernel->arg_info.value()[arg_indx].name.size() + 1);
      OCL_CHECK(
          param_value && param_value_size <
                             kernel->arg_info.value()[arg_indx].name.size() + 1,
          return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(reinterpret_cast<char *>(param_value),
                     kernel->arg_info.value()[arg_indx].name.c_str(),
                     param_value_size);
      }
      break;
    default:
      return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetKernelWorkGroupInfo(
    cl_kernel kernel, cl_device_id device_id,
    cl_kernel_work_group_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetKernelWorkGroupInfo");
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(
      nullptr == device_id && kernel->program->context->devices.size() > 1,
      return CL_INVALID_DEVICE);
  OCL_CHECK(nullptr != device_id && !kernel->program->hasDevice(device_id),
            return CL_INVALID_DEVICE);

  cl_device_id device;
  if (nullptr != device_id) {
    device = device_id;
  } else {
    device = kernel->program->context->devices[0];
  }

  switch (param_name) {
    case CL_KERNEL_GLOBAL_WORK_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, (sizeof(size_t) * 3));
      OCL_CHECK(param_value && param_value_size < (sizeof(size_t) * 3),
                return CL_INVALID_VALUE);
      // If this isn't a custom type device the kernel must be a builtin kernel.
      OCL_CHECK(device->type != CL_DEVICE_TYPE_CUSTOM &&
                    kernel->program->type != cl::program_type::BUILTIN,
                return CL_INVALID_VALUE);
      if (param_value) {
        memcpy(param_value, device->max_work_item_sizes, sizeof(size_t) * 3);
      }
    } break;
    case CL_KERNEL_WORK_GROUP_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
      OCL_CHECK(param_value && param_value_size < sizeof(size_t),
                return CL_INVALID_VALUE);
      // Redmine #6204
      OCL_SET_IF_NOT_NULL((reinterpret_cast<size_t *>(param_value)),
                          device->max_work_group_size);
    } break;
    case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t) * 3);
      OCL_CHECK(param_value && param_value_size < (sizeof(size_t) * 3),
                return CL_INVALID_VALUE);
      if (param_value) {
        memcpy(param_value, kernel->info->getReqdWGSizeOrZero().data(),
               sizeof(size_t) * 3);
      }
    } break;
    case CL_KERNEL_LOCAL_MEM_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_ulong));
      OCL_CHECK(param_value && param_value_size < sizeof(cl_ulong),
                return CL_INVALID_VALUE);

      OCL_ASSERT(device, "No device was provided");
      OCL_SET_IF_NOT_NULL((reinterpret_cast<cl_ulong *>(param_value)),
                          kernel->device_kernel_map[device]->local_memory_size);
    } break;
    case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
      OCL_CHECK(param_value && param_value_size < sizeof(size_t),
                return CL_INVALID_VALUE);
      const size_t preferred_work_group_size_multiple =
          kernel->device_kernel_map[device]->preferred_local_size_x *
          kernel->device_kernel_map[device]->preferred_local_size_y *
          kernel->device_kernel_map[device]->preferred_local_size_z;
      OCL_SET_IF_NOT_NULL((reinterpret_cast<size_t *>(param_value)),
                          preferred_work_group_size_multiple);
    } break;
    case CL_KERNEL_PRIVATE_MEM_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_ulong));
      OCL_CHECK(param_value && param_value_size < sizeof(cl_ulong),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL((reinterpret_cast<cl_ulong *>(param_value)),
                          kernel->info->private_mem_size);
    } break;
    default: {
      return extension::GetKernelWorkGroupInfo(kernel, device_id, param_name,
                                               param_value_size, param_value,
                                               param_value_size_ret);
    }
  }

  return CL_SUCCESS;
}

cl_int _cl_kernel::checkReqdWorkGroupSize(cl_uint work_dim,
                                          const size_t *&local_work_size) {
  // Error check reqd_work_group_size attribute if present.
  if (info->reqd_work_group_size.has_value()) {
    // If local_work_size was not set, but the kernel has a
    // reqd_work_group_size, just use the required size for the kernel.  Note
    // that the specification actually states that CL_INVALID_WORK_GROUP_SIZE
    // should be returned in this situation, but real world program do this and
    // there are obvious semantics to follow so we're generous here.
    if (!local_work_size) {
      local_work_size = info->reqd_work_group_size->data();
    }

    for (cl_uint i = 0; i < work_dim; ++i) {
      OCL_CHECK((local_work_size[i] != (*info->reqd_work_group_size)[i]),
                return CL_INVALID_WORK_GROUP_SIZE);
    }
  }

  return CL_SUCCESS;
}

cl_int _cl_kernel::checkWorkSizes(cl_device_id device, cl_uint work_dim,
                                  const size_t *global_work_offset,
                                  const size_t *global_work_size,
                                  const size_t *local_work_size) {
  // Validate local and global sizes.
  size_t totalWorkGroupSize = 1;
  const size_t max_work_group_size = device->max_work_group_size;
  const size_t *max_work_item_sizes = device->max_work_item_sizes;
  for (cl_uint i = 0; i < work_dim; i++) {
#ifndef CL_VERSION_2_1
    // Returning an error code for zero dimensional ND range was deprecated
    // by OpenCL 2.1.
    if (global_work_size != nullptr) {
      OCL_CHECK(0 == global_work_size[i], return CL_INVALID_GLOBAL_WORK_SIZE);
    }
#endif
    if (local_work_size) {
      OCL_CHECK(0 == local_work_size[i], return CL_INVALID_WORK_GROUP_SIZE);
      if (global_work_size != nullptr) {
        OCL_CHECK(0 != (global_work_size[i] % local_work_size[i]),
                  return CL_INVALID_WORK_GROUP_SIZE);
      }
      OCL_CHECK(max_work_group_size < local_work_size[i],
                return CL_INVALID_WORK_GROUP_SIZE);
      OCL_CHECK(max_work_item_sizes[i] < local_work_size[i],
                return CL_INVALID_WORK_ITEM_SIZE);
      totalWorkGroupSize *= local_work_size[i];
    }

    // If this overflows it will wrap around, thus if the values are added and
    // become smaller then we have an invalid global offset
    if (global_work_size != nullptr && global_work_offset != nullptr) {
      OCL_CHECK(
          global_work_size[i] + global_work_offset[i] < global_work_size[i],
          return CL_INVALID_GLOBAL_OFFSET);
    }
  }
  OCL_CHECK(max_work_group_size < totalWorkGroupSize,
            return CL_INVALID_WORK_GROUP_SIZE);

  return CL_SUCCESS;
}

std::array<size_t, cl::max::WORK_ITEM_DIM> _cl_kernel::getDefaultLocalSize(
    cl_device_id device, const size_t *global_work_size, cl_uint work_dim) {
  std::array<size_t, cl::max::WORK_ITEM_DIM> local_sizes{1, 1, 1};
  const std::array<size_t, cl::max::WORK_ITEM_DIM> prefered_sizes{
      device_kernel_map[device]->preferred_local_size_x,
      device_kernel_map[device]->preferred_local_size_y,
      device_kernel_map[device]->preferred_local_size_z};

  if (global_work_size == nullptr) {
    return prefered_sizes;
  }

  for (cl_uint i = 0; i < work_dim; ++i) {
    // If global size does not divide equally by the local size (which is
    // defaulting to the preferred local size as advertised through the
    // mux_kernel_t), then we halve the local size in that dimension and
    // check if its acceptable otherwise we set it to 1 instead.
    if (0 == (global_work_size[i] % prefered_sizes[i])) {
      local_sizes[i] = prefered_sizes[i];
    } else {
      // Keep halving the `preferred_local_size` until we either get a value
      // that fits or `1`.
      size_t alternative_preferred_size = prefered_sizes[i];
      while (0 != (global_work_size[i] % alternative_preferred_size)) {
        alternative_preferred_size /= 2;
      }
      local_sizes[i] = alternative_preferred_size;
    }
  }

  return local_sizes;
}

cl_int _cl_kernel::checkKernelArgs() {
  for (size_t i = 0, e = info->getNumArguments(); i < e; i++) {
    OCL_CHECK(compiler::ArgumentKind::UNKNOWN == saved_args[i].type.kind,
              return CL_INVALID_KERNEL_ARGS);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueNDRangeKernel(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t *global_work_offset, const size_t *global_work_size,
    const size_t *local_work_size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueNDRangeKernel");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(!(kernel->program), return CL_INVALID_PROGRAM_EXECUTABLE);
  OCL_CHECK(!(command_queue->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(!(kernel->program->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(command_queue->context != kernel->program->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK((0 == work_dim) || (cl::max::WORK_ITEM_DIM < work_dim) ||
                (command_queue->device->max_work_item_dimensions < work_dim),
            return CL_INVALID_WORK_DIMENSION);
  OCL_CHECK(!global_work_size, return CL_INVALID_GLOBAL_WORK_SIZE);

  // Check the required work group size (if it exists).
  if (auto error = kernel->checkReqdWorkGroupSize(work_dim, local_work_size)) {
    return error;
  }

  // Check the local and global work sizes are correct.
  if (auto error = kernel->checkWorkSizes(command_queue->device, work_dim,
                                          global_work_offset, global_work_size,
                                          local_work_size)) {
    return error;
  }

  // If the user didn't pass a local size and the kernel doesn't require one
  // then pick one based on the device.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_local_work_size{1, 1, 1};
  if (local_work_size) {
    std::copy_n(local_work_size, work_dim, std::begin(final_local_work_size));
  } else {
    final_local_work_size = kernel->getDefaultLocalSize(
        command_queue->device, global_work_size, work_dim);
  }

  // If the user passed a NULL pointer as the global offset then this means that
  // the offset is {0,0,0}.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_global_offset{0, 0, 0};
  if (global_work_offset) {
    std::copy_n(global_work_offset, work_dim, std::begin(final_global_offset));
  }

  // The user must pass a global size but here we also initialize the global
  // sizes for the unused dimensions so callers don't have to keep checking the
  // work dimensions.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_global_size{1, 1, 1};
  std::copy_n(global_work_size, work_dim, std::begin(final_global_size));

  // Check the current kernel arguments are valid.
  if (auto error = kernel->checkKernelArgs()) {
    return error;
  }

  // Validate the event wait list.
  if (auto error =
          cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event)) {
    return error;
  }

  // Handle the signal event.
  cl_event return_event = nullptr;
  cl_int error = 0;
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  // We need to lock the context for the remainder of the function as we need to
  // ensure any blocking operations such clMemBlockingFreeINTEL are entirely in
  // sync as createBlockingEventForKernel adds to USM lists assuming that they
  // reflect already queued events.
  const std::lock_guard<std::mutex> context_guard(
      command_queue->context->usm_mutex);
  error = extension::usm::createBlockingEventForKernel(
      command_queue, kernel, CL_COMMAND_NDRANGE_KERNEL, return_event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  // Manually retain _cl_event since there might not be a USM allocation used in
  // the kernel to do retain for us
  cl::retainInternal(return_event);
  if (nullptr != event) {
    *event = return_event;
  } else {
    // If the user didn't pass an output event there won't be a user
    // call `clReleaseEvent()`, so we manually decrement the external
    // reference count instead on command completion
    cl::releaseExternal(return_event);
  }
#else
  if (nullptr != event) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_NDRANGE_KERNEL);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }
#endif

  // clang-tidy thinks cl::releaseExternal will reduce the ref-count to 0 and
  // delete return_event, but that's impossible because cl::retainInternal is
  // called just before.

  error =
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
      PushExecuteKernel(command_queue, kernel, work_dim, final_global_offset,
                        final_global_size, final_local_work_size,
                        num_events_in_wait_list, event_wait_list, return_event);
  if (error) {
    return error;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueTask(cl_command_queue command_queue,
                                                cl_kernel kernel,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event *event_wait_list,
                                                cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueTask");
  // Redmine issue 5014
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(command_queue->context != kernel->program->context,
            return CL_INVALID_CONTEXT);

  for (size_t i = 0, e = kernel->info->getNumArguments(); i < e; ++i) {
    OCL_CHECK(
        compiler::ArgumentKind::UNKNOWN == kernel->saved_args[i].type.kind,
        return CL_INVALID_KERNEL_ARGS);
  }

  // Error check reqd_work_group_size attribute if present
  if (auto reqd_wg_size = kernel->info->reqd_work_group_size) {
    for (uint32_t i = 0; i < 3; ++i) {
      OCL_CHECK(((0 != (*reqd_wg_size)[i]) && (1 != (*reqd_wg_size)[i])),
                return CL_INVALID_WORK_GROUP_SIZE);
    }
  }

  cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  // We need to lock the context for the remainder of the function as we need to
  // ensure any blocking operations such clMemBlockingFreeINTEL are entirely in
  // sync as createBlockingEventForKernel adds to USM lists assuming that they
  // reflect already queued events.
  const std::lock_guard<std::mutex> context_guard(
      command_queue->context->usm_mutex);
  error = extension::usm::createBlockingEventForKernel(
      command_queue, kernel, CL_COMMAND_TASK, return_event);
  OCL_CHECK(error != CL_SUCCESS, return error);
  // Manually retain _cl_event since there might not be a USM allocation used in
  // the kernel to do retain for us
  cl::retainInternal(return_event);

  if (nullptr != event) {
    *event = return_event;
  } else {
    // If the user didn't pass an output event there won't be a user
    // call `clReleaseEvent()`, so we manually decrement the external
    // reference count instead on command completion
    cl::releaseExternal(return_event);
  }
#else
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_TASK);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }
#endif

  const cl_uint work_dim = 1;
  const std::array<size_t, cl::max::WORK_ITEM_DIM> global_work_size{1, 1, 1};
  const std::array<size_t, cl::max::WORK_ITEM_DIM> local_work_size{1, 1, 1};
  const std::array<size_t, cl::max::WORK_ITEM_DIM> global_offset{0, 0, 0};

  // clang-tidy thinks cl::releaseExternal will reduce the ref-count to 0 and
  // delete return_event, but that's impossible because cl::retainInternal is
  // called just before.

  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
  error = PushExecuteKernel(
      command_queue, kernel, work_dim, global_offset, global_work_size,
      local_work_size, num_events_in_wait_list, event_wait_list, return_event);
  if (error) {
    return error;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueNativeKernel(
    cl_command_queue command_queue, void(CL_CALLBACK *user_func)(void *),
    void *args, size_t cb_args, cl_uint num_mem_objects, const cl_mem *mem_list,
    const void *const *args_mem_loc, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueNativeKernel");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!user_func, return CL_INVALID_VALUE);
  OCL_CHECK(!args && cb_args > 0, return CL_INVALID_VALUE);
  OCL_CHECK(!args && num_mem_objects > 0, return CL_INVALID_VALUE);
  OCL_CHECK(args && cb_args == 0, return CL_INVALID_VALUE);
  OCL_CHECK(!mem_list && num_mem_objects > 0, return CL_INVALID_VALUE);
  OCL_CHECK(mem_list && num_mem_objects == 0, return CL_INVALID_VALUE);
  OCL_CHECK(!args_mem_loc && num_mem_objects > 0, return CL_INVALID_VALUE);
  OCL_CHECK(args_mem_loc && num_mem_objects == 0, return CL_INVALID_VALUE);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  for (cl_uint i = 0; i < num_mem_objects; ++i) {
    OCL_CHECK(!mem_list[i], return CL_INVALID_MEM_OBJECT);
  }

  OCL_UNUSED(event);

  // Codeplay::ocl does not support native kernels, function fails gracefully
  return CL_INVALID_OPERATION;
}
