// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_LIB_BAKERY_H
#define RISCV_LIB_BAKERY_H

#include <cstdint>

/// @brief gets the data which represents the compiled runtime library for 64
/// bit RISC-V
/// @return Return a pointer to the data
const uint8_t *get_rtlib64_data();

/// @brief gets size of the 64 bit runtime library data
/// @return Return the size in bytes of the runtime library data
size_t get_rtlib64_size();

/// @brief gets the data which represents the compiled runtime library for 32
/// bit RISC-V
/// @return Return a pointer to the data
const uint8_t *get_rtlib32_data();

/// @brief gets size of the 32 bit runtime library data
/// @return Return the size in bytes of the runtime library data
size_t get_rtlib32_size();

#endif
