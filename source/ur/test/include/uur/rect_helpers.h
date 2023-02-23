// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Helpers functions to reduce boiler plate when testing the
/// urEnqueueMemBuffer{Read,Write,Copy)Rect entry points.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RECT_HELPERS_INCLUDED
#define RECT_HELPERS_INCLUDED

#include <string>
#include <vector>

#include "fixtures.h"
#include "ur_api.h"

namespace uur {
struct test_parameters_t {
  std::string name;
  size_t src_size;
  size_t dst_size;
  ur_rect_offset_t src_origin;
  ur_rect_offset_t dst_origin;
  ur_rect_region_t region;
  size_t src_row_pitch;
  size_t src_slice_pitch;
  size_t dst_row_pitch;
  size_t dst_slice_pitch;
};

template <typename T>
inline std::string printRectTestString(
    const testing::TestParamInfo<typename T::ParamType> &info) {
  // ParamType will be std::tuple<uint32_t, test_parameters_t>
  const auto device_idx = std::get<0>(info.param);
  const auto device_name = uur::getDeviceNameFromDeviceIndex(device_idx);
  const auto test_name = std::get<1>(info.param).name;
  return device_name + '_' + test_name;
}

// Performs host side equivalent of urEnqueueMemBufferReadRect,
// urEnqueueMemBufferWriteRect and urEnqueueMemBufferCopyRect.
inline void copyRect(std::vector<uint8_t> src, ur_rect_offset_t src_offset,
                     ur_rect_offset_t dst_offset, ur_rect_region_t region,
                     size_t src_row_pitch, size_t src_slice_pitch,
                     size_t dst_row_pitch, size_t dst_slice_pitch,
                     std::vector<uint8_t> &dst) {
  const auto src_linear_offset = src_offset.x + src_offset.y * src_row_pitch +
                                 src_offset.z * src_slice_pitch;
  const auto src_start = src.data() + src_linear_offset;

  const auto dst_linear_offset = dst_offset.x + dst_offset.y * dst_row_pitch +
                                 dst_offset.z * dst_slice_pitch;
  const auto dst_start = dst.data() + dst_linear_offset;

  for (unsigned k = 0; k < region.depth; ++k) {
    const auto src_slice = src_start + k * src_slice_pitch;
    const auto dst_slice = dst_start + k * dst_slice_pitch;
    for (unsigned j = 0; j < region.height; ++j) {
      auto src_row = src_slice + j * src_row_pitch;
      auto dst_row = dst_slice + j * dst_row_pitch;
      std::memcpy(dst_row, src_row, region.width);
    }
  }
}
}  // namespace uur
#endif
