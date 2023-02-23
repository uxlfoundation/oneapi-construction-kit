# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_vector_add.c for the device and generate a C header file
# containing the compiled kernel executable using Bin2H.
add_baked_kernel(vector_add_kernel kernel_binary.h device_vector_add.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET vector_add
  SOURCES vector_add.cpp
  KERNELS vector_add_kernel
)

target_link_libraries(vector_add PRIVATE clik_runtime_sync)
