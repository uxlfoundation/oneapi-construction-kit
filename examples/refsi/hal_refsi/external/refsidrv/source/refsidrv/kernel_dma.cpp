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

#include "kernel_dma.h"
#include "slim_sim.h"
#include "device/dma_regs.h"

DMADevice::~DMADevice() {
  for (auto &mapping : dma_reg_contents) {
    delete [] mapping.second;
  }
  dma_reg_contents.clear();
}

bool DMADevice::load(reg_t addr, size_t len, uint8_t *bytes,
                        unit_id_t unit_id) {
  switch (machine) {
  default:
    return false;
  case elf_machine::riscv_rv32:
    if (len != sizeof(uint32_t)) {
      return false;
    }
    break;
  case elf_machine::riscv_rv64:
    if (len != sizeof(uint64_t)) {
      return false;
    }
    break;
  }
  uint64_t val = 0;
  size_t dma_reg = 0;
  if (get_dma_reg(addr, dma_reg)) {
    if (read_dma_reg(dma_reg, &val, unit_id)) {
      if (len == sizeof(uint64_t)) {
        memcpy(bytes, &val, len);
      } else if (len == sizeof(uint32_t)) {
        uint32_t lo_val = (uint32_t)val;
        memcpy(bytes, &lo_val, len);
      }
      return true;
    }
  }
  return false;
}

bool DMADevice::store(reg_t addr, size_t len, const uint8_t *bytes,
                         unit_id_t unit_id) {
  uint64_t val;
  switch (machine) {
  default:
    return false;
  case elf_machine::riscv_rv32:
    if (len != sizeof(uint32_t)) {
      return false;
    }
    val = *(const uint32_t *)bytes;
    break;
  case elf_machine::riscv_rv64:
    if (len != sizeof(uint64_t)) {
      return false;
    }
    val = *(const uint64_t *)bytes;
    break;
  }
  size_t dma_reg = 0;
  if (get_dma_reg(addr, dma_reg)) {
    return write_dma_reg(dma_reg, val, unit_id);
  }
  return false;
}

size_t DMADevice::mem_size() const {
  return REFSI_DMA_NUM_REGS * sizeof(uint64_t);
}

uint64_t * DMADevice::get_dma_regs(unit_id_t unit_id) {
  uint64_t *contents = nullptr;
  auto it = dma_reg_contents.find(unit_id);
  if (it == dma_reg_contents.end()) {
    constexpr const size_t num_regs = REFSI_DMA_NUM_REGS;
    contents = new uint64_t[num_regs];
    memset(contents, 0, num_regs * sizeof(uint64_t));
    dma_reg_contents[unit_id] = contents;
  } else {
    contents = it->second;
  }
  return contents;
}

bool DMADevice::get_dma_reg(reg_t rel_addr, size_t &dma_reg) const {
  reg_t addr = rel_addr + base_addr;
  reg_t end_addr = base_addr + REFSI_DMA_NUM_REGS * sizeof(uint64_t);
  if ((addr >= base_addr) && (addr < end_addr)) {
    dma_reg = REFSI_DMA_GET_REG(base_addr, addr);
    return true;
  }
  dma_reg = 0;
  return false;
}

bool DMADevice::read_dma_reg(size_t dma_reg, uint64_t *val,
                                unit_id_t unit_id) {
  uint64_t *dma_regs = get_dma_regs(unit_id);
  *val = dma_regs[dma_reg];
  if (debug) {
    if (dma_reg == REFSI_REG_DMASTARTSEQ) {
      uint32_t xfer_id = (uint32_t)*val;
      fprintf(stderr, "dma_device_t::read_dma_reg() Most recent transfer "
              "ID: %d\n", xfer_id);
    }
  }
  return true;
}

bool DMADevice::write_dma_reg(size_t dma_reg, uint64_t val,
                                 unit_id_t unit_id) {
  uint64_t *dma_regs = get_dma_regs(unit_id);

  if (dma_reg == REFSI_REG_DMADONESEQ) {
    // Writing to DMADONESEQ has special behaviour. The current hart is blocked
    // until the transfer identified by val is complete.
    uint32_t xfer_id = (uint32_t)val;
    uint32_t last_done_id = (uint32_t)dma_regs[REFSI_REG_DMADONESEQ];
    // TODO Implement the waiting logic. This can be implemented in a similar
    // way as waiting for barriers. Waiting is not required at the moment, as
    // DMA transfers happen instantaneously from the perspective of the hart.
    if (debug) {
      fprintf(stderr, "dma_device_t::write_dma_reg() Waiting for transfer "
              "ID %d\n", xfer_id);
    }
    return last_done_id >= xfer_id;
  }

  // Determine the write mask for the register, i.e. which bits can be written
  // to by the user.
  uint64_t write_mask = ~0ull;
  if (dma_reg == REFSI_REG_DMACTRL) {
    // The LSB of DMACTRL always reads zero.
    write_mask = ~1ull;
  } else if (dma_reg == REFSI_REG_DMASTARTSEQ) {
    // DMASTARTSEQ is read-only.
    write_mask = 0;
  }
  if (!write_mask) {
    // Nothing to write.
    return true;
  }

  // Write the value to the register.
  reg_t to_write = val & write_mask;
  dma_regs[dma_reg] = to_write;

  // Provide feedback for register writes.
  if (debug) {
    switch (dma_reg) {
    default:
      fprintf(stderr, "dma_device_t::write_dma_reg() Wrote 0x%zx to register "
              "%zd\n", to_write, dma_reg);
      break;
    case REFSI_REG_DMASRCADDR:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set source address to "
              "0x%zx\n", to_write);
      break;
    case REFSI_REG_DMADSTADDR:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set destination address "
              "to 0x%zx\n", to_write);
      break;
    case REFSI_REG_DMAXFERSIZE0 + 0:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set transfer size[0] to "
              "0x%zx bytes\n", to_write);
      break;
    case REFSI_REG_DMAXFERSIZE0 + 1:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set transfer size[1] to "
              "0x%zx elements\n", to_write);
      break;
    case REFSI_REG_DMAXFERSIZE0 + 2:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set transfer size[2] to "
              "0x%zx elements\n", to_write);
      break;
    case REFSI_REG_DMAXFERSRCSTRIDE0 + 0:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set source stride[0] to "
                      "0x%zx bytes\n", to_write);
      break;
    case REFSI_REG_DMAXFERSRCSTRIDE0 + 1:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set source stride[1] to "
                      "0x%zx bytes\n", to_write);
      break;
    case REFSI_REG_DMAXFERDSTSTRIDE0 + 0:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set destination stride[0] "
                      "to 0x%zx bytes\n", to_write);
      break;
    case REFSI_REG_DMAXFERDSTSTRIDE0 + 1:
      fprintf(stderr, "dma_device_t::write_dma_reg() Set destination stride[1] "
                      "to 0x%zx bytes\n", to_write);
      break;
    case REFSI_REG_DMACTRL:
      break;
    }
  }

  // Trigger a DMA operation when the START bit is set.
  if ((dma_reg == REFSI_REG_DMACTRL) && (val & REFSI_DMA_START)) {
    return do_kernel_dma(unit_id);
  }

  return true;
}

bool DMADevice::do_kernel_dma(unit_id_t unit_id) {
  uint64_t *dma_regs = get_dma_regs(unit_id);

  // Get a pointer to the source buffer.
  reg_t src_addr = dma_regs[REFSI_REG_DMASRCADDR];
  uint8_t *src_mem = (uint8_t *)mem_if.addr_to_mem(src_addr, 0, unit_id);
  if (!src_mem) {
    // This should only happen for 'special' memory like hart-local memory or
    // ROM, neither of which are currently supported by in-kernel DMA.
    return false;
  }

  // Get a pointer to the destination buffer.
  reg_t dst_addr = dma_regs[REFSI_REG_DMADSTADDR];
  uint8_t *dst_mem = (uint8_t *)mem_if.addr_to_mem(dst_addr, 0, unit_id);
  if (!dst_mem) {
    // This should only happen for 'special' memory like hart-local memory or
    // ROM, neither of which are currently supported by in-kernel DMA.
    return false;
  }

  // Validate the transfer dimension.
  reg_t dim = dma_regs[REFSI_REG_DMACTRL] & REFSI_DMA_DIM_MASK;
  if (dim == REFSI_DMA_1D) {
    return do_kernel_dma_1d(unit_id, dst_mem, src_mem);
  } else if (dim == REFSI_DMA_2D) {
    return do_kernel_dma_2d(unit_id, dst_mem, src_mem);
  } else if (dim == REFSI_DMA_3D) {
    return do_kernel_dma_3d(unit_id, dst_mem, src_mem);
  } else {
    if (debug) {
      fprintf(stderr, "dma_device_t::do_kernel_dma() Invalid dimension: %zd\n",
              dim);
    }
    return false;
  }
}

bool DMADevice::do_kernel_dma_1d(unit_id_t unit_id, uint8_t *dst_mem,
                                    uint8_t *src_mem) {
  uint64_t *dma_regs = get_dma_regs(unit_id);

  // Retrieve the size of the transfer.
  reg_t size = dma_regs[REFSI_REG_DMAXFERSIZE0];
  if (size == 0) {
    return true;
  }

  // Validate the stride mode.
  reg_t stride_mode = dma_regs[REFSI_REG_DMACTRL] & REFSI_DMA_STRIDE_MODE_MASK;
  if (stride_mode != REFSI_DMA_STRIDE_NONE) {
    // Strides are not supported for 1D transfers.
    if (debug) {
      fprintf(stderr, "dma_device_t::do_kernel_dma_1d() Unsupported stride "
                      "mode: 0x%zx\n", stride_mode);
    }
    return false;
  }

  // Allocate a new ID for the transfer.
  uint32_t xfer_id = (uint32_t)dma_regs[REFSI_REG_DMASTARTSEQ] + 1;
  dma_regs[REFSI_REG_DMASTARTSEQ] = xfer_id;

  // Perform the transfer.
  if (debug) {
    fprintf(stderr, "dma_device_t::do_kernel_dma_1d() Started transfer with ID "
            "%d\n", xfer_id);
  }
  memcpy(dst_mem, src_mem, size);

  // Mark the transfer as completed.
  dma_regs[REFSI_REG_DMADONESEQ] = xfer_id;

  return true;
}

static const char * get_stride_mode_text(reg_t stride_mode) {
  switch (stride_mode) {
  default:
  case REFSI_DMA_STRIDE_NONE:
    return "sequential";
  case REFSI_DMA_STRIDE_SRC:
    return "gather";
  case REFSI_DMA_STRIDE_DST:
    return "scatter";
  case REFSI_DMA_STRIDE_BOTH:
    return "multi-stride";
  }
}

bool DMADevice::do_kernel_dma_2d(unit_id_t unit_id, uint8_t *dst_mem,
                                    uint8_t *src_mem) {
  uint64_t *dma_regs = get_dma_regs(unit_id);
  reg_t sizes[2];
  reg_t src_strides[2];
  reg_t dst_strides[2];

  // Retrieve the size of the transfer.
  sizes[0] = dma_regs[REFSI_REG_DMAXFERSIZE0 + 0];
  sizes[1] = dma_regs[REFSI_REG_DMAXFERSIZE0 + 1];
  if (sizes[0] == 0 || sizes[1] == 0) {
    return true;
  }
  for (uint i = 0; i < 2; i++) {
    src_strides[i] = dst_strides[i] = sizes[i];
  }

  // Retrieve the stride mode.
  reg_t stride_mode = dma_regs[REFSI_REG_DMACTRL] & REFSI_DMA_STRIDE_MODE_MASK;
  const char *mode_text = get_stride_mode_text(stride_mode);

  // Retrieve the source stride of the transfer.
  if (stride_mode & REFSI_DMA_STRIDE_SRC) {
    src_strides[0] = dma_regs[REFSI_REG_DMAXFERSRCSTRIDE0];
    if ((src_strides[0] < sizes[0]) && (src_strides[0] != 0)) {
      if (debug) {
        fprintf(stderr, "dma_device_t::do_kernel_dma_2d() Invalid source "
                        "stride\n");
      }
      return false;
    }
  }

  // Retrieve the destination stride of the transfer.
  if (stride_mode & REFSI_DMA_STRIDE_DST) {
    dst_strides[0] = dma_regs[REFSI_REG_DMAXFERDSTSTRIDE0];
    if (dst_strides[0] < sizes[0]) {
      if (debug) {
        fprintf(stderr, "dma_device_t::do_kernel_dma_2d() Invalid destination "
                        "stride\n");
      }
      return false;
    }
  }

  // Allocate a new ID for the transfer.
  uint32_t xfer_id = (uint32_t)dma_regs[REFSI_REG_DMASTARTSEQ] + 1;
  dma_regs[REFSI_REG_DMASTARTSEQ] = xfer_id;

  // Perform the transfer.
  if (debug) {
    fprintf(stderr, "dma_device_t::do_kernel_dma_2d() Started %s transfer with "
                    "ID %d\n", mode_text, xfer_id);
  }
  for (uint y = 0; y < sizes[1]; y++) {
    memcpy(dst_mem, src_mem, sizes[0]);
    dst_mem += dst_strides[0];
    src_mem += src_strides[0];
  }

  // Mark the transfer as completed.
  dma_regs[REFSI_REG_DMADONESEQ] = xfer_id;

  return true;
}

bool DMADevice::do_kernel_dma_3d(unit_id_t unit_id, uint8_t *dst_mem,
                                    uint8_t *src_mem) {
  uint64_t *dma_regs = get_dma_regs(unit_id);
  reg_t sizes[3];
  reg_t src_strides[3];
  reg_t dst_strides[3];

  // Retrieve the size of the transfer.
  sizes[0] = dma_regs[REFSI_REG_DMAXFERSIZE0 + 0];
  sizes[1] = dma_regs[REFSI_REG_DMAXFERSIZE0 + 1];
  sizes[2] = dma_regs[REFSI_REG_DMAXFERSIZE0 + 2];
  if (sizes[0] == 0 || sizes[1] == 0|| sizes[2] == 0) {
    return true;
  }
  for (uint i = 0; i < 3; i++) {
    src_strides[i] = dst_strides[i] = sizes[i];
  }

  // Retrieve the stride mode.
  reg_t stride_mode = dma_regs[REFSI_REG_DMACTRL] & REFSI_DMA_STRIDE_MODE_MASK;
  const char *mode_text = get_stride_mode_text(stride_mode);

  // Retrieve the source stride of the transfer.
  if (stride_mode & REFSI_DMA_STRIDE_SRC) {
    src_strides[0] = dma_regs[REFSI_REG_DMAXFERSRCSTRIDE0 + 0];
    src_strides[1] = dma_regs[REFSI_REG_DMAXFERSRCSTRIDE0 + 1];
    if ((src_strides[0] < sizes[0]) || (src_strides[1] < sizes[1])) {
      if (debug) {
        fprintf(stderr, "dma_device_t::do_kernel_dma_3d() Invalid source "
                        "stride\n");
      }
      return false;
    }
  }

  // Retrieve the destination stride of the transfer.
  if (stride_mode & REFSI_DMA_STRIDE_DST) {
    dst_strides[0] = dma_regs[REFSI_REG_DMAXFERDSTSTRIDE0 + 0];
    dst_strides[1] = dma_regs[REFSI_REG_DMAXFERDSTSTRIDE0 + 1];
    if ((dst_strides[0] < sizes[0]) || (dst_strides[1] < sizes[1])) {
      if (debug) {
        fprintf(stderr, "dma_device_t::do_kernel_dma_3d() Invalid destination "
                        "stride\n");
      }
      return false;
    }
  }

  // Allocate a new ID for the transfer.
  uint32_t xfer_id = (uint32_t)dma_regs[REFSI_REG_DMASTARTSEQ] + 1;
  dma_regs[REFSI_REG_DMASTARTSEQ] = xfer_id;

  // Perform the transfer.
  if (debug) {
    fprintf(stderr, "dma_device_t::do_kernel_dma_3d() Started %s transfer with "
                    "ID %d\n", mode_text, xfer_id);
  }
  for (uint z = 0; z < sizes[2]; z++) {
    for (uint y = 0; y < sizes[1]; y++) {
      memcpy(dst_mem, src_mem, sizes[0]);
      dst_mem += dst_strides[0];
      src_mem += src_strides[0];
    }
    dst_mem += dst_strides[1] - (sizes[1] * dst_strides[0]);
    src_mem += src_strides[1] - (sizes[1] * src_strides[0]);
  }

  // Mark the transfer as completed.
  dma_regs[REFSI_REG_DMADONESEQ] = xfer_id;

  return true;
}
