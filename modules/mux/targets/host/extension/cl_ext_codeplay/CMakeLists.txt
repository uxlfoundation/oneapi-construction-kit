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

add_ca_cl_runtime_extension(host-cl-runtime-exts
  EXTENSIONS codeplay_set_threads
  HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/CL/cl_ext_codeplay_host.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_set_threads.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_set_threads.cpp)

if(CA_HOST_ENABLE_BUILTINS_EXTENSION)
  # We only build cl_codeplay_host_builtins on non-release builds, since it's
  # really just a flag used for testing.
  add_ca_cl_compiler_extension(host-cl-compiler-exts
    EXTENSIONS codeplay_host_builtins
    HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/CL/cl_ext_codeplay_host.h
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_host_builtins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_host_builtins.cpp)
  # The host force-included header is only for testing.
  # TODO: Remove the prefix add_ca_force_header here, it's not used anymore.
  add_ca_force_header(PREFIX "host_force_inc_h"
    DEVICE_NAME "${host_CA_HOST_CL_DEVICE_NAME}"
    PATH "${CMAKE_CURRENT_SOURCE_DIR}/source/host_force_inc.h")
endif()
