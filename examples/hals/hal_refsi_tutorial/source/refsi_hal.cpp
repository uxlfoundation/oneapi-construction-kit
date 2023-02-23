// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "refsi_hal.h"

refsi_hal_device::refsi_hal_device(riscv::hal_device_info_riscv_t *info,
                                   std::recursive_mutex &hal_lock)
    : hal::hal_device_t(info), hal_lock(hal_lock) {}

hal::hal_kernel_t refsi_hal_device::program_find_kernel(
    hal::hal_program_t program, const char *name) {
  refsi_locker locker(hal_lock);
  return hal::hal_invalid_kernel;
}

hal::hal_program_t refsi_hal_device::program_load(const void *data,
                                                  hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  return hal::hal_invalid_program;
}

bool refsi_hal_device::program_free(hal::hal_program_t program) {
  refsi_locker locker(hal_lock);
  return false;
}

bool refsi_hal_device::kernel_exec(hal::hal_program_t program,
                                   hal::hal_kernel_t kernel,
                                   const hal::hal_ndrange_t *nd_range,
                                   const hal::hal_arg_t *args,
                                   uint32_t num_args, uint32_t work_dim) {
  refsi_locker locker(hal_lock);
  return false;
}

hal::hal_addr_t refsi_hal_device::mem_alloc(hal::hal_size_t size,
                                            hal::hal_size_t alignment) {
  refsi_locker locker(hal_lock);
  return hal::hal_nullptr;
}

bool refsi_hal_device::mem_free(hal::hal_addr_t addr) {
  refsi_locker locker(hal_lock);
  return false;
}

bool refsi_hal_device::mem_read(void *dst, hal::hal_addr_t src,
                                hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  return false;
}

bool refsi_hal_device::mem_write(hal::hal_addr_t dst, const void *src,
                                 hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  return false;
}
