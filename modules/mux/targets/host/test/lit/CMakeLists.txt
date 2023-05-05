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

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "host-lit tests will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

if(NOT CA_ENABLE_API MATCHES "(^$|cl)")
  message(WARNING "host-lit disabled: requires CL API")
  return()
endif()

# Find required LLVM tools.
find_package(LLVMTool COMPONENTS
  opt llc llvm-profdata llvm-cov llvm-as llvm-dis FileCheck not)
if(NOT LLVMTool_FOUND)
  message(WARNING "host-lit disabled, not found: "
    "opt llc llvm-profdata llvm-cov llvm-as llvm-dis FileCheck not")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  oclc_EXECUTABLE=$<TARGET_FILE:oclc>
  clc_EXECUTABLE=$<TARGET_FILE:clc>
  CA_HOST_ENABLE_FP64=${CA_HOST_ENABLE_FP64}
  host_CA_HOST_CL_DEVICE_NAME=${host_CA_HOST_CL_DEVICE_NAME}
  CA_ENABLE_HOST_IMAGE_SUPPORT=${CA_ENABLE_HOST_IMAGE_SUPPORT}
  CA_HOST_TARGET_CPU=${CA_HOST_TARGET_CPU}
  CA_CL_ENABLE_ICD_LOADER=${CA_CL_ENABLE_ICD_LOADER}
  CL_TARGET_FILE=$<TARGET_FILE:CL>
  host_CAPABILITIES="${host_CAPABILITIES}")

add_ca_lit_testsuite(host-mux
  ${CMAKE_CURRENT_BINARY_DIR}
  TARGET host
  DEPENDS ca-common-lit clc oclc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION "share/host" COMPONENT OCLTest)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION "share/host" COMPONENT OCLTest)
