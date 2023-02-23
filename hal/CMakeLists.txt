# Copyright (C) Codeplay Software Limited. All Rights Reserved.

cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY HAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(HALBin2H)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(HAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HAL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(HAL_SOURCE
  ${HAL_INCLUDE_DIR}/hal.h
  ${HAL_INCLUDE_DIR}/hal_types.h
  ${HAL_INCLUDE_DIR}/hal_riscv.h
  ${HAL_INCLUDE_DIR}/allocator.h
  ${HAL_INCLUDE_DIR}/arg_pack.h
  ${HAL_INCLUDE_DIR}/hal_library.h
  ${HAL_INCLUDE_DIR}/hal_riscv_common.h
  ${HAL_INCLUDE_DIR}/hal_profiler.h
  ${HAL_INCLUDE_DIR}/program.h
  ${HAL_SOURCE_DIR}/program.cpp
  ${HAL_SOURCE_DIR}/profiler.cpp
  ${HAL_SOURCE_DIR}/hal_riscv_common.cpp
  ${HAL_SOURCE_DIR}/arg_pack.cpp
  ${HAL_SOURCE_DIR}/hal_library.cpp)

add_library(
  hal_common STATIC ${HAL_SOURCE})
target_include_directories(
  hal_common PUBLIC ${HAL_INCLUDE_DIR})
set_target_properties(
  hal_common PROPERTIES LINKER_LANGUAGE CXX)

if (MSVC)
target_compile_options(hal_common PRIVATE
  /GR-      # Disable Run-Time Type Information
  /EHs      # Enables standard C++ stack unwinding
  /EHc      # Assume extern "C" functions will not throw
  /WX       # Treat warnings as errors
)
endif()

target_compile_definitions(hal_common PRIVATE
  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN>)

target_link_libraries(hal_common PUBLIC $<$<PLATFORM_ID:Linux>:dl>)

add_subdirectory(source/hal_null)
