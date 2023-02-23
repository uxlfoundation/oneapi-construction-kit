// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux_util.h"

#include <stdlib.h>

#include <vector>

core_device_t createDevice(core_device_type_e type,
                           core_allocator_info_t allocator) {
#if defined(MUX_HAL_DEVICE)
  // Select the HAL device to use.
  setenv("CA_HAL_DEVICE", MUX_HAL_DEVICE, 0);
#endif

  uint64_t num_devices = 0;
  if (core_error_success !=
      coreGetDeviceInfos(type, 0, nullptr, &num_devices)) {
    return nullptr;
  } else if (num_devices < 1) {
    return nullptr;
  }

  std::vector<core_device_info_t> device_infos(num_devices);
  if (core_error_success !=
      coreGetDeviceInfos(type, num_devices, device_infos.data(), nullptr)) {
    return nullptr;
  }

  core_device_t device = nullptr;
  if (core_error_success !=
      coreCreateDevices(1, &device_infos[0], allocator, &device)) {
    return nullptr;
  }
  return device;
}

void *ExampleAlloc(void *, size_t size, size_t alignment) {
  void *pointer = 0;

  // our minimum alignment is the pointer width
  if (alignment < sizeof(void *)) {
    alignment = sizeof(void *);
  }

  size_t v = alignment;

  // http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
  if (!(v && !(v & (v - 1)))) {
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > 4294967295
    static_assert(8 == sizeof(size_t), "Can only shift 64-bit integers by 32");
    v |= v >> 32;
#endif
    v++;
  }

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  pointer = _aligned_malloc(size, v);
#else
  if (0 != posix_memalign(&pointer, v, size)) {
    pointer = 0;
  }
#endif

  return pointer;
}

void ExampleFree(void *user_data, void *pointer) {
  (void)user_data;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  _aligned_free(pointer);
#else
  free(pointer);
#endif
}

uint32_t findFirstSupportedHeap(uint32_t heaps) {
  // https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
  static const uint32_t lookupTable[32] = {
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
  return 0x1 << lookupTable[(((heaps & -heaps) * 0x077CB531U)) >> 27];
}
