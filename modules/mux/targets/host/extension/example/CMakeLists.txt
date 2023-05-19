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

if(NOT CA_CL_ENABLED)
  return()
endif()

# Include our OpenCL CMake machinery.
include(${ComputeAorta_SOURCE_DIR}/source/cl/cmake/AddCACL.cmake)

add_ca_cl_executable(cl_ext_codeplay_example
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
add_ca_cl_check(cl_ext_codeplay_example
  COMMAND cl_ext_codeplay_example --platform ComputeAorta
  DEPENDS cl_ext_codeplay_example)
install(TARGETS cl_ext_codeplay_example RUNTIME DESTINATION bin)
