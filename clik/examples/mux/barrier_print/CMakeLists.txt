# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_mux_example(barrier_print_mux
  barrier_print.cpp
)

# Compile device_barrier_print.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(barrier_print_mux barrier_print_mux_kernel kernel_binary.h device_barrier_print.c)
