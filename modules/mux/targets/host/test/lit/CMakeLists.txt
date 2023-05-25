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
set(SUITE host-mux)

if(NOT CA_ENABLE_API MATCHES "(^$|cl)")
  message(WARNING "${SUITE}-lit disabled: requires CL API")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  CA_HOST_ENABLE_FP64=${CA_HOST_ENABLE_FP64}
  host_CA_HOST_CL_DEVICE_NAME=${host_CA_HOST_CL_DEVICE_NAME}
  CA_ENABLE_HOST_IMAGE_SUPPORT=${CA_ENABLE_HOST_IMAGE_SUPPORT}
  CA_HOST_TARGET_CPU=${CA_HOST_TARGET_CPU}
  CA_CL_ENABLE_ICD_LOADER=${CA_CL_ENABLE_ICD_LOADER}
  CL_TARGET_FILE=$<TARGET_FILE:CL>
  host_CAPABILITIES="${host_CAPABILITIES}")

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    TARGET host
    DEPENDS ca-common-lit clc oclc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
endif()
