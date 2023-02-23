// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Host's image interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_IMAGE_H_INCLUDED
#define HOST_IMAGE_H_INCLUDED

#include <mux/mux.h>

#ifdef HOST_IMAGE_SUPPORT
#include <libimg/host.h>
#endif

namespace host {
/// @addtogroup host
/// @{

struct image_s final : public mux_image_s {
  /// @brief Host image constructor.
  image_s(mux_memory_requirements_s memory_requirements, mux_image_type_e type,
          mux_image_format_e format, uint32_t pixel_size, uint32_t width,
          uint32_t height, uint32_t depth, uint32_t array_layers,
          uint64_t row_size, uint64_t slice_size);

#ifdef HOST_IMAGE_SUPPORT
  libimg::HostImage image;
#endif
};

/// @}
}  // namespace host

#endif  // HOST_IMAGE_H_INCLUDED
