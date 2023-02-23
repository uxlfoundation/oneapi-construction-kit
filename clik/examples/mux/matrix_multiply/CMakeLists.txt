# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_mux_example(matrix_multiply_mux
  matrix_multiply.cpp
)

# Compile device_matrix_multiply.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(matrix_multiply_mux matrix_multiply_mux_kernel kernel_binary.h device_matrix_multiply.c)
