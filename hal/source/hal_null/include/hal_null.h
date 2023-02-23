// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Device Hardware Abstraction Layer stub implementation.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HAL_NULL_H_INCLUDED
#define HAL_NULL_H_INCLUDED

#include <memory>

// hal interface API
#include <hal.h>
#include <hal_riscv.h>

struct hal_null_t;

struct hal_device_null_t : public hal::hal_device_t {
  hal_device_null_t(hal_null_t &h, riscv::hal_device_info_riscv_t *info);

  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  hal::hal_program_t program_load(const void *data,
                                  hal::hal_size_t size) override;

  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  bool program_free(hal::hal_program_t program) override;

  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

  bool mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                hal::hal_size_t size) override;

  bool mem_free(hal::hal_addr_t addr) override;

  bool mem_fill(hal::hal_addr_t dst, const void *pattern,
                hal::hal_size_t pattern_size, hal::hal_size_t size) override;

  bool mem_zero(hal::hal_addr_t dst, hal::hal_size_t size);

  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) override;

  bool mem_write(hal::hal_addr_t dst, const void *src,
                 hal::hal_size_t size) override;

 protected:
  hal_null_t &hal;
};

struct hal_null_t : public hal::hal_t {
  hal_null_t();

  const hal::hal_info_t &get_info() override;

  const hal::hal_device_info_t *device_get_info(uint32_t device_index) override;

  hal::hal_device_t *device_create(uint32_t device_index) override;

  bool device_delete(hal::hal_device_t *device) override;

 protected:
  riscv::hal_device_info_riscv_t device_info;
  std::unique_ptr<hal_device_null_t> device;
  hal::hal_info_t info;
};

namespace hal {
struct hal::hal_t *get_hal();
}  // namespace hal

#endif  // HAL_NULL_H_INCLUDED
