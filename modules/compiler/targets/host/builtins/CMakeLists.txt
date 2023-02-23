# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 AND
   NOT CMAKE_SYSTEM_PROCESSOR STREQUAL AMD64)
  message(STATUS
    "host: Target specific builtins disabled for ${CMAKE_SYSTEM_PROCESSOR}")
  return()
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(fp64 FP64)
endif()

add_target_builtins(host
  TRIPLE x86_64-unknown-unknown 64BIT ${fp64}
  HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/include/host/resources.h
  SOURCES source/sqrt.cpp)
