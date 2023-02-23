# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(NOT CA_CL_ENABLED)
  return()
endif()

# Include our OpenCL CMake machinery.
include(${ComputeAorta_SOURCE_DIR}/source/cl/cmake/AddCACL.cmake)

add_ca_cl_executable(cl_ext_codeplay_example
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
add_ca_cl_check(cl_ext_codeplay_example
  COMMAND cl_ext_codeplay_example --platform ComputeAorta
  DEPENDS cl_ext_codeplay_example)
install(TARGETS cl_ext_codeplay_example RUNTIME DESTINATION bin)
