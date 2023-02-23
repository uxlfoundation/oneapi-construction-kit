// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_REFSI_MEMORY_H
#define _REFSIDRV_REFSI_MEMORY_H

#include <vector>

#include "common_devices.h"
#include "kernel_dma.h"
#include "refsidrv.h"

class RefSiMemoryWindow;
class PerfCounterDevice;

/// @brief Controls the various kinds of memory and memory-like devices attached
/// to the RefSi device.
struct RefSiMemoryController : public MemoryController {
  RefSiMemoryController(RefSiDevice &soc);
  virtual ~RefSiMemoryController();

  using map_type = std::map<reg_t, MemoryDevice *>;
  using const_iterator = map_type::const_iterator;
  const_iterator begin() { return get_devices().cbegin(); }
  const_iterator end() { return get_devices().cend(); }

  const std::vector<refsi_memory_map_entry> &getMemoryMap() const;

  RAMDevice *createMemRange(refsi_memory_map_kind kind, reg_t address,
                            size_t size);
  void addMemDevice(reg_t address, size_t size, refsi_memory_map_kind kind,
                    MemoryDevice *device);

  RefSiMemoryWindow * getWindow(unsigned index) const;

  refsi_result handleWindowRegWrite(refsi_cmp_register_id reg_idx,
                                    uint64_t value);

private:
  RefSiDevice &soc;
  std::vector<refsi_memory_map_entry> memory_map;
  RAMDevice *tcdm = nullptr;
  RAMDevice *dram = nullptr;
  DMADevice *dma_device = nullptr;
  PerfCounterDevice *perf_counter_device = nullptr;
  std::vector<RefSiMemoryWindow *> windows;
};

#endif  // _REFSIDRV_REFSI_MEMORY_H
