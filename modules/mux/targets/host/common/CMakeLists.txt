# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_library(host-common STATIC
  include/host/common/jit_kernel.h
  source/common/jit_kernel.cpp)
target_include_directories(host-common PUBLIC include)
target_link_libraries(host-common PUBLIC cargo)
