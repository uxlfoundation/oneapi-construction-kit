// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _HAL_REFSI_RISCV_ENCODER_H
#define _HAL_REFSI_RISCV_ENCODER_H

#include <stddef.h>
#include <stdint.h>

#include <vector>

enum riscv_register {
  ZERO = 0,
  RA = 1,
  SP = 2,
  GP = 3,
  TP = 4,
  T0 = 5,
  T1 = 6,
  T2 = 7,
  S0 = 8,
  S1 = 9,
  A0 = 10,
  A1 = 11,
  A2 = 12,
  A3 = 13,
  A4 = 14,
  A5 = 15,
  A6 = 16,
  A7 = 17,
  S2 = 18,
  S3 = 19,
  S4 = 20,
  S5 = 21,
  S6 = 22,
  S7 = 23,
  S8 = 24,
  S9 = 25,
  S10 = 26,
  S11 = 27,
  T3 = 28,
  T4 = 29,
  T5 = 30,
  T6 = 31
};

enum riscv_mul_opcode {
  MUL = 0,
  MULH = 1,
  MULHSU = 2,
  MULHU = 3,
  DIV = 4,
  DIVU = 5,
  REM = 6,
  REMU = 7
};

class riscv_encoder {
public:
  const uint32_t * data() const { return insns.data(); }
  size_t size() const { return insns.size() * sizeof(uint32_t); }

  uint32_t addADDI(unsigned rd, unsigned rs, uint32_t imm);
  uint32_t addLI(unsigned rd, uint32_t imm) { return addADDI(rd, ZERO, imm); }
  uint32_t addMulInst(riscv_mul_opcode opc, unsigned rd, unsigned rs1,
                      unsigned rs2);
  uint32_t addMV(unsigned rd, unsigned rs) { return addADDI(rd, rs, 0); }
  uint32_t addECALL();
  uint32_t addJR(unsigned rs) { return addJALR(ZERO, rs, 0); }
  uint32_t addJALR(unsigned rd, unsigned rs, uint32_t imm);
  uint32_t addLW(unsigned rd, unsigned rs, uint32_t imm);
  uint32_t addSW(unsigned rs2, unsigned rs1, uint32_t imm);

private:
  std::vector<uint32_t> insns;
};

#endif  // _HAL_REFSI_RISCV_ENCODER_H
