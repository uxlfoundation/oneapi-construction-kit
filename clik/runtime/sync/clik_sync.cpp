// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vector>

#include "clik_hal_version.h"
#include "hal.h"
#include "hal_library.h"
#include "sync/clik_objects.h"

// Create a new device object.
clik_device *clik_create_device() {
  hal::hal_library_t library = nullptr;
  hal::hal_t *hal =
      hal::load_hal(CLIK_HAL_NAME, supported_hal_api_version, library);
  if (!hal) {
    return nullptr;
  } else if (hal->get_info().num_devices < 0) {
    hal::unload_hal(library);
    return nullptr;
  }
  hal::hal_device_t *hal_device = hal->device_create(0);
  if (!hal_device) {
    hal::unload_hal(library);
    return nullptr;
  }
  fprintf(stderr, "Using device '%s'\n", hal->get_info().platform_name);
  clik_device *device = new clik_device();
  device->hal_device = hal_device;
  device->hal = hal;
  device->library = library;
  return device;
}

// Free the resources used by the device object.
void clik_release_device(clik_device *device) {
  if (!device) {
    return;
  }

  // Release the HAL.
  device->hal->device_delete(device->hal_device);
  delete device;
}

// Create a program object from an ELF binary. The program can contain one or
// more kernel functions.
clik_program *clik_create_program(clik_device *device, const void *binary_data,
                                  uint64_t binary_size) {
  if (!device) {
    return nullptr;
  }
  hal::hal_program_t elf =
      device->hal_device->program_load(binary_data, binary_size);
  if (elf == hal::hal_invalid_program) {
    return nullptr;
  }
  clik_program *program = new clik_program();
  program->device = device;
  program->elf = elf;
  return program;
}

// Free the resources used by the program object.
void clik_release_program(clik_program *program) {
  if (!program) {
    return;
  }
  program->device->hal_device->program_free(program->elf);
  delete program;
}

// Create a buffer object with the given size, which lives in device memory.
clik_buffer *clik_create_buffer(clik_device *device, uint64_t size) {
  if (!device) {
    return nullptr;
  }
  const uint64_t buffer_alignment = 4096;
  clik_buffer *buffer = new clik_buffer();
  buffer->device = device;
  buffer->device_addr = device->hal_device->mem_alloc(size, buffer_alignment);
  if (buffer->device_addr == hal::hal_nullptr) {
    delete buffer;
    return nullptr;
  }
  buffer->size = size;
  return buffer;
}

// Free the resources used by the buffer object.
void clik_release_buffer(clik_buffer *buffer) {
  if (!buffer) {
    return;
  }
  buffer->device->hal_device->mem_free(buffer->device_addr);
  delete buffer;
}

// Initialize a kernel argument with a buffer value.
void clik_init_buffer_arg(clik_argument *arg, clik_buffer *buffer) {
  arg->type = clik_argument_type::buffer;
  arg->buffer = buffer;
  arg->contents = nullptr;
}

// Initialize a kernel argument with a scalar value.
void clik_init_scalar_arg(clik_argument *arg, const void *val, uint64_t size) {
  arg->type = clik_argument_type::byval;
  arg->buffer = nullptr;
  arg->size = size;
  arg->contents = (uint8_t *)val;
}

// Initialize a kernel argument with a local memory value.
void clik_init_local_memory_arg(clik_argument *arg, uint64_t size) {
  arg->type = clik_argument_type::local;
  arg->buffer = nullptr;
  arg->size = size;
  arg->contents = nullptr;
}

// Initialize a N-D range value with a 1-dimensional range.
void clik_init_ndrange_1d(clik_ndrange *ndrange, uint64_t global_size,
                          uint64_t local_size) {
  ndrange->dims = 1;
  for (uint32_t i = 0; i < clik_ndrange::max_dimensions; i++) {
    ndrange->offset[i] = 0;
    ndrange->local[i] = (i == 0) ? local_size : 1;
    ndrange->global[i] = (i == 0) ? global_size : 1;
  }
}

// Initialize a N-D range value with a 2-dimensional range.
void clik_init_ndrange_2d(clik_ndrange *ndrange, uint64_t global_x,
                          uint64_t global_y, uint64_t local_x,
                          uint64_t local_y) {
  ndrange->dims = 2;
  for (uint32_t i = 0; i < clik_ndrange::max_dimensions; i++) {
    ndrange->offset[i] = 0;
    switch (i) {
      default:
        ndrange->local[i] = 1;
        ndrange->global[i] = 1;
        break;
      case 0:
        ndrange->local[i] = local_x;
        ndrange->global[i] = global_x;
        break;
      case 1:
        ndrange->local[i] = local_y;
        ndrange->global[i] = global_y;
        break;
    }
  }
}

// Read the contents of a buffer back to host memory.
bool clik_read_buffer(clik_device *device, void *dst, clik_buffer *src,
                      uint64_t src_offset, uint64_t size) {
  if (!device || !src || !dst) {
    return false;
  } else if ((src_offset + size) > src->size) {
    return false;
  }
  return device->hal_device->mem_read(dst, src->device_addr + src_offset, size);
}

// Write host data to device memory.
bool clik_write_buffer(clik_device *device, clik_buffer *dst,
                       uint64_t dst_offset, const void *src, uint64_t size) {
  if (!device || !src || !dst) {
    return false;
  } else if ((dst_offset + size) > dst->size) {
    return false;
  }
  return device->hal_device->mem_write(dst->device_addr + dst_offset, src,
                                       size);
}

// Copy data from one buffer to another buffer.
bool clik_copy_buffer(clik_device *device, clik_buffer *dst,
                      uint64_t dst_offset, clik_buffer *src,
                      uint64_t src_offset, uint64_t size) {
  if (!device || !src || !dst) {
    return false;
  } else if ((dst_offset + size) > dst->size) {
    return false;
  } else if ((src_offset + size) > src->size) {
    return false;
  }
  return device->hal_device->mem_copy(dst->device_addr + dst_offset,
                                      src->device_addr + src_offset, size);
}

bool clik_run_kernel(clik_program *program, const char *name,
                     const clik_ndrange *nd_range, const clik_argument *args,
                     uint32_t num_args) {
  if (!program) {
    return false;
  }
  hal::hal_device_t *hal_device = program->device->hal_device;
  hal::hal_kernel_t function_addr =
      hal_device->program_find_kernel(program->elf, name);
  if (!function_addr) {
    return false;
  }

  // Copy scheduling information.
  hal::hal_ndrange_t ndrange;
  hal::hal_size_t work_group_size = 1;
  for (uint32_t i = 0; i < nd_range->max_dimensions; i++) {
    ndrange.offset[i] = (i < nd_range->dims) ? nd_range->offset[i] : 0;
    ndrange.local[i] = (i < nd_range->dims) ? nd_range->local[i] : 1;
    ndrange.global[i] = (i < nd_range->dims) ? nd_range->global[i] : 1;
    work_group_size *= ndrange.local[i];
  }
  if (work_group_size == 0) {
    // Do not allow a local size of zero in any dimension.
    return false;
  }

  // Translate clik arguments to HAL arguments.
  std::vector<hal::hal_arg_t> hal_args;
  for (uint32_t i = 0; i < num_args; i++) {
    const clik_argument &arg(args[i]);
    hal::hal_arg_t hal_arg;
    switch (arg.type) {
      default:
        return false;
      case clik_argument_type::buffer:
        hal_arg.kind = hal::hal_arg_address;
        hal_arg.space = hal::hal_space_global;
        hal_arg.size = 0;
        hal_arg.address = arg.buffer->device_addr;
        break;
      case clik_argument_type::byval:
        hal_arg.kind = hal::hal_arg_value;
        hal_arg.space = hal::hal_space_global;
        hal_arg.size = arg.size;
        hal_arg.pod_data = arg.contents;
        break;
      case clik_argument_type::local:
        hal_arg.kind = hal::hal_arg_address;
        hal_arg.space = hal::hal_space_local;
        hal_arg.size = arg.size;
        hal_arg.address = hal::hal_nullptr;
        break;
    }
    hal_args.push_back(hal_arg);
  }

  if (!hal_device->kernel_exec(program->elf, function_addr, &ndrange,
                               hal_args.data(), hal_args.size(),
                               nd_range->dims)) {
    return false;
  }
  return true;
}
