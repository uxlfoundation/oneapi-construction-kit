# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_barrier_sum.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(barrier_sum_kernel kernel_binary.h device_barrier_sum.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET barrier_sum
  SOURCES barrier_sum.cpp
  KERNELS barrier_sum_kernel
)

target_link_libraries(barrier_sum PRIVATE clik_runtime_async)
