# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_executable(MultiDevice
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp)

target_include_directories(MultiDevice PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(MultiDevice PRIVATE
  CA_VERSION="${PROJECT_VERSION}")
target_link_libraries(MultiDevice PRIVATE ca_gtest)

add_ca_cl_check(MultiDevice GTEST
  COMMAND MultiDevice --opencl-platform=ComputeAorta
    --gtest_output=xml:${PROJECT_BINARY_DIR}/MultiDevice.xml
  CLEAN ${PROJECT_BINARY_DIR}/MultiDevice.xml
  DEPENDS MultiDevice)

install(TARGETS MultiDevice RUNTIME DESTINATION bin COMPONENT MultiDevice)
