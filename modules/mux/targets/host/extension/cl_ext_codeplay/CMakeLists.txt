# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_runtime_extension(host-cl-runtime-exts
  EXTENSIONS codeplay_set_threads
  HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/CL/cl_ext_codeplay_host.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_set_threads.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_set_threads.cpp)

if(CA_HOST_ENABLE_BUILTINS_EXTENSION)
  # We only build cl_codeplay_host_builtins on non-release builds, since it's
  # really just a flag used for testing.
  add_ca_cl_compiler_extension(host-cl-compiler-exts
    EXTENSIONS codeplay_host_builtins
    HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/CL/cl_ext_codeplay_host.h
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_host_builtins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_host_builtins.cpp)
  # The host force-included header is only for testing.
  # TODO: Remove the prefix add_ca_force_header here, it's not used anymore.
  add_ca_force_header(PREFIX "host_force_inc_h"
    DEVICE_NAME "${host_CA_HOST_CL_DEVICE_NAME}"
    PATH "${CMAKE_CURRENT_SOURCE_DIR}/source/host_force_inc.h")
endif()
