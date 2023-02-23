# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_concatenate_dma.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(concatenate_dma_kernel kernel_binary.h device_concatenate_dma.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET concatenate_dma
  SOURCES concatenate_dma.cpp
  KERNELS concatenate_dma_kernel
)

target_link_libraries(concatenate_dma PRIVATE clik_runtime_async)
