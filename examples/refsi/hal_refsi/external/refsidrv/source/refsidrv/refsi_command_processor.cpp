// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "refsidrv/refsi_command_processor.h"

#include "refsidrv/refsi_accelerator.h"
#include "refsidrv/refsi_device.h"
#include "refsidrv/refsi_memory.h"
#include "refsidrv/refsi_memory_window.h"

#include <time.h>
#include <sstream>

#if defined(__BIG_ENDIAN__)
#error Decoding CMP commands is currently only supported where the host is a little-endian system.
#endif

static float time_diff_in_sec(timespec &start, timespec &end) {
  return (float) (end.tv_sec - start.tv_sec) +
      ((float) ((end.tv_nsec - start.tv_nsec) * 1e-6f)) * 1e-3f;
}

RefSiCommandProcessor::RefSiCommandProcessor(RefSiDevice &soc) : soc(soc),
  debug(soc.getDebug()) {
  registers.resize(CMP_NUM_REGS);
}

void RefSiCommandProcessor::start(RefSiLock &lock) {
  if (started) {
    return;
  }
  if (debug) {
    fprintf(stderr, "[CMP] Starting.\n");
  }
  stopping = false;
  started = true;
  worker_thread = new std::thread(workerMain, this);
}

void RefSiCommandProcessor::stop(RefSiLock &lock) {
  if (!started) {
    return;
  }
  // Shut down the queue and wait for the worker thread to be done.
  if (debug) {
    fprintf(stderr, "[CMP] Requesting stop.\n");
  }
  stopping = true;
  dispatched.notify_all();
  lock.unlock();
  worker_thread->join();
  lock.lock();
  stopping = false;
  delete worker_thread;
  worker_thread = nullptr;
}

void RefSiCommandProcessor::enqueueRequest(RefSiCommandRequest request,
                                           RefSiLock &lock) {
  if (!started) {
    start(lock);
  }

  // Wait for the queue to have a free space for the request.
  while (requests.size() > max_requests) {
    executed.wait(lock);
  }

  // Enqueue the request and notify the worker thread.
  requests.push_back(request);
  dispatched.notify_all();
}

void RefSiCommandProcessor::waitEmptyQueue(RefSiLock &lock) {
  while (!requests.empty()) {
    executed.wait(lock);
  }
}

void RefSiCommandProcessor::workerMain(RefSiCommandProcessor *cmp) {
  RefSiLock lock(cmp->soc.getLock());
  auto &requests = cmp->requests;
  while (true) {
    if (cmp->stopping) {
      if (cmp->debug) {
        fprintf(stderr, "[CMP] Stopping.\n");
      }
      break;
    }

    for (auto I = requests.begin(); I != requests.end();) {
      // Execute the request.
      timespec start, end;
      if (cmp->debug) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        fprintf(stderr, "[CMP] Starting to execute command buffer at 0x%zx.\n",
                I->command_buffer_addr);
      }
      cmp->execute(*I, lock);
      if (cmp->debug) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        fprintf(stderr, "[CMP] Finished executing command buffer in "
                "%0.3f s\n", time_diff_in_sec(start, end));
      }

      // Remove the request from the queue.
      I = requests.erase(I);

      // Notify clients that a request has been executed.
      cmp->executed.notify_all();
    }

    // Wait for something to happen:
    //   1) A command request has been dispatched
    //   2) The command processor is shutting down
    cmp->dispatched.wait(lock);
  }
}

static const char *getOpcodeName(refsi_cmp_command_id opcode) {
  switch (opcode) {
    default:
      return nullptr;
    case CMP_FINISH:
      return "FINISH";
    case CMP_NOP:
      return "NOP";
    case CMP_WRITE_REG64:
      return "WRITE_REG64";
    case CMP_LOAD_REG64:
      return "LOAD_REG64";
    case CMP_STORE_REG64:
      return "STORE_REG64";
    case CMP_STORE_IMM64:
      return "STORE_IMM64";
    case CMP_COPY_MEM64:
      return "COPY_MEM64";
    case CMP_RUN_KERNEL_SLICE:
      return "RUN_KERNEL_SLICE";
    case CMP_RUN_INSTANCES:
      return "RUN_INSTANCES";
    case CMP_SYNC_CACHE:
      return "SYNC_CACHE";
  }
}

std::string
RefSiCommandProcessor::getRegisterName(refsi_cmp_register_id reg_id) {
  switch (reg_id) {
  default:
    break;
  case CMP_REG_SCRATCH:
    return "SCRATCH";
  case CMP_REG_ENTRY_PT_FN:
    return "ENTRY_PT_FN";
  case CMP_REG_KUB_DESC:
    return "KUB_DESC";
  case CMP_REG_KARGS_INFO:
    return "KARGS_INFO";
  case CMP_REG_TSD_INFO:
    return "TSD_INFO";
  case CMP_REG_STACK_TOP:
    return "STACK_TOP";
  case CMP_REG_RETURN_ADDR:
    return "RETURN_ADDR";
  }

  // Handle register arrays specially.
  if ((reg_id >= CMP_REG_WINDOW_BASE0) && (reg_id <= CMP_REG_WINDOW_SCALEn)) {
    refsi_cmp_register_id canon_reg_id;
    uint32_t window_id = 0;
    if (RefSiMemoryWindow::splitCmpRegister(reg_id, canon_reg_id, window_id)) {
      std::string prefix;
      switch (canon_reg_id) {
      case CMP_REG_WINDOW_BASE0:
        prefix = "WINDOW_BASE";
        break;
      case CMP_REG_WINDOW_TARGET0:
        prefix = "WINDOW_TARGET";
        break;
      case CMP_REG_WINDOW_MODE0:
        prefix = "WINDOW_MODE";
        break;
      case CMP_REG_WINDOW_SCALE0:
        prefix = "WINDOW_SCALE";
        break;
      }
      return prefix + std::to_string(window_id);
    }
  }

  return std::string("UNKNOWN_") + std::to_string((uint32_t)reg_id);
}

std::string RefSiCommandProcessor::formatDeviceAddress(refsi_addr_t address) {
  // Format DMA register addresses specially.
  if ((address >= dma_io_base) && (address < (dma_io_base + dma_io_size))) {
    uint32_t reg_idx = REFSI_DMA_GET_REG(dma_io_base, address);
    switch (reg_idx) {
    case REFSI_REG_DMACTRL:
      return "DMA_CTRL";
    case REFSI_REG_DMASTARTSEQ:
      return "DMA_START_SEQ";
    case REFSI_REG_DMADONESEQ:
      return "DMA_DONE_SEQ";
    case REFSI_REG_DMASRCADDR:
      return "DMA_SRC_ADDR";
    case REFSI_REG_DMADSTADDR:
      return "DMA_DST_ADDR";
    case REFSI_REG_DMAXFERSIZE0 + 0:
      return "DMA_XFER_SIZE0";
    case REFSI_REG_DMAXFERSIZE0 + 1:
      return "DMA_XFER_SIZE1";
    case REFSI_REG_DMAXFERSIZE0 + 2:
      return "DMA_XFER_SIZE2";
    case REFSI_REG_DMAXFERSRCSTRIDE0 + 0:
      return "DMA_XFER_SRC_STRIDE0";
    case REFSI_REG_DMAXFERSRCSTRIDE0 + 1:
      return "DMA_XFER_SRC_STRIDE1";
    case REFSI_REG_DMAXFERDSTSTRIDE0 + 0:
      return "DMA_XFER_DST_STRIDE0";
    case REFSI_REG_DMAXFERDSTSTRIDE0 + 1:
      return "DMA_XFER_DST_STRIDE1";
    }
  }

  if (address == perf_counters_io_base) {
    return "PERF_COUNTERS";
  }

  // Format 'unknown' addresses as hex.
  std::stringstream ss;
  ss << "0x" << std::hex << address;
  return ss.str();
}

refsi_result RefSiCommandProcessor::execute(RefSiCommandRequest request,
                                            RefSiLock &lock) {
  // Retrieve a pointer to the command buffer area and divide it into 64-bit
  // chunks.
  uint64_t *command_buffer = (uint64_t *)soc.getMemory().addr_to_mem(
      request.command_buffer_addr, request.command_buffer_size,
      make_unit(unit_kind::cmp));
  if (!command_buffer) {
    return refsi_failure;
  }
  size_t total_chunks = request.command_buffer_size / sizeof(uint64_t);

  // Decode commands in the command buffer.
  refsi_result result = refsi_success;
  size_t pos = 0;
  while (pos < total_chunks) {
    // Decode the command header.
    RefSiCommandContext cmd(lock);
    result = refsiDecodeCMPCommand(command_buffer[pos], &cmd.opcode,
                                   &cmd.num_chunks, &cmd.inline_chunk);
    if (refsi_success != result) {
      return result;
    }
    /*if (debug) {
      const char *op_name = getOpcodeName(cmd.opcode);
      if (op_name) {
        fprintf(stderr, "[CMP] Decoded CMP_%s command.\n", op_name);
      } else {
        fprintf(stderr, "[CMP] Decoded command with opcode %d.\n", cmd.opcode);
      }
    }*/
    pos++;
    cmd.chunks = &command_buffer[pos];

    // Execute the command.
    result = executeCommand(cmd);
    if (refsi_success != result) {
      // Any command failure aborts execution of the command buffer.
      return result;
    } else if (cmd.opcode == CMP_FINISH) {
      break;
    }
    pos += cmd.num_chunks;
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeCommand(RefSiCommandContext &cmd) {
  switch (cmd.opcode) {
    default:
      return refsi_failure;
    case CMP_FINISH:
    case CMP_NOP:
      if (debug) {
        fprintf(stderr, "[CMP] CMP_%s\n", getOpcodeName(cmd.opcode));
      }
      return refsi_success;
    case CMP_WRITE_REG64:
      return executeWRITE_REG64(cmd);
    case CMP_LOAD_REG64:
      return executeLOAD_REG64(cmd);
    case CMP_STORE_REG64:
      return executeSTORE_REG64(cmd);
    case CMP_STORE_IMM64:
      return executeSTORE_IMM64(cmd);
    case CMP_COPY_MEM64:
      return executeCOPY_MEM64(cmd);
    case CMP_RUN_KERNEL_SLICE:
      return executeRUN_KERNEL_SLICE(cmd);
    case CMP_RUN_INSTANCES:
      return executeRUN_INSTANCES(cmd);
    case CMP_SYNC_CACHE:
      return executeSYNC_CACHE(cmd);
  }
}

refsi_result RefSiCommandProcessor::executeWRITE_REG64(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 1) {
    return refsi_failure;
  }
  refsi_cmp_register_id reg_idx = (refsi_cmp_register_id)cmd.inline_chunk;
  uint64_t imm_val = cmd.chunks[0];
  if (reg_idx >= registers.size()) {
    return refsi_failure;
  } else if ((reg_idx >= CMP_REG_WINDOW_BASE0) &&
             (reg_idx <= CMP_REG_WINDOW_SCALEn)) {
    RefSiMemoryController &mem_ctl = soc.getMemory();
    refsi_result result = mem_ctl.handleWindowRegWrite(reg_idx, imm_val);
    if (result != refsi_success) {
      return result;
    }
  }
  registers[reg_idx] = imm_val;
  if (debug) {
    std::string reg_name = getRegisterName(reg_idx);
    fprintf(stderr, "[CMP] CMP_WRITE_REG64(%s, 0x%zx)\n", reg_name.c_str(),
            imm_val);
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeLOAD_REG64(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 1) {
    return refsi_failure;
  }
  refsi_cmp_register_id reg_idx = (refsi_cmp_register_id)cmd.inline_chunk;
  uint64_t src_addr = cmd.chunks[0];
  if (reg_idx >= registers.size()) {
    return refsi_failure;
  }
  uint64_t val = 0;
  if (!soc.getMemory().load(src_addr, sizeof(uint64_t), (uint8_t *)&val,
                            make_unit(unit_kind::cmp))) {
    return refsi_failure;
  }
  registers[reg_idx] = val;
  if (debug) {
    std::string reg_name = getRegisterName(reg_idx);
    std::string src_addr_str = formatDeviceAddress(src_addr);
    fprintf(stderr, "[CMP] CMP_LOAD_REG64(%s, %s) -> 0x%zx\n",
            reg_name.c_str(), src_addr_str.c_str(), val);
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeSTORE_REG64(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 1) {
    return refsi_failure;
  }
  refsi_cmp_register_id reg_idx = (refsi_cmp_register_id)cmd.inline_chunk;
  uint64_t dst_addr = cmd.chunks[0];
  if (reg_idx >= registers.size()) {
    return refsi_failure;
  }
  uint64_t val = registers[reg_idx];
  if (!soc.getMemory().store(dst_addr, sizeof(uint64_t), (uint8_t *)&val,
                             make_unit(unit_kind::cmp))) {
    return refsi_failure;
  }
  if (debug) {
    std::string reg_name = getRegisterName(reg_idx);
    std::string dst_addr_str = formatDeviceAddress(dst_addr);
    fprintf(stderr, "[CMP] CMP_STORE_REG64(%s, %s) -> 0x%zx\n",
            reg_name.c_str(), dst_addr_str.c_str(), val);
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeSTORE_IMM64(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 1) {
    return refsi_failure;
  }
  uint64_t dest_addr = cmd.inline_chunk;
  uint64_t imm_val = cmd.chunks[0];
  if (!soc.getMemory().store(dest_addr, sizeof(uint64_t), (uint8_t *)&imm_val,
                             make_unit(unit_kind::cmp))) {
    return refsi_failure;
  }
  if (debug) {
    std::string dest_addr_str = formatDeviceAddress(dest_addr);
    fprintf(stderr, "[CMP] CMP_STORE_IMM64(0x%zx, %s)\n", imm_val,
            dest_addr_str.c_str());
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeCOPY_MEM64(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 3) {
    return refsi_failure;
  }
  uint32_t count = cmd.inline_chunk;
  uint64_t src_addr = cmd.chunks[0];
  uint64_t dst_addr = cmd.chunks[1];
  constexpr const size_t reg_size = sizeof(uint64_t);
  if (src_addr % reg_size) {
    return refsi_failure;
  }

  // Locate the source and destination devices and ensure that all registers can
  // be copied
  uint64_t copy_size = count * reg_size;
  uint64_t src_target_offset = 0;
  uint64_t dst_target_offset = 0;
  MemoryDevice *src_device = soc.getMemory().find_device(src_addr,
                                                         src_target_offset);
  MemoryDevice *dst_device = soc.getMemory().find_device(dst_addr,
                                                         dst_target_offset);
  if (!src_device || (src_target_offset + copy_size) > src_device->mem_size()) {
    return refsi_failure;
  } else if (!dst_device ||
             (dst_target_offset + copy_size) > dst_device->mem_size()) {
    return refsi_failure;
  }

  // Copy registers one by one. I/O devices cannot access more than one register
  // at a time.
  unit_id_t unit_id = (unit_id_t)cmd.chunks[2];
  for (uint32_t i = 0; i < count; i++) {
    uint64_t reg_src_addr = src_target_offset + (i * reg_size);
    uint64_t reg_dst_addr = dst_target_offset + (i * reg_size);
    uint64_t val = 0;
    if (!src_device->load(reg_src_addr, reg_size, (uint8_t *)&val, unit_id)) {
      return refsi_failure;
    }
    if (!dst_device->store(reg_dst_addr, reg_size, (const uint8_t *)&val,
                           make_unit(unit_kind::cmp))) {
      return refsi_failure;
    }
  }

  if (debug) {
    std::string src_addr_str = formatDeviceAddress(src_addr);
    std::string dst_addr_str = formatDeviceAddress(dst_addr);
    std::string unit_str = format_unit(unit_id);
    fprintf(stderr, "[CMP] CMP_COPY_MEM64(%s@%s, %s, %d)\n",
            src_addr_str.c_str(), unit_str.c_str(), dst_addr_str.c_str(),
            count);
  }
  return refsi_success;
}

refsi_result RefSiCommandProcessor::executeRUN_KERNEL_SLICE(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 2) {
    return refsi_failure;
  }

  uint32_t max_harts = cmd.inline_chunk & 0xff;
  uint64_t num_instances = cmd.chunks[0];
  uint64_t slice_id = cmd.chunks[1];
  if (debug) {
    fprintf(stderr,
            "[CMP] CMP_RUN_KERNEL_SLICE(n=%zd, slice_id=%zd, "
            "max_harts=%d)\n",
            num_instances, slice_id, max_harts);
  }
  uint64_t entry_point =
      CMP_GET_ENTRY_POINT_ADDR(registers[CMP_REG_ENTRY_PT_FN]);
  uint64_t kub_addr = CMP_GET_KUB_ADDR(registers[CMP_REG_KUB_DESC]);
  uint32_t kub_size = (uint32_t)CMP_GET_KUB_SIZE(registers[CMP_REG_KUB_DESC]);
  uint32_t kargs_size =
      (uint32_t)CMP_GET_KARGS_SIZE(registers[CMP_REG_KARGS_INFO]);
  uint32_t kargs_offset =
      (uint32_t)CMP_GET_KARGS_OFFSET(registers[CMP_REG_KARGS_INFO]);
  uint32_t tsd_size = (uint32_t)CMP_GET_TSD_SIZE(registers[CMP_REG_TSD_INFO]);
  uint32_t tsd_offset =
      (uint32_t)CMP_GET_TSD_OFFSET(registers[CMP_REG_TSD_INFO]);
  uint64_t stack_top = registers[CMP_REG_STACK_TOP];
  uint64_t return_addr = registers[CMP_REG_RETURN_ADDR];
  (void)kub_size;
  (void)kargs_size;
  (void)kargs_offset;

  // Prepare per-hart data.
  size_t num_harts = (max_harts > 0) ? max_harts : num_harts_per_core;
  size_t tcdm_hart_size_per_hart = tcdm_hart_size / num_harts;
  auto getTCDMHartAddress = [&](size_t hart_idx, reg_t address) {
    return (hart_idx * tcdm_hart_size_per_hart) + address + tcdm_hart_base;
  };
  std::vector<hart_state_entry> per_hart_data(num_harts);
  for (size_t hart_id = 0; hart_id < num_harts; hart_id++) {
    hart_state_entry &hart_data(per_hart_data[hart_id]);
    if (stack_top != 0) {
      hart_data.stack_top_addr = stack_top;
    } else {
      hart_data.stack_top_addr = getTCDMHartAddress(hart_id,
                                                    tcdm_hart_size_per_hart);
    }
    hart_data.extra_args.push_back(slice_id);
    hart_data.extra_args.push_back(kub_addr);

    reg_t ktb_addr = 0;
    if (tsd_size > 0) {
      // Copy thread-specific data to this thread's Kernel Thread Block.
      unit_id_t unit = make_unit(unit_kind::acc_hart, hart_id);
      ktb_addr = getTCDMHartAddress(hart_id, 0);
      uint8_t *ktb = soc.getMemory().addr_to_mem(ktb_addr, tsd_size, unit);
      uint8_t *tsd =
          soc.getMemory().addr_to_mem(kub_addr + tsd_offset, tsd_size, unit);
      if (!ktb || !tsd) {
        return refsi_failure;
      }
      memcpy(ktb, tsd, tsd_size);
    }
    hart_data.extra_args.push_back(ktb_addr);
  }

  // Run the kernel.
  return soc.getAccelerator().runKernelSlice(num_instances, entry_point,
                                             return_addr, num_harts,
                                             per_hart_data.data());
}

refsi_result RefSiCommandProcessor::executeRUN_INSTANCES(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks < 1) {
    return refsi_failure;
  }

  const uint32_t max_extra_args = 7;
  uint32_t max_harts = cmd.inline_chunk & 0xff;
  uint32_t num_extra_args = (cmd.inline_chunk >> 8) & 0x07;
  if ((num_extra_args > max_extra_args) ||
      (cmd.num_chunks != (num_extra_args + 1))) {
    return refsi_failure;
  }
  uint64_t num_instances = cmd.chunks[0];
  if (debug) {
    fprintf(stderr, "[CMP] CMP_RUN_INSTANCES(n=%zd, max_harts=%d",
            num_instances, max_harts);
    for (uint32_t i = 0; i < num_extra_args; i++) {
      fprintf(stderr, ", 0x%zx", cmd.chunks[i + 1]);
    }
    fprintf(stderr, ")\n");
  }
  uint64_t entry_point =
      CMP_GET_ENTRY_POINT_ADDR(registers[CMP_REG_ENTRY_PT_FN]);
  uint64_t stack_top = registers[CMP_REG_STACK_TOP];
  uint64_t return_addr = registers[CMP_REG_RETURN_ADDR];

  // Prepare per-hart data.
  size_t num_harts = (max_harts > 0) ? max_harts : num_harts_per_core;
  std::vector<hart_state_entry> per_hart_data(num_harts);
  for (size_t hart_id = 0; hart_id < num_harts; hart_id++) {
    hart_state_entry &hart_data(per_hart_data[hart_id]);
    hart_data.stack_top_addr = stack_top;
    for (size_t i = 0; i < num_extra_args; i++) {
      hart_data.extra_args.push_back(cmd.chunks[i + 1]);
    }
  }

  // Run the kernel.
  return soc.getAccelerator().runKernelSlice(num_instances, entry_point,
                                             return_addr, num_harts,
                                             per_hart_data.data());
}

refsi_result RefSiCommandProcessor::executeSYNC_CACHE(
    RefSiCommandContext &cmd) {
  if (cmd.num_chunks != 0) {
    return refsi_failure;
  }

  uint32_t flags = cmd.inline_chunk & (CMP_CACHE_SYNC_ACC_DCACHE |
                                       CMP_CACHE_SYNC_ACC_ICACHE);
  if (debug) {
    fprintf(stderr,
            "[CMP] CMP_SYNC_CACHE(flags=0x%x)\n", flags);
  }
  return soc.getAccelerator().syncCache(flags);
}
