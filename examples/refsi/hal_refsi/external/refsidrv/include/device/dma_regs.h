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

#ifndef _REFSIDRV_DEVICE_DMA_REGS_H
#define _REFSIDRV_DEVICE_DMA_REGS_H

#define REFSI_REG_DMACTRL               0x00
#define REFSI_REG_DMASTARTSEQ           0x01
#define REFSI_REG_DMADONESEQ            0x02
#define REFSI_REG_DMASRCADDR            0x03
#define REFSI_REG_DMADSTADDR            0x04
#define REFSI_REG_DMAXFERSIZE0          0x05
#define REFSI_REG_DMAXFERSRCSTRIDE0     0x08
#define REFSI_REG_DMAXFERDSTSTRIDE0     0x0a

#define REFSI_DMA_REG_ADDR(base, reg)  ((base) + ((reg) << 3))
#define REFSI_DMA_GET_REG(base, addr)  (((addr) - (base)) >> 3)
#define REFSI_DMA_NUM_REGS              0x20

#define REFSI_DMA_IO_ADDRESS            0x20002000
#define REFSI_DMA_IO_END_ADDRESS        0x20002100

#define REFSI_DMA_START                 0x01

#define REFSI_DMA_1D                    0x10
#define REFSI_DMA_2D                    0x20
#define REFSI_DMA_3D                    0x30
#define REFSI_DMA_DIM_MASK              0x30

#define REFSI_DMA_STRIDE_NONE           0x00
#define REFSI_DMA_STRIDE_DST            0x40
#define REFSI_DMA_STRIDE_SRC            0x80
#define REFSI_DMA_STRIDE_BOTH           ((REFSI_DMA_STRIDE_DST) | (REFSI_DMA_STRIDE_SRC))
#define REFSI_DMA_STRIDE_MODE_MASK      REFSI_DMA_STRIDE_BOTH

#endif // _REFSIDRV_DEVICE_DMA_REGS_H
