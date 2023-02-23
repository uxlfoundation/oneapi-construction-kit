# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(HAL_CPU_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${HAL_CPU_SOURCE_DIR}/cmake)

include(CompileKernel)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fvisibility=hidden)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fvisibility=hidden)

set_property(GLOBAL APPEND PROPERTY KNOWN_HAL_DEVICES "cpu")
set_property(GLOBAL PROPERTY HAL_CPU_DIR "${HAL_CPU_SOURCE_DIR}")
set_property(GLOBAL PROPERTY HAL_CPU_EXAMPLE_DIR "${HAL_CPU_SOURCE_DIR}/examples/wi")

add_subdirectory(source)
