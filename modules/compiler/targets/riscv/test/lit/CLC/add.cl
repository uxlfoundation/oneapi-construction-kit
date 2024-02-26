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

// REQUIRES: riscv_rvv

// RUN: clc --device "%riscv_device" --strip-binary-header -o %t.o.novec %s -cl-wfv=never
// RUN: llvm-objdump --mattr=%vattr -d %t.o.novec > %t.novec
// RUN: FileCheck --check-prefixes NOVEC < %t.novec %s

// RUN: env CA_RISCV_VF=4 clc --device "%riscv_device" --strip-binary-header -o %t.o %s
// RUN: llvm-objdump --mattr=%vattr -d %t.o >  %t_v4
// RUN: FileCheck --check-prefixes V4 < %t_v4 %s

// RUN: env CA_RISCV_VF=1,S clc --device "%riscv_device" --strip-binary-header -o %t.o %s
// RUN: llvm-objdump --mattr=%vattr -d %t.o > %t_1s
// RUN: FileCheck --check-prefixes 1S < %t_1s %s

// RUN: env CA_RISCV_VF=1,S,V clc --device "%riscv_device" --strip-binary-header -o %t.o %s
// RUN: llvm-objdump --mattr=%vattr -d %t.o > %t_1s_v
// RUN: FileCheck --check-prefixes 1S_V < %t_1s_v %s

// Check that vector flags do not add more load word instruction than the two
// generated without vectorization

// NOVEC-COUNT-2: lw
// NOVEC-NOT: lw

// Check that for vectorize factor of 4
// sets e32 and passes 4 for the length requested
// We want to ensure that standard vector width of 4 mode still produces the
// same "add" name
// V4: <__vecz_v4_add.mux-kernel-wrapper>
// V4: vsetivli zero, {{(0x)?}}4, e32

// Check for vectorize factor of 1,S
// This means that we use vscale x 1 in LLVM IR terms
// vscale = VLEN/64 so you can fit vscale x 2 i32s in one VLEN-bit vector register
// So check for mf2 (LMUL=1/2)
// Check also that zero is passed for the count, meaning as much as we can do per
// vector instruction.
// This is a limitation of using LLVM IR without intrinsics which would allow
// us to set the count and use predication

// We want to ensure that 1,S still produces the same "add" name
// 1S: <__vecz_nxv1_add.mux-kernel-wrapper>
// 1S: vsetvli {{[as][0-9]*}}, zero, e32, mf2, ta, {{(mu|ma)}}

// We want to ensure that 1,S,V still produces the same "add" name
// 1S_V: <__vecz_nxv1_add.mux-kernel-wrapper>
// 1S_V: vsetvli {{[as][0-9]*}}, zero, e32, mf2, ta, {{(mu|ma)}}

__kernel void add(__global int *in1, __global int *in2, __global int *out) {
  size_t tid = get_global_id(0);
  // Vectorize load and add chain.
  out[tid] = in1[tid] + in2[tid];
}
