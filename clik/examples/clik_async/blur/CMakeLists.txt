# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_blur.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(blur_kernel blur_kernel_bin.h device_blur.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET blur
  SOURCES blur.cpp
  KERNELS blur_kernel
)

target_link_libraries(blur PRIVATE clik_runtime_async)
