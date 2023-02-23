# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(host_EXTERNAL_UNITCL_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/cl_ext_codeplay.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/host_clGetDeviceInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/unitcl_divisible_preferred_size.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/unitcl_kernel_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/unitcl_test.cpp)
if(CA_HOST_ENABLE_BUILTINS_EXTENSION)
  list(APPEND host_EXTERNAL_UNITCL_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/cl_codeplay_host_builtins.cpp)
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lit)
endif()

if(CA_HOST_ENABLE_BUILTIN_KERNEL)
  list(APPEND host_EXTERNAL_UNITCL_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitCL/hostBuiltInKernels.cpp)
endif()
set(host_EXTERNAL_UNITCL_SRC ${host_EXTERNAL_UNITCL_SRC}
  CACHE INTERNAL "List of additional host UnitCL source files.")

set(host_EXTERNAL_UNITCL_INC
  ${host_SOURCE_DIR}/extension/cl_ext_codeplay/include
  CACHE INTERNAL
  "List of additional include directories required by host UnitCL.")

set(host_EXTERNAL_UNITVK_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/UnitVK/unitvk_test.cpp
  CACHE INTERNAL "List of additional host UnitVK source files.")

add_subdirectory(UnitCL/kernels)
