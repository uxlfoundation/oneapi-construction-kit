# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32 AND NOT CA_BUILD_32_BITS)
  set(CMAKE_CL_64 true)
endif()

if(CA_CL_ENABLE_ICD_LOADER)
  set(CLConform_HEADERS ${CL_INCLUDE_DIR})
  set(CLConform_LIBRARIES OpenCL)
else()
  set(CLConform_LIBRARIES CL)
endif()
set(BENCHCL_LIBRARY CL)
set(CLConf_OUT_DIR bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UnitCL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiDevice)
if(TARGET ca-benchmark)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/BenchCL)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FuzzCL)
