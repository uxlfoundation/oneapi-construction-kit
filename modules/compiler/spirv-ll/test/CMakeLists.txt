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

ca_option(CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE STRING
  "Offline-assemble spirv-ll lit spvasm test binaries" ON)

set(SPVASM_UNSUPPORTED False)

# Check for tools at build time if asked to assemble everything offline.
if(CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE)
  find_package(SpirvTools COMPONENTS spirv-as spirv-val)
  if(TARGET spirv::spirv-as)
    # Get the found spirv-as version, takes the form: v<year>.<point>(-dev)?
    # We treat 'dev' as the previous point release: see below.
    execute_process(
      COMMAND ${SpirvTools_spirv-as_EXECUTABLE} --version
      OUTPUT_VARIABLE SpirvAsVersionOutput)
    string(REGEX MATCH "v(20[0-9][0-9]).([0-9]+)(-dev)?" Tmp ${SpirvAsVersionOutput})
    set(SpirvAsVersionYear "${CMAKE_MATCH_1}")
    set(SpirvAsVersionPoint "${CMAKE_MATCH_2}")
    set(SpirvAsVersionDev "${CMAKE_MATCH_3}")
    message(STATUS "spirv-as: v${SpirvAsVersionYear}.${SpirvAsVersionPoint}${SpirvAsVersionDev}")
    # If we have a 'development' version, canonicalize the internal version as
    # the previous version:
    # * Decrement the 'point release' by one
    # * If that would result in a negative number, decrement the year and set
    #   the 'point release' to 9 (an arbitrarily high version number)
    if(SpirvAsVersionDev)
      if(SpirvAsVersionPoint STREQUAL "0")
        set(SpirvAsVersionPoint "9")
        MATH(EXPR SpirvAsVersionYear "${SpirvAsVersionYear}-1")
      else()
        MATH(EXPR SpirvAsVersionPoint "${SpirvAsVersionPoint}-1")
      endif()
      message(STATUS "Detected 'dev' spirv-as version - tracking version "
        "internally as: v${SpirvAsVersionYear}.${SpirvAsVersionPoint}")
    endif()
    set(SpirvAsVersion "${SpirvAsVersionYear}.${SpirvAsVersionPoint}")
  else()
    set(SPVASM_UNSUPPORTED True)
    message(WARNING "${SUITE} spvasm lit tests unsupported: spirv-as not found")
  endif()
endif()

# Create the test/test inputs directories and set the relevent variables
set(INPUTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  TRIPLE=${TRIPLE}
  CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE=${CA_ASSEMBLE_SPIRV_LL_LIT_TESTS_OFFLINE}
  SPVASM_UNSUPPORTED=${SPVASM_UNSUPPORTED}
  SPIRV_AS_VERSION_YEAR=${SpirvAsVersionYear}
  SPIRV_AS_VERSION_POINT=${SpirvAsVersionPoint})

add_subdirectory(spvasm)

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS spirv-ll-tool spirv-ll-spvasm-lit ${SUITE}-lit)
endif()
