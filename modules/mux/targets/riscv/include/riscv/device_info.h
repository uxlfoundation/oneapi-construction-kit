// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// riscv's device_info interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_DEVICE_INFO_H_INCLUDED
#define RISCV_DEVICE_INFO_H_INCLUDED

#include <hal.h>
#include <hal_library.h>
#include <mux/mux.h>

#include <array>

namespace riscv {
/// @addtogroup riscv
/// @{

struct device_info_s final : public mux_device_info_s {
  /// @brief constructor.
  device_info_s();

  /// @brief update device info from a hal_device_info.
  void update_from_hal_info(const hal::hal_device_info_t *);

  /// @brief returns true if this device_info has been initialized.
  bool is_valid() const { return valid; }

  /// @brief hal_device_info object used to fill out this device_info.
  const hal::hal_device_info_t *hal_device_info;

  /// @brief the hal index coresponding to this device.
  uint32_t hal_device_index;

  /// @brief true if this is an initialized device_info.
  bool valid;
};

/// @brief query the hal and update `device_infos` to reflect the available
/// devices.
bool enumerate_device_infos();

static const size_t max_device_infos = 1;
extern std::array<device_info_s, max_device_infos> device_infos;

typedef device_info_s *device_info_t;
/// @}
}  // namespace riscv

#endif  // RISCV_DEVICE_INFO_H_INCLUDED
