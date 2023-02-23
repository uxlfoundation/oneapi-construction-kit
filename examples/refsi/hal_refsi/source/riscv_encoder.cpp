// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  return opc | ((imm & 0x1f) << 7) | (funct3 << 12) | (rs1 << 15) | (rs2 << 20)
      | (((imm & 0xfe0) >> 5) << 25);
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
