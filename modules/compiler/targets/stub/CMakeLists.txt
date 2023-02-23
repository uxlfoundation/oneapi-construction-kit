# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the Mux stub target is not enabled, then don't enable this one either.
if(NOT CA_STUB_ENABLED)
  return()
endif()

add_ca_library(compiler-stub STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/stub/compiler-stub.h)
target_include_directories(compiler-stub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(compiler-stub PUBLIC compiler-base)

add_mux_compiler_target(compiler-stub
  COMPILER_INFO stub::StubInfo::get
  HEADER_FILE stub/compiler-stub.h)
