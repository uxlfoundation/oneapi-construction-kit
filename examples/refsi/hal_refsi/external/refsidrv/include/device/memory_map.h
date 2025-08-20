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

#ifndef _REFSIDRV_DEVICE_MEMORY_MAP_H
#define _REFSIDRV_DEVICE_MEMORY_MAP_H

// 00001000-0000ffff: bootloader (60 KB)
#define REFSI_LOADER_ADDRESS             0x00001000
#define REFSI_LOADER_END_ADDRESS         0x00010000

// 00010000-003fffff: kernel (~4 MB)
#define REFSI_KERNEL_ADDRESS             0x00010000
#define REFSI_GLOBAL_MEM_ADDRESS         0x40000000

// Memory mapped I/O and special memory

// 10000000-101fffff: local memory (2 MB) shared between harts in a cluster
#define REFSI_LOCAL_MEM_ADDRESS          0x10000000
#define REFSI_LOCAL_MEM_END_ADDRESS      0x10200000

// 20002000-200020ff: per-hart DMA management registers (256 B)
#include "device/dma_regs.h"

// 20800000-20ffffff: hart private storage (8 MB) for stack and compute context
// CDE-32: revert this size to a more realistic value.
#define REFSI_HART_LOCAL_ADDRESS         0x20800000
#define REFSI_HART_LOCAL_END_ADDRESS     0x21000000

#define REFSI_CONTEXT_ADDRESS            (REFSI_HART_LOCAL_ADDRESS + 0x0000)

#endif // _REFSIDRV_DEVICE_MEMORY_MAP_H
