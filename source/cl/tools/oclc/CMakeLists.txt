# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_executable(oclc
  ${CMAKE_CURRENT_SOURCE_DIR}/oclc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/oclc.cpp)
target_compile_options(oclc PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)
target_compile_definitions(oclc PRIVATE ${CA_CL_COMPILE_DEFINITIONS}
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
# Always link against CL instead of OpenCL to avoid the need to set environment
# varaibles and retain the usability of the tool.
target_link_libraries(oclc PRIVATE cargo CL)

install(TARGETS oclc RUNTIME DESTINATION bin COMPONENT OCLTools)
