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

#include "riscv_encoder.h"

#include <stdio.h>

// Encode a I-type instruction.
static uint32_t encodeI(unsigned opc, unsigned func3, unsigned rd, unsigned rs1,
                        unsigned imm) {
  return opc | (rd << 7) | (func3 << 12) | (rs1 << 15) | ((imm & 0xfff) << 20);
}

// Encode a S-type instruction.
static uint32_t encodeS(unsigned opc, unsigned funct3, unsigned rs1,
                        unsigned rs2, unsigned imm) {
  return opc | ((imm & 0x1f) << 7) | (funct3 << 12) | (rs1 << 15) |
         (rs2 << 20) | (((imm & 0xfe0) >> 5) << 25);
}

// Encode a R-type instruction.
static uint32_t encodeR(unsigned opc, unsigned func3, unsigned func7,
                        unsigned rd, unsigned rs1, unsigned rs2) {
  return opc | (rd << 7) | (func3 << 12) | (rs1 << 15) | (rs2 << 20) |
         (func7 << 25);
}

uint32_t riscv_encoder::addADDI(unsigned rd, unsigned rs, uint32_t imm) {
  uint32_t insn = encodeI(0x13, 0, rd, rs, imm);
  insns.push_back(insn);
  return insn;
}

uint32_t riscv_encoder::addECALL() {
  uint32_t insn = 0x00000073;
  insns.push_back(insn);
  return insn;
}

uint32_t riscv_encoder::addJALR(unsigned rd, unsigned rs, uint32_t imm) {
  uint32_t insn = encodeI(0x67, 0, rd, rs, imm);
  insns.push_back(insn);
  return insn;
}

uint32_t riscv_encoder::addLW(unsigned rd, unsigned rs, uint32_t imm) {
  uint32_t insn = encodeI(0x3, 0x2, rd, rs, imm);
  insns.push_back(insn);
  return insn;
}

uint32_t riscv_encoder::addSW(unsigned rs2, unsigned rs1, uint32_t imm) {
  uint32_t insn = encodeS(0x23, 0x2, rs1, rs2, imm);
  insns.push_back(insn);
  return insn;
}

uint32_t riscv_encoder::addMulInst(riscv_mul_opcode opc, unsigned rd,
                                   unsigned rs1, unsigned rs2) {
  uint32_t insn = encodeR(0x33, (unsigned)opc, 0x1, rd, rs1, rs2);
  insns.push_back(insn);
  return insn;
}
