# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_matrix_multiply.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(matrix_multiply_kernel kernel_binary.h device_matrix_multiply.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET matrix_multiply
  SOURCES matrix_multiply.cpp
  KERNELS matrix_multiply_kernel
)

target_link_libraries(matrix_multiply PRIVATE clik_runtime_async)
