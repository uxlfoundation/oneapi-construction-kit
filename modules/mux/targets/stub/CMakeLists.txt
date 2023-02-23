# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# The stub target is only ever used for manually testing unusual corner cases,
# so it needs to be explicitly enabled. Most of the time we pretend it isn't
# even here. It will probably be removed completely before too long.
ca_option(CA_STUB_ENABLED BOOL "Enable stub mux target" OFF)
if(NOT CA_STUB_ENABLED)
  return()
endif()

add_ca_library(stub STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/stub/stub.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stub.cpp)
target_include_directories(stub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(stub PRIVATE
  $<$<BOOL:${UNIX}>:-Wno-unused-parameter>)
target_link_libraries(stub PUBLIC mux-headers)

add_mux_target(stub CAPABILITIES "64bit"
  HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/stub
  DEVICE_NAMES "Codeplay Software Ltd. - stub")

