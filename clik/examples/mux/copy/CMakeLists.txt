# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_mux_example(copy_mux
  copy.cpp
)

# Compile device_copy.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(copy_mux copy_mux_kernel kernel_binary.h device_copy.c)

# Generate a C header containing SPIR bitcode for the kernel using Bin2H.
if (${DEFAULT_ISA} MATCHES "^RV64")
  set(BITCODE_SRC device_copy64.bc)
else()
  set(BITCODE_SRC device_copy32.bc)
endif()
hal_add_baked_data(copy_mux copy_kernel_bitcode kernel_bitcode.h ${BITCODE_SRC})
