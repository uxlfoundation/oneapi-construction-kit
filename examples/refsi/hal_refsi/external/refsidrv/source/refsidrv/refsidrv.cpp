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

#include "refsidrv/refsidrv.h"

#include <assert.h>

#include "refsidrv/refsi_device.h"
#include "refsidrv/refsi_device_m.h"
#include "refsidrv/refsi_device_g.h"

static refsi_device_t global_m1_device = nullptr;
static refsi_device_t global_g1_device = nullptr;
static bool initialized = false;

refsi_result refsiInitialize() {
  if (!initialized) {
    global_m1_device = nullptr;
    global_g1_device = nullptr;
    initialized = true;
  }
  return refsi_success;
}

refsi_result refsiTerminate() {
  if (global_m1_device) {
    refsiShutdownDevice(global_m1_device);
  }
  if (global_g1_device) {
    refsiShutdownDevice(global_g1_device);
  }
  return refsi_success;
}

refsi_device_t refsiOpenDevice(refsi_device_family family) {
  const char *isa = nullptr;
  int vlen = 0;
  switch (family) {
  default:
    break;
  case REFSI_DEFAULT:
  case REFSI_M:
    if (!global_m1_device) {
      global_m1_device = new RefSiMDevice();
      if (global_m1_device->initialize() != refsi_success) {
        delete global_m1_device;
        global_m1_device = nullptr;
      }
    }
    return global_m1_device;
  case REFSI_G:
    if (!global_g1_device) {
      RefSiGDevice::getDefaultConfig(isa, vlen);
      global_g1_device = new RefSiGDevice(isa, vlen);
      if (global_g1_device->initialize() != refsi_success) {
        delete global_g1_device;
        global_g1_device = nullptr;
      }
    }
    return global_g1_device;
  }
  return nullptr;
}

refsi_result refsiShutdownDevice(refsi_device_t device) {
  if (device) {
    if (device == global_m1_device) {
      delete device;
      global_m1_device = nullptr;
    } else if (device == global_g1_device) {
      delete device;
      global_g1_device = nullptr;
    } else {
      return refsi_failure;
    }
    return refsi_success;
  } else {
    return refsi_failure;
  }
}

refsi_result refsiQueryDeviceInfo(refsi_device_t device,
                                  refsi_device_info_t *device_info) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->queryDeviceInfo(*device_info);
}

refsi_result refsiQueryDeviceMemoryMap(refsi_device_t device, size_t index,
                                       refsi_memory_map_entry *entry) {
  if (!device) {
    return refsi_invalid_device;
  }
  RefSiLock locker(device->getLock());
  if (index > device->getMemoryMap().size()) {
    return refsi_failure;
  } else if (!entry) {
    return refsi_failure;
  }
  *entry = device->getMemoryMap()[index];
  return refsi_success;
}

refsi_addr_t refsiAllocDeviceMemory(refsi_device_t device, size_t size,
                                    size_t alignment,
                                    refsi_memory_map_kind kind) {
  if (!device) {
    return refsi_nullptr;
  }
  return device->allocDeviceMemory(size, alignment, kind);
}

refsi_result refsiFreeDeviceMemory(refsi_device_t device,
                                   refsi_addr_t phys_addr) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->freeDeviceMemory(phys_addr);
}

void *refsiGetMappedAddress(refsi_device_t device, refsi_addr_t phys_addr,
                            size_t size) {
  if (!device) {
    return nullptr;
  }
  return device->getMappedAddress(phys_addr, size);
}

refsi_result refsiFlushDeviceMemory(refsi_device_t device,
                                    refsi_addr_t phys_addr, size_t size) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->flushDeviceMemory(phys_addr, size);
}

refsi_result refsiInvalidateDeviceMemory(refsi_device_t device,
                                         refsi_addr_t phys_addr, size_t size) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->invalidateDeviceMemory(phys_addr, size);
}

refsi_result refsiReadDeviceMemory(refsi_device_t device, uint8_t *dest,
                                   refsi_addr_t phys_addr, size_t size,
                                   uint32_t unit_id) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->readDeviceMemory(dest, phys_addr, size, unit_id);
}

refsi_result refsiWriteDeviceMemory(refsi_device_t device,
                                    refsi_addr_t phys_addr,
                                    const uint8_t *source, size_t size,
                                    uint32_t unit_id) {
  if (!device) {
    return refsi_invalid_device;
  }
  return device->writeDeviceMemory(phys_addr, source, size, unit_id);
}

refsi_result refsiExecuteCommandBuffer(refsi_device_t device,
                                       refsi_addr_t cb_addr, size_t size) {
  if (!device) {
    return refsi_invalid_device;
  } else if (device->getFamily() != refsi_soc_family::m) {
    return refsi_not_supported;
  }
  RefSiMDevice *m_device = (RefSiMDevice *)device;
  return m_device->executeCommandBuffer(cb_addr, size);
}

void refsiWaitForDeviceIdle(refsi_device_t device) {
  if (!device) {
    return;
  } else if (device->getFamily() != refsi_soc_family::m) {
    return;
  }
  RefSiMDevice *m_device = (RefSiMDevice *)device;
  m_device->waitForDeviceIdle();
}

refsi_result refsiExecuteKernel(refsi_device_t device,
                                refsi_addr_t entry_fn_addr,
                                uint32_t num_harts) {
  if (!device) {
    return refsi_failure;
  } else if (device->getFamily() != refsi_soc_family::g) {
    return refsi_not_supported;
  }
  RefSiGDevice *g_device = (RefSiGDevice *)device;
  return g_device->executeKernel(entry_fn_addr, num_harts);
}

refsi_result refsiDecodeCMPCommand(uint64_t header,
                                   refsi_cmp_command_id *opcode,
                                   uint32_t *chunk_count,
                                   uint32_t *inline_chunk) {
  if ((header & 0xc00000ff) != 0xc0000000) {
    return refsi_failure;
  }
  if (opcode) {
    *opcode = (refsi_cmp_command_id)((header & 0xff00) >> 8);
  }
  if (chunk_count) {
    *chunk_count = (uint32_t)(((header & 0x3fff0000) >> 16) / 2);
  }
  if (inline_chunk) {
    *inline_chunk = (uint32_t)(header >> 32ull);
  }
  return refsi_success;
}

uint64_t refsiEncodeCMPCommand(refsi_cmp_command_id opcode,
                               uint32_t chunk_count, uint32_t inline_chunk) {
  return 0xc0000000ull | (opcode << 8ull) | ((chunk_count * 2) << 16ull) |
         ((uint64_t)inline_chunk << 32ull);
}
