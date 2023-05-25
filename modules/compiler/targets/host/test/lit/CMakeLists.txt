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
set(SUITE host-compiler)

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "${SUITE}-lit will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  muxc_EXECUTABLE=$<TARGET_FILE:muxc>
  COMPILER_CROSS_x86_64=${cross_x86_64_CA_HOST_CL_DEVICE_NAME}
  COMPILER_CROSS_AArch64=${cross_aarch64_CA_HOST_CL_DEVICE_NAME}
  COMPILER_CROSS_x86=${cross_x86_CA_HOST_CL_DEVICE_NAME}
  COMPILER_CROSS_Arm=${cross_arm_CA_HOST_CL_DEVICE_NAME}
  HOST_DEVICE_NAME=${host_DEVICE_NAMES})

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    TARGET host
    DEPENDS ca-common-lit muxc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
endif()
