// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_REFSI_DEVICE_M_H
#define _REFSIDRV_REFSI_DEVICE_M_H

#include <memory>

#include "refsi_device.h"

struct RefSiCommandProcessor;
class DMADevice;
class PerfCounterDevice;

struct RefSiMDevice : public RefSiDevice {
public:
  RefSiMDevice();

  /// @brief Shut down the device.
  virtual ~RefSiMDevice();

  /// @brief Perform device initialization.
  refsi_result initialize() override;

  /// @brief Asynchronously execute a series of commands on the device.
  /// @param cb_addr Address of the command buffer in device memory.
  /// @param size Size of the command buffer, in bytes.
  refsi_result executeCommandBuffer(refsi_addr_t cb_addr, size_t size);

  /// @brief Wait for all previously enqueued command buffers to be finished.
  void waitForDeviceIdle();

 private:
  void preRunSim(slim_sim_t &sim);

  std::unique_ptr<RefSiCommandProcessor> cmp;
  RAMDevice *tcdm = nullptr;
  RAMDevice *dram = nullptr;
  DMADevice *dma_device = nullptr;
  PerfCounterDevice *perf_counter_device = nullptr;
};

#endif  // _REFSIDRV_REFSI_DEVICE_M_H
