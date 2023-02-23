# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_hello.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(hello_kernel kernel_binary.h device_hello.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET hello
  SOURCES hello.cpp
  KERNELS hello_kernel
)

target_link_libraries(hello PRIVATE clik_runtime_sync)
