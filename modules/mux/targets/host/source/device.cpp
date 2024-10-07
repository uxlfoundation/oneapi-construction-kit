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

#include <cargo/endian.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>
#include <host/buffer.h>
#include <host/command_buffer.h>
#include <host/device.h>
#include <host/host.h>
#include <host/queue.h>
#include <mux/config.h>
#include <mux/mux.h>
#include <mux/utils/allocator.h>
#include <utils/system.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <unordered_map>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <unistd.h>
#endif

#ifdef __MCOS_POSIX__
#include <emcos/emcos_device_info.h>
#endif

static uint32_t os_cpu_frequency() {
#if defined(__APPLE__)

  int mib[] = {CTL_HW, HW_CPU_FREQ};
  uint32_t frequency;
  size_t length = sizeof(frequency);
  if (sysctl(mib, 2, &frequency, &length, nullptr, 0)) {
    return 0;  // could not request frequency!
  }
  return frequency / 1000000;

#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  HKEY regKey;

  const LONG openError = RegOpenKeyEx(
      HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
      0, KEY_READ, &regKey);

  if (ERROR_SUCCESS != openError) {
    return 0;  // could not open registry!
  }

  DWORD frequency = 0;
  DWORD bufferSize = sizeof(frequency);

  const LONG queryError =
      RegQueryValueEx(regKey, "~MHz", NULL, NULL,
                      reinterpret_cast<LPBYTE>(&frequency), &bufferSize);

  if (ERROR_SUCCESS != queryError) {
    return 0;  // could not query registry!
  }

  const LONG closeError = RegCloseKey(regKey);

  if (ERROR_SUCCESS != closeError) {
    return 0;  // could not close key!
  }

  return static_cast<uint32_t>(frequency);
#elif defined(__linux__)
  FILE *const file = fopen("/proc/cpuinfo", "r");
  if (nullptr == file) {
    return 0;  // could not query info file!
  }

  const uint32_t size = 256;
  char buffer[size];

  const cargo::string_view wanted = "cpu MHz";

  while (fgets(buffer, size, file)) {
    if (cargo::string_view(buffer, wanted.size()) == wanted) {
      uint32_t i = wanted.size();

      while (i < size) {
        if (buffer[i++] == ':') break;
      }

      (void)fclose(file);
      const float mhz = atof(buffer + i);
      return static_cast<uint32_t>(mhz);
    }
  }

  (void)fclose(file);
  return 0;  // could not find mhz value!
#elif defined(__MCOS_POSIX__)
  return 0;
#else
#error Unknown platform!
#endif
}

/// @brief The size of this system's memory in bytes.
///
/// Note that this will return the true memory size, which may happen to be
/// greater than 4GB, even on 32-bit systems.  If you want to report the total
/// memory size that will still fit inside a pointer use
/// @p os_memory_bounded_size.
static uint64_t os_memory_total_size() {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status)) {
    return static_cast<uint64_t>(status.ullTotalPhys);
  } else {
    return 0;
  }
#elif defined(__APPLE__)
  // query the physical memory size by name, name documented in
  // https://opensource.apple.com/source/xnu/xnu-792.12.6/libkern/libkern/sysctl.h
  uint64_t memsize;
  size_t size = sizeof(uint64_t);
  if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0)) {
    return 0;
  }
  return memsize;
#elif defined(__linux__)
  struct sysinfo info;
  if (0 == sysinfo(&info)) {
    return static_cast<uint64_t>(info.totalram) *
           static_cast<uint64_t>(info.mem_unit);
  } else {
    return 0;
  }
#elif defined(__MCOS_POSIX__)
  return emcos::get_device_total_memory_size();
#else
#error Unknown platform!
#endif
}

static uint64_t os_cache_size() {
// we query the L1 cache on all platforms, arbitrarily chosen
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  DWORD length = 0;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
  if (FALSE == GetLogicalProcessorInformation(buffer, &length)) {
    assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
  }

  buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(_alloca(length));

  DWORD result = GetLogicalProcessorInformation(buffer, &length);
  assert(FALSE != result);
#ifdef NDEBUG
  (void)result;
#endif

  for (DWORD i = 0, e = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
       i < e; i++) {
    switch (buffer[i].Relationship) {
      default:
        break;
      case RelationCache:
        if (1 == buffer[i].Cache.Level) {
          return buffer[i].Cache.Size;
        }
    }
  }

  return 0;
#elif defined(__APPLE__)
  // query the L1 cache size by name, name documented in
  // https://opensource.apple.com/source/xnu/xnu-792.12.6/libkern/libkern/sysctl.h
  uint64_t l1dcachesize = 0;
  size_t size = sizeof(uint64_t);
  if (sysctlbyname("hw.l1dcachesize", &l1dcachesize, &size, nullptr, 0)) {
    return 0;
  }
  return l1dcachesize;
#elif defined(__linux__)
  // open the cache size file for reading
  FILE *const file =
      fopen("/sys/devices/system/cpu/cpu0/cache/index0/size", "r");

  // did we open the file successfully
  if (nullptr == file) {
    return 0;
  }

  // data from the file
  char data[256];

  // read the contents of the cache file
  const size_t bytes_read = fread(data, sizeof(data), 1, file);

  if (sizeof(data) < bytes_read) {
    assert(0 && "Reading from the cache file failed!");
  }

  (void)fclose(file);

  // caches are described in kilobytes, so multiply the cache size
  return static_cast<uint64_t>(atoi(data)) * 1024;
#elif defined(__MCOS_POSIX__)
  return 0;
#else
#error Unknown platform!
#endif
}

static uint64_t os_cacheline_size() {
// we query the L1 cacheline on all platforms, arbitrarily chosen
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  DWORD length = 0;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
  if (FALSE == GetLogicalProcessorInformation(buffer, &length)) {
    assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
  }

  buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(_alloca(length));

  DWORD result = GetLogicalProcessorInformation(buffer, &length);
  assert(FALSE != result);
#ifdef NDEBUG
  (void)result;
#endif

  for (DWORD i = 0, e = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
       i < e; i++) {
    switch (buffer[i].Relationship) {
      default:
        break;
      case RelationCache:
        if (1 == buffer[i].Cache.Level) {
          return buffer[i].Cache.LineSize;
        }
    }
  }

  return 0;
#elif defined(__APPLE__)
  // query the cacheline size by name, name documented in
  // https://opensource.apple.com/source/xnu/xnu-792.12.6/libkern/libkern/sysctl.h
  uint64_t cachelinesize = 0;
  size_t size = sizeof(uint64_t);
  if (sysctlbyname("hw.cachelinesize", &cachelinesize, &size, nullptr, 0)) {
    return 0;
  }
  return cachelinesize;
#elif defined(__linux__)
  // open the cacheline size file for reading
  FILE *const file = fopen(
      "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");

  // did we open the file successfully
  if (nullptr == file) {
    return 0;
  }

  // data from the file
  char data[256];

  // read the contents of the cache file
  const size_t bytes_read = fread(data, sizeof(data), 1, file);

  if (sizeof(data) < bytes_read) {
    assert(0 && "Reading from the cache file failed!");
  }

  (void)fclose(file);

  // caches are described in bytes
  return static_cast<uint64_t>(atoi(data));
#elif defined(__MCOS_POSIX__)
  return 0;
#else
#error Unknown platform!
#endif
}

/// @brief The size of this systems memory in bytes, bounded by pointer size.
///
/// The function is like @p os_memory_total_size, but it will bound the
/// reported memory by what can be addressed by a pointer.
static uint64_t os_memory_bounded_size() {
  const uint64_t size = os_memory_total_size();
  // Limit the memory size to what fits in a size_t, this is necessary when
  // compiling for 32 bits on a 64 bits host
  return std::numeric_limits<size_t>::max() >= size
             ? size
             : std::numeric_limits<size_t>::max();
}

static uint32_t os_num_cpus() {
#if defined(__linux__) || defined(__APPLE__)
  return static_cast<uint32_t>(std::max(1L, sysconf(_SC_NPROCESSORS_ONLN)));
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return static_cast<uint32_t>(sysinfo.dwNumberOfProcessors);
#elif defined(__MCOS_POSIX__)
  return emcos::get_num_cpus();
#else
#error Unknown platform!
#endif
}

namespace host {
device_info_s::device_info_s()
    : device_info_s(detectHostArch(), detectHostOS(), /* native */ true,
                    CA_HOST_CL_DEVICE_NAME) {}

device_info_s::device_info_s(const char *device_name)
    : device_info_s(detectHostArch(), detectHostOS(), /* native */ true,
                    device_name) {}

device_info_s::device_info_s(host::arch arch, host::os os, bool native,
                             const char *device_name)
    : arch(arch), os(os), native(native) {
  this->allocation_capabilities = mux_allocation_capabilities_alloc_device |
                                  mux_allocation_capabilities_coherent_host |
                                  mux_allocation_capabilities_cached_host;

  this->address_capabilities = mux_address_capabilities_logical;
  switch (arch) {
    case host::arch::ARM:
    case host::arch::X86:
    case host::arch::RISCV32:
      this->address_capabilities |= mux_address_capabilities_bits32;
      this->atomic_capabilities = mux_atomic_capabilities_8bit |
                                  mux_atomic_capabilities_16bit |
                                  mux_atomic_capabilities_32bit;
      break;
    case host::arch::AARCH64:
    case host::arch::X86_64:
    case host::arch::RISCV64:
      this->address_capabilities |= mux_address_capabilities_bits64;
      this->atomic_capabilities =
          mux_atomic_capabilities_8bit | mux_atomic_capabilities_16bit |
          mux_atomic_capabilities_32bit | mux_atomic_capabilities_64bit;
      break;
  }

  // See Redmine #4946
  this->cache_capabilities =
      mux_cache_capabilities_read | mux_cache_capabilities_write;

#ifdef CA_HOST_ENABLE_FP16
  this->half_capabilities = mux_floating_point_capabilities_inf_nan |
                            mux_floating_point_capabilities_rte |
                            mux_floating_point_capabilities_full;
  if (arch != host::arch::ARM) {
    // Don't report half precision denormals on 32bit arm since we run floats
    // on NEON which is FTZ.
    this->half_capabilities |= mux_floating_point_capabilities_denorm;
  }

#else
  this->half_capabilities = 0;
#endif  // CA_HOST_ENABLE_FP16

  this->float_capabilities = mux_floating_point_capabilities_inf_nan |
                             mux_floating_point_capabilities_rte |
                             mux_floating_point_capabilities_full;
  if (arch != host::arch::ARM) {
    // Don't report single precision denormals on 32bit arm since NEON is FTZ.
    this->float_capabilities |= mux_floating_point_capabilities_denorm;
  }

#ifdef CA_HOST_ENABLE_FP64
  // See redmine #7924
  this->double_capabilities = mux_floating_point_capabilities_denorm |
                              mux_floating_point_capabilities_inf_nan |
                              mux_floating_point_capabilities_rte |
                              mux_floating_point_capabilities_rtz |
                              mux_floating_point_capabilities_rtp |
                              mux_floating_point_capabilities_rtn |
                              mux_floating_point_capabilities_fma |
                              mux_floating_point_capabilities_full;
#else
  this->double_capabilities = 0;
#endif  // CA_HOST_ENABLE_FP64

  // As an ISV without a unique device PCIe identifier, 0x10004 is the vendor ID
  // we've reserved in Khronos specs. Matches enums
  // 'CL_KHRONOS_VENDOR_ID_CODEPLAY' from OpenCL and 'VK_VENDOR_ID_CODEPLAY'
  // from Vulkan, but we can't use these symbols here because it would
  // introduce unwanted dependencies to our mux target.
  this->khronos_vendor_id = 0x10004;
  this->shared_local_memory_type = mux_shared_local_memory_virtual;
  this->device_type = mux_device_type_cpu;
  this->device_name = device_name;
  this->max_concurrent_work_items = 1024;
  this->max_work_group_size_x = this->max_concurrent_work_items;
  this->max_work_group_size_y = this->max_concurrent_work_items;
  this->max_work_group_size_z = this->max_concurrent_work_items;
  this->max_work_width = 64;
  // Tests for CL_DEVICE_MAX_CLOCK_FREQUENCY check that it is non-zero. For lack
  // of a better alternative, if we cannot figure it out, choose 1.
  this->clock_frequency =
      std::max<uint32_t>(native ? os_cpu_frequency() : 0, 1);
  this->compute_units = native ? os_num_cpus() : 0;
  this->buffer_alignment = sizeof(uint64_t) * 16;
  // TODO Reported memory size is quartered (rounded up) in order to pass the
  // OpenCL CTS however this probably should be an OCL specific fix and not in
  // the host target.
  this->memory_size = native ? ((os_memory_bounded_size() - 1) / 4) + 1 : 0;
  // All memory could be allocated at once.
  this->allocation_size = this->memory_size;
  this->cache_size = native ? os_cache_size() : 0;
  this->cacheline_size = native ? os_cacheline_size() : 0;

  // See redmine #4947
  this->shared_local_memory_size = 32L * 1024L;

  // default to 128 bit (16 bytes)
  this->native_vector_width = 128 / (8 * sizeof(uint8_t));
  this->preferred_vector_width = 128 / (8 * sizeof(uint8_t));

#ifdef HOST_IMAGE_SUPPORT
  // NOTE: Image max values are minimum allowed by the OpenCL specification.
  this->image_support = true;
  this->image2d_array_writes = true;
  this->image3d_writes = true;
  this->max_image_dimension_1d = 65536;
  this->max_image_dimension_2d = 8192;
  this->max_image_dimension_3d = 2048;
  this->max_image_array_layers = 2048;
  this->max_sampled_images = 128;
  this->max_storage_images = 8;
  this->max_samplers = 16;
#else
  this->image_support = false;
  this->image2d_array_writes = false;
  this->image3d_writes = false;
  this->max_image_dimension_1d = 0;
  this->max_image_dimension_2d = 0;
  this->max_image_dimension_3d = 0;
  this->max_image_array_layers = 0;
  this->max_sampled_images = 0;
  this->max_storage_images = 0;
  this->max_samplers = 0;
#endif

  // we have only one queue on host
  this->queue_types[mux_queue_type_compute] = 1;

  this->device_priority = 0;

  this->integer_capabilities =
      mux_integer_capabilities_8bit | mux_integer_capabilities_16bit |
      mux_integer_capabilities_32bit | mux_integer_capabilities_64bit;
  if (native) {
    this->endianness =
        cargo::is_little_endian() ? mux_endianness_little : mux_endianness_big;
  } else {
    // Assume that all cross-compile targets are little endian as it's not
    // possible to detect it.
    this->endianness = mux_endianness_little;
  }

  // The list of builtin kernels is defined in builtin_kernel.h. We want to get
  // the names of these kernels and store them in the device so it can be known
  // which kernels are supported. Must occur after all other initialization as
  // the values above may be used to generate builtin kernels.
  builtin_kernel_map = getBuiltinKernels(this);
  if (!builtin_kernel_map.empty()) {
    std::vector<cargo::string_view> str_list;
    str_list.reserve(builtin_kernel_map.size());
    for (auto &builtin_kernel : builtin_kernel_map) {
      auto &kernel_name = builtin_kernel.first;
      str_list.emplace_back(kernel_name.data(), kernel_name.size());
    }
    builtin_kernel_list = cargo::join(str_list.begin(), str_list.end(), ";");
  }
  this->builtin_kernel_declarations = builtin_kernel_list.c_str();

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  auto errorOrCounterArray = initPapiCounters();
  if (errorOrCounterArray.has_value() && !errorOrCounterArray.value().empty()) {
    this->query_counter_support = true;
    this->papi_counters = std::move(errorOrCounterArray.value());
    // 0 is the default index a system's CPU will occupy, if we ever want to run
    // host perf counters on something weirder than a desktop this may need
    // changing.
    auto component_info = PAPI_get_component_info(0);
    this->max_hardware_counters = component_info->num_cntrs;
  } else {
    this->query_counter_support = false;
    this->max_hardware_counters = 0;
  }
#else
  this->query_counter_support = false;
#endif
  this->descriptors_updatable = true;
  this->can_clone_command_buffers = true;
  this->max_sub_group_count = this->max_concurrent_work_items;
  this->sub_groups_support_ifp = false;
  this->supports_work_group_collectives = true;
  this->supports_generic_address_space = true;

  // A list of sub-group sizes we report. Roughly ordered according to
  // desirability.
  static std::array<size_t, 5> sg_sizes = {
      8, 4, 16, 32,
      1,  // we can always produce a 'trivial' sub-group if asked.
  };
  this->sub_group_sizes = sg_sizes.data();
  this->num_sub_group_sizes = sg_sizes.size();
}

host::arch device_info_s::detectHostArch() {
  host::arch arch;
#ifdef UTILS_SYSTEM_ARM
#ifdef UTILS_SYSTEM_32_BIT
  arch = host::arch::ARM;
#elif defined(UTILS_SYSTEM_64_BIT)
  arch = host::arch::AARCH64;
#endif
#elif defined(UTILS_SYSTEM_X86)
#ifdef UTILS_SYSTEM_32_BIT
  arch = host::arch::X86;
#elif defined(UTILS_SYSTEM_64_BIT)
  arch = host::arch::X86_64;
#endif
#elif defined(UTILS_SYSTEM_RISCV)
#ifdef UTILS_SYSTEM_32_BIT
  arch = host::arch::RISCV32;
#elif defined(UTILS_SYSTEM_64_BIT)
  arch = host::arch::RISCV64;
#endif
#else
#error cant detect host architecture
#endif
  return arch;
}

host::os device_info_s::detectHostOS() {
#if __ANDROID__
  return host::os::ANDROID;
#elif defined(__linux__) || defined(__MCOS_POSIX__)
  return host::os::LINUX;
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  return host::os::WINDOWS;
#elif defined(__APPLE__)
  return host::os::MACOS;
#else
#error cant detect host operating system
#endif
}

host::device_info_s &device_info_s::getHostInstance() {
  // Spec: the mux_device_info_t objects must have static lifetimes.
  static device_info_s device_info{};
  return device_info;
}

device_s::device_s(device_info_s *info, mux_allocator_info_t allocator_info)
    : queue(allocator_info, this) {
  this->info = info;
}

}  // namespace host

mux_result_t hostGetDeviceInfos(uint32_t device_types,
                                uint64_t device_infos_length,
                                mux_device_info_t *out_device_infos,
                                uint64_t *out_device_infos_length) {
  // Check if this device's type has been requested.
  if (0 ==
      (host::device_info_s::getHostInstance().device_type & device_types)) {
    // This device has not been requested so report zero device infos available
    // and return success.
    if (nullptr != out_device_infos_length) {
      *out_device_infos_length = 0;
    }
    return mux_success;
  }
  if (nullptr != out_device_infos_length) {
    *out_device_infos_length = 1;
  }
  if (nullptr != out_device_infos && 1 <= device_infos_length) {
    out_device_infos[0] =
        static_cast<mux_device_info_t>(&host::device_info_s::getHostInstance());
  }
  return mux_success;
}

mux_result_t hostCreateDevices(uint64_t devices_length,
                               mux_device_info_t *device_infos,
                               mux_allocator_info_t allocator,
                               mux_device_t *out_devices) {
  if (devices_length != 1) {
    return mux_error_invalid_value;
  }
  (void)device_infos;  // only one device defined for host

  void *const allocation = allocator.alloc(
      allocator.user_data, sizeof(host::device_s), alignof(host::device_s));

  if (nullptr == allocation) {
    return mux_error_out_of_memory;
  }

  out_devices[0] = new (allocation)
      host::device_s(&host::device_info_s::getHostInstance(), allocator);

  return mux_success;
}

void hostDestroyDevice(mux_device_t device,
                       mux_allocator_info_t allocator_info) {
  mux::allocator allocator(allocator_info);
  allocator.destroy(static_cast<host::device_s *>(device));
}
