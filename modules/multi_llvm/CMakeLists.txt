# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_interface_library(multi_llvm)

target_include_directories(multi_llvm INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(multi_llvm SYSTEM INTERFACE
  ${LLVM_INCLUDE_DIR})

# No need to append interface libraries to MODULES_LIBRARIES variable
