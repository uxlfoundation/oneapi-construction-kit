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

/// @file
///
/// @brief Device Hardware Abstraction Layer extended RISC-V Interface.

#ifndef RISCV_HAL_RISCV_H_INCLUDED
#define RISCV_HAL_RISCV_H_INCLUDED

#include <cstdint>
#include <string>

#include "hal.h"
#include "hal_types.h"

namespace riscv {
/// @addtogroup riscv
/// @{

enum riscv_extension_t : uint64_t {
  rv_extension_M = (1 << 0x0),  // Integer Multiplication and Division
  rv_extension_A = (1 << 0x1),  // Atomic Instructions
  rv_extension_F = (1 << 0x2),  // Single-Precision Floating-Point
  rv_extension_D = (1 << 0x3),  // Double-Precision Floating-Point
  rv_extension_G = (rv_extension_M | rv_extension_A | rv_extension_F |
                    rv_extension_D),  // Shorthand for base and above extensions
  rv_extension_Q = (1 << 0x5),        // Quad-Precision Floating-Point
  rv_extension_L = (1 << 0x6),        // Decimal Floating-Point
  rv_extension_C = (1 << 0x7),        // Compressed Instructions
  rv_extension_B = (1 << 0x8),        // Bit Manipulation
  rv_extension_J = (1 << 0x9),        // Dynamically Translated Languages
  rv_extension_T = (1 << 0xa),        // Transactional Memory
  rv_extension_P = (1 << 0xb),        // Packed-SIMD Instructions
  rv_extension_V = (1 << 0xc),        // Vector Operations
  rv_extension_N = (1 << 0xd),        // User-Level Interrupts
  rv_extension_H = (1 << 0xe),        // Hypervisor
  rv_extension_E = (1 << 0xf),        // 16 rather than 32 GPRs
  rv_extension_Zba = (1 << 0x10),     // Bit-Manipulation (Address Generation)
  rv_extension_Zbb = (1 << 0x11),     // Bit-Manipulation (Basic)
  rv_extension_Zbc = (1 << 0x12),     // Bit-Manipulation (Carry-less mul)
  rv_extension_Zbs = (1 << 0x13),     // Bit-Manipulation (Single bit)
  rv_extension_Zfh = (1 << 0x14),     // Half-precision floating point
  // Cryptography
  rv_extension_Zbkb = (1 << 0x15),   // Bitmanip instructions for Cryptography
  rv_extension_Zbkc = (1 << 0x16),   // Carry-less multiply instructions
  rv_extension_Zbkx = (1 << 0x17),   // Crossbar permutation instructions
  rv_extension_Zknd = (1 << 0x18),   // NIST Suite: AES Decryption
  rv_extension_Zkne = (1 << 0x19),   // NIST Suite: AES Encryption
  rv_extension_Zknh = (1 << 0x1a),   // NIST Suite: Hash Function Instructions
  rv_extension_Zksed = (1 << 0x1b),  // ShangMi Suite: SM4 Block Cipher Insts
  rv_extension_Zksh = (1 << 0x1c),   // ShangMi Suite: SM3 Hash Function Insts
  rv_extension_Zkr = (1 << 0x1d),    // Entropy Source Extension
  rv_extension_Zkt = (1 << 0x1e),    // Data Independent Execution Latency
  rv_extension_Zkn = rv_extension_Zbkb | rv_extension_Zbkc | rv_extension_Zbkx |
                     rv_extension_Zkne | rv_extension_Zknd |
                     rv_extension_Zknh,  // NIST Algorithm Suite
  rv_extension_Zks = rv_extension_Zbkb | rv_extension_Zbkc | rv_extension_Zbkx |
                     rv_extension_Zksed |
                     rv_extension_Zksh,  // ShangMi Algorithm Suite
  rv_extension_Zk = rv_extension_Zkn | rv_extension_Zkr |
                    rv_extension_Zkt,  // Standard scalar cryptography extension
};

enum riscv_abi_t {
  rv_abi_ILP32,
  rv_abi_ILP32F,
  rv_abi_ILP32D,
  rv_abi_ILP32E,
  rv_abi_LP64,
  rv_abi_LP64F,
  rv_abi_LP64D,
  rv_abi_LP64Q
};

struct hal_device_info_riscv_t : public hal::hal_device_info_t {
  // bit-field describing the supported extensions (combination of
  // rv_extension_?`s)
  uint64_t extensions;
  // the target ABI ComputeAorta should compile for
  uint32_t abi;

  /// @brief Update base device info with known values in the riscv info
  /// extensions for example want should_vectorize
  void update_base_info_from_riscv(::hal::hal_device_info_t &info) {
    info.should_vectorize = extensions & rv_extension_V;
    info.supports_doubles = extensions & rv_extension_D;
    info.supports_fp16 = extensions & rv_extension_Zfh;
  }

  // vlen - default to 0 (which means the v extension is not enabled or the
  // actual vlen cannot be determined)
  uint32_t vlen = 0;
};

/// @}
}  // namespace riscv

#endif  // RISCV_HAL_RISCV_H_INCLUDED
