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

find_program(glslangValidator_EXECUTABLE
  NAMES glslangValidator PATHS ENV VULKAN_SDK PATH_SUFFIXES bin
  CMAKE_FIND_ROOT_PATH_BOTH)
if(glslangValidator_EXECUTABLE STREQUAL
    glslangValidator_EXECUTABLE-NOTFOUND)
  set(GLSL_UNSUPPORTED True)
  message(WARNING "${SUITE} glsl lit tests unsupported: glslangValidator not found")
else()
  set(GLSL_UNSUPPORTED False)
endif()

find_package(SpirvTools COMPONENTS spirv-as)
if(TARGET spirv::spirv-as)
  # Create custom commands to assemble .spvasm files in .spv.
  set(SPVASM_UNSUPPORTED False)
  # Get the found spirv-as version, takes the form: v<year>.<release>
  execute_process(
    COMMAND ${SpirvTools_spirv-as_EXECUTABLE} --version
    OUTPUT_VARIABLE SpirvAsVersionOutput)
  string(REGEX MATCH "v20[0-9][0-9].[0-9]+"
    SpirvAsVersion ${SpirvAsVersionOutput})
  string(REGEX MATCH "20[0-9][0-9]"
    SpirvAsVersionYear ${SpirvAsVersion})
  string(REGEX MATCH "[0-9]+$"
    SpirvAsVersionRelease ${SpirvAsVersion})
  message(STATUS "spirv-as: v${SpirvAsVersionYear}.${SpirvAsVersionRelease}")
else()
  set(SPVASM_UNSUPPORTED True)
  message(WARNING "${SUITE} spvasm lit tests unsupported: spirv-as not found")
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
  GLSL_UNSUPPORTED=${GLSL_UNSUPPORTED}
  SPVASM_UNSUPPORTED=${SPVASM_UNSUPPORTED}
  SPIRV_AS_VERSION_YEAR=${SpirvAsVersionYear})

add_subdirectory(glsl)
add_subdirectory(spvasm)

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS spirv-ll-tool ca-common-lit spirv-ll-glsl-lit spirv-ll-spvasm-lit
            ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
endif()
