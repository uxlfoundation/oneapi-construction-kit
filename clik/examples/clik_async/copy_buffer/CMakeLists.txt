# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_clik_example(
  TARGET copy_buffer_async
  SOURCES copy_buffer.cpp
)

target_link_libraries(copy_buffer_async PRIVATE clik_runtime_async)
