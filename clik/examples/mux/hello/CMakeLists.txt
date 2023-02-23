# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_mux_example(hello_mux
  hello.cpp
)

# Compile device_hello.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(hello_mux hello_mux_kernel kernel_binary.h device_hello.c)
