# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_mux_example(vector_add_mux
  vector_add.cpp
)

# Compile device_vector_add.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(vector_add_mux vector_add_mux_kernel kernel_binary.h device_vector_add.c)
