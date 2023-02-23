# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(NOT TARGET hal_common)
  return()
endif()

add_ca_library(mux-hal STATIC
  include/mux/hal/buffer.h source/buffer.cpp
  include/mux/hal/device.h source/device.cpp
  include/mux/hal/executable.h source/executable.cpp
  include/mux/hal/fence.h source/fence.cpp
  include/mux/hal/kernel.h
  include/mux/hal/memory.h source/memory.cpp
  include/mux/hal/query_pool.h source/query_pool.cpp
  include/mux/hal/semaphore.h source/semaphore.cpp)

target_include_directories(mux-hal PUBLIC include)
target_link_libraries(mux-hal PUBLIC
  cargo hal_common mux-headers mux-utils utils)
