# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_ternary.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(ternary_async_kernel kernel_binary.h device_ternary.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET ternary_async
  SOURCES ternary.cpp
  KERNELS ternary_async_kernel
)

target_link_libraries(ternary_async PRIVATE clik_runtime_async)
