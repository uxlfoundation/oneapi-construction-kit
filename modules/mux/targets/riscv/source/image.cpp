// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <mux/mux.h>

#include <cstdlib>

extern "C" {

mux_result_t riscvCreateImage(mux_device_t device, mux_image_type_e type,
                              mux_image_format_e format, uint32_t width,
                              uint32_t height, uint32_t depth,
                              uint32_t array_layers, uint64_t row_size,
                              uint64_t slice_size,
                              mux_allocator_info_t allocator_info,
                              mux_image_t *out_image) {
  (void)device;
  (void)type;
  (void)format;
  (void)width;
  (void)height;
  (void)depth;
  (void)array_layers;
  (void)row_size;
  (void)slice_size;
  (void)allocator_info;
  (void)*out_image;

  return mux_error_feature_unsupported;
}

void riscvDestroyImage(mux_device_t device, mux_image_t image,
                       mux_allocator_info_t allocator_info) {
  (void)device;
  (void)image;
  (void)allocator_info;
}

mux_result_t riscvBindImageMemory(mux_device_t device, mux_memory_t memory,
                                  mux_image_t image, uint64_t offset) {
  (void)device;

  (void)memory;
  (void)image;
  (void)offset;

  return mux_error_feature_unsupported;
}

mux_result_t riscvGetSupportedImageFormats(
    mux_device_t device, mux_image_type_e image_type,
    mux_allocation_type_e allocation_type, uint32_t count,
    mux_image_format_e *out_formats, uint32_t *out_count) {
  (void)device;
  (void)image_type;
  (void)allocation_type;
  (void)count;
  (void)*out_formats;
  (void)*out_count;

  return mux_error_feature_unsupported;
}
}
