# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_clik_example(
  TARGET copy_buffer
  SOURCES copy_buffer.cpp
)

target_link_libraries(copy_buffer PRIVATE clik_runtime_sync)
