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

#include <hal.h>
#include <hal_null.h>
#include <hal_riscv.h>

#include <memory>

using namespace hal;

hal_device_null_t::hal_device_null_t(hal_null_t &h,
                                     riscv::hal_device_info_riscv_t *info)
    : hal_device_t(info), hal(h) {}

hal_kernel_t hal_device_null_t::program_find_kernel(hal_program_t index,
                                                    const char *name) {
  (void)index;
  (void)name;
  return hal_invalid_kernel;
}

hal_program_t hal_device_null_t::program_load(const void *data,
                                              hal_size_t size) {
  (void)data;
  (void)size;
  return hal_invalid_program;
}

bool hal_device_null_t::kernel_exec(hal_program_t program, hal_kernel_t kernel,
                                    const hal_ndrange_t *nd_range,
                                    const hal_arg_t *args, uint32_t num_args,
                                    uint32_t work_dim) {
  (void)program;
  (void)kernel;
  (void)nd_range;
  (void)args;
  (void)num_args;
  (void)work_dim;
  return false;
}

bool hal_device_null_t::program_free(hal_program_t index) {
  (void)index;
  return false;
}

hal_addr_t hal_device_null_t::mem_alloc(hal_size_t size, hal_size_t alignment) {
  (void)size;
  (void)alignment;
  return hal_nullptr;
}

bool hal_device_null_t::mem_copy(hal_addr_t dst, hal_addr_t src,
                                 hal_size_t size) {
  (void)dst;
  (void)src;
  (void)size;
  return false;
}

bool hal_device_null_t::mem_free(hal_addr_t addr) {
  (void)addr;
  return false;
}

bool hal_device_null_t::mem_zero(hal_addr_t dst, hal_size_t size) {
  (void)dst;
  (void)size;
  return false;
}

bool hal_device_null_t::mem_fill(hal_addr_t dst, const void *pattern,
                                 hal_size_t pattern_size, hal_size_t size) {
  (void)dst;
  (void)pattern;
  (void)pattern_size;
  (void)size;
  return false;
}

bool hal_device_null_t::mem_read(void *dst, hal_addr_t src, hal_size_t size) {
  (void)dst;
  (void)src;
  (void)size;
  return false;
}

bool hal_device_null_t::mem_write(hal_addr_t dst, const void *src,
                                  hal_size_t size) {
  (void)dst;
  (void)src;
  (void)size;
  return false;
}

hal_null_t::hal_null_t() {
  info.platform_name = "HAL NULL";
  info.num_devices = 1;
  // store the api_version we compiled with
  info.api_version = hal_t::api_version;
  device_info.target_name = "NULL";
  device_info.type = hal_device_type_riscv;
}

const hal_info_t &hal_null_t::get_info() { return info; }

const hal_device_info_t *hal_null_t::device_get_info(uint32_t device_index) {
  if (device_index != 0) {
    return nullptr;
  }
  return &device_info;
}

hal_device_t *hal_null_t::device_create(uint32_t device_index) {
  // check index is valid (single device, only valid index is 0)
  if (device_index != 0) {
    return nullptr;
  }
  // instance already exists, cant create new device
  if (device) {
    return nullptr;
  }
  // create a new instance
  device.reset(new hal_device_null_t(*this, &device_info));
  return device.get();
}

bool hal_null_t::device_delete(hal_device_t *dev) {
  if (dev == device.get()) {
    device.reset();
  }
  return true;
}

// static HAL instance
namespace {
std::unique_ptr<hal_null_t> hal_instance;
}  // namespace

hal_t *get_hal(uint32_t &api_version) {
  if (!hal_instance) {
    hal_instance.reset(new hal_null_t);
  }
  api_version = hal_instance->get_info().api_version;
  return hal_instance.get();
}
