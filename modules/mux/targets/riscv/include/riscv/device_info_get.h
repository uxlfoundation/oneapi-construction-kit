// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_DEVICE_INFO_GET_H
#define RISCV_DEVICE_INFO_GET_H

#include <cargo/array_view.h>
#include <mux/mux.h>
#include <riscv/device_info.h>
#include <riscv/hal.h>

#include <array>
#include <cassert>

namespace riscv {

/// @brief get the riscv device infos as an array view.
cargo::array_view<riscv::device_info_s> GetDeviceInfosArray();

}  // namespace riscv

// namespace riscv
#endif
