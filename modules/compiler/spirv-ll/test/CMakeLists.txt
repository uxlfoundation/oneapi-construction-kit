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
set(SUITE spirv-ll)

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "${SUITE} lit tests disabled: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

find_package(LLVMTool COMPONENTS opt)
if(NOT TARGET LLVM::opt)
  message(WARNING "${SUITE} lit tests disabled: opt not found")
  return()
endif()

find_package(SpirvTools COMPONENTS spirv-as)
if(NOT TARGET spirv::spirv-as)
  message(WARNING "${SUITE} some lit tests unsupported: spirv-as not found")
  return()
endif()

# Create the test/test inputs directories and set the relevent variables
set(INPUTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  SPIRV_LL=$<TARGET_FILE:spirv-ll-tool>
  OPT=$<TARGET_FILE:LLVM::opt>
  TRIPLE=${TRIPLE})

add_subdirectory(glsl)
add_subdirectory(spvasm)

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ca-common-lit spirv-ll-lit-glsl spirv-ll-lit-spvasm
            ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
endif()
