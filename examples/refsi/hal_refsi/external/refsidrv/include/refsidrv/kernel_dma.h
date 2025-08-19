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

#ifndef _REFSIDRV_REFSI_KERNEL_DMA_H
#define _REFSIDRV_REFSI_KERNEL_DMA_H

#include <map>
#include <memory>

#include "devices.h"
#include "elf_loader.h"
#include "common_devices.h"

class DMADevice : public MemoryDeviceBase {
public:
  DMADevice(elf_machine machine, reg_t base_addr, MemoryInterface &mem_if,
            bool debug = false)
     : machine(machine), base_addr(base_addr), mem_if(mem_if), debug(debug) {}
  virtual ~DMADevice();

  reg_t get_base() const { return base_addr; }

  uint64_t* get_dma_regs(unit_id_t unit_id);

  size_t mem_size() const override;

  bool load(reg_t addr, size_t len, uint8_t* bytes, unit_id_t unit_id) override;
  bool store(reg_t addr, size_t len, const uint8_t* bytes,
             unit_id_t unit_id) override;

private:
  bool get_dma_reg(reg_t rel_addr, size_t &dma_reg) const;
  bool read_dma_reg(size_t dma_reg, uint64_t *val, unit_id_t unit_id);
  bool write_dma_reg(size_t dma_reg, uint64_t val, unit_id_t unit_id);
  bool do_kernel_dma(unit_id_t unit_id);
  bool do_kernel_dma_1d(unit_id_t unit_id, uint8_t *dst_mem, uint8_t *src_mem);
  bool do_kernel_dma_2d(unit_id_t unit_id, uint8_t *dst_mem, uint8_t *src_mem);
  bool do_kernel_dma_3d(unit_id_t unit_id, uint8_t *dst_mem, uint8_t *src_mem);

  elf_machine machine;
  reg_t base_addr;
  MemoryInterface &mem_if;
  bool debug;
  std::map<unit_id_t, uint64_t *> dma_reg_contents;
};

#endif
