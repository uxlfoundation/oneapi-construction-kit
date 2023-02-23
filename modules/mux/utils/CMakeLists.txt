# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_library(mux-utils STATIC
  include/mux/utils/allocator.h
  include/mux/utils/helpers.h source/helpers.cpp
  include/mux/utils/id.h
  include/mux/utils/small_vector.h)
target_include_directories(mux-utils PUBLIC include)
target_link_libraries(mux-utils PUBLIC mux-headers mux cargo)
