# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_barrier_print.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(barrier_print_kernel kernel_binary.h device_barrier_print.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET barrier_print
  SOURCES barrier_print.cpp
  KERNELS barrier_print_kernel
)

target_link_libraries(barrier_print PRIVATE clik_runtime_async)
