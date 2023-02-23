# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile device_vector_add_wfv.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(vector_add_wfv_kernel kernel_binary.h device_vector_add_wfv.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET vector_add_wfv
  SOURCES vector_add_wfv.cpp
  KERNELS vector_add_wfv_kernel
)

target_link_libraries(vector_add_wfv PRIVATE clik_runtime_async)
