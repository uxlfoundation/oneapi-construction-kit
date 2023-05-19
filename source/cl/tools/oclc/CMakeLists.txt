# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
