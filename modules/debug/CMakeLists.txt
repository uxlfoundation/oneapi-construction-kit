# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(CA_ENABLE_DEBUG_BACKTRACE)
  # Only build with debug backtrace when enabled.
  add_ca_library(debug-backtrace STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/debug/backtrace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/backtrace.cpp)
  target_include_directories(debug-backtrace PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_definitions(debug-backtrace PUBLIC
    DEBUG_BACKTRACE_ENABLED)
  target_link_libraries(debug-backtrace PUBLIC cargo)
endif()
