# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(BINARY_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/argument.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/binary.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/kernel_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/program_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/spirv.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/argument.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/binary.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv.cpp)

add_ca_library(CL-binary STATIC ${BINARY_SOURCES})

target_include_directories(CL-binary PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/source/cl/include)

target_link_libraries(CL-binary PUBLIC
  builtins cargo compiler-loader mux utils)

# spirv-ll defines the CA_ENABLE_SPIRV_LL_EXPERIMENTAL CMake option, so spirv-ll
# should be included with add_subdirectory before this file.
# TODO(CA-3067): Remove SPIRV_LL_EXPERIMENTAL when we have a proper mechanism
# for extending spirv-ll.
if(${CA_ENABLE_SPIRV_LL_EXPERIMENTAL})
  target_compile_definitions(CL-binary PUBLIC SPIRV_LL_EXPERIMENTAL)
endif()
