// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_REFSI_PERF_COUNTERS_H
#define _REFSIDRV_REFSI_PERF_COUNTERS_H

#include <map>

#include "devices.h"
#include "common_devices.h"

class RefSiDevice;

/// @brief Memory device that allows performance counters to be accessed at a
/// given memory region through two sets of memory-mapped registers: global
/// counters and per-hart counters.
class PerfCounterDevice : public MemoryDeviceBase {
public:
  PerfCounterDevice(RefSiDevice &soc);
  virtual ~PerfCounterDevice();

  uint64_t* get_perf_counters(unit_id_t unit_id);

  size_t mem_size() const override;

  bool load(reg_t addr, size_t len, uint8_t* bytes, unit_id_t unit_id) override;
  bool store(reg_t addr, size_t len, const uint8_t* bytes,
             unit_id_t unit_id) override;

private:
  bool get_perf_counter_index(reg_t rel_addr, size_t &counter_idx,
                              bool &is_per_hart) const;

  RefSiDevice &soc;
  std::vector<uint64_t> global_counters;
};

#endif
