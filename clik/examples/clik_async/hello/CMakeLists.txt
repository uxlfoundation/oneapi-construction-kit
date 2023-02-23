# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_hello.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(hello_async_kernel kernel_binary.h device_hello.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET hello_async
  SOURCES hello.cpp
  KERNELS hello_async_kernel
)

target_link_libraries(hello_async PRIVATE clik_runtime_async)
