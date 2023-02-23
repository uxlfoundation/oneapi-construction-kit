// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <mutex>

#include "cpu_hal.h"
#include "linker_script.h"

namespace {

class cpu_hal_platform : public hal::hal_t {
 protected:
  hal::hal_info_t hal_info;
  hal::hal_device_info_t hal_device_info;
  std::mutex lock;

 public:
  // return generic platform information
  const hal::hal_info_t &get_info() override {
    std::lock_guard<std::mutex> locker(lock);
    return hal_info;
  }

  // return generic target information
  const hal::hal_device_info_t *device_get_info(uint32_t index) override {
    std::lock_guard<std::mutex> locker(lock);
    return &hal_device_info;
  }

  // request the creation of a new hal
  hal::hal_device_t *device_create(uint32_t index) override {
    std::lock_guard<std::mutex> locker(lock);
    if (index > 0) {
      return nullptr;
    }
    return new cpu_hal(&hal_device_info, lock);
  }

  // destroy a device instance
  bool device_delete(hal::hal_device_t *device) override {
    // No locking - this is done by cpu_hal's destructor.
    delete static_cast<cpu_hal *>(device);
    return device != nullptr;
  }

  cpu_hal_platform() {
    const uint64_t global_ram_size = 256 << 20;
    const uint64_t global_mem_max_over_allocation = 16 << 20;
    const uint64_t local_ram_size = 8 << 20;
    hal_device_info.type = hal::hal_device_type_riscv;
    hal_device_info.word_size = sizeof(uintptr_t) * 8;
    hal_device_info.target_name = "CPU";
    hal_device_info.global_memory_avail =
        global_ram_size - global_mem_max_over_allocation;
    hal_device_info.shared_local_memory_size = local_ram_size;
    hal_device_info.should_link = true;
    hal_device_info.should_vectorize = false;
    hal_device_info.preferred_vector_width = 0;
    hal_device_info.supports_fp16 = false;
    hal_device_info.supports_doubles = true;
    hal_device_info.max_workgroup_size = 16;
    hal_device_info.is_little_endian = true;
    hal_device_info.linker_script =
        std::string(hal_cpu_linker_script, hal_cpu_linker_script_size);

    constexpr static uint32_t implemented_api_version = 6;
    static_assert(implemented_api_version == hal_t::api_version,
                  "Implemented API version for CPU HAL does not match hal.h");
    hal_info.platform_name = hal_device_info.target_name;
    hal_info.num_devices = 1;
    hal_info.api_version = implemented_api_version;
  }
};
}  // namespace

static cpu_hal_platform hal_object;

hal::hal_t *get_hal(uint32_t &api_version) {
  api_version = hal_object.get_info().api_version;
  return &hal_object;
}
