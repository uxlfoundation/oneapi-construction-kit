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

include(GLSL)

# GLSL shader
set(comp ${CMAKE_CURRENT_SOURCE_DIR}/source/vector_add.comp)
if(NOT EXISTS ${comp})
  message(FATAL_ERROR "${comp} doesn't exist!")
endif()

# Header to be generated containing SPIR-V binary
set(header ${CMAKE_CURRENT_BINARY_DIR}/vector_add.h)

# Compile GLSL to create SPIR-V, then bin2h to convert it to a header
add_glsl_bin2h_command(vector_add_shader ${comp} ${header})
add_custom_target(vector_add_shader DEPENDS ${header})

add_ca_executable(vkVectorAddition
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.c ${header})
target_link_libraries(vkVectorAddition PRIVATE VK)

# Look in binary directory for generated header
target_include_directories(vkVectorAddition PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR})

if(TARGET tidy-vkVectorAddition)
  add_dependencies(tidy-vkVectorAddition vector_add_shader)
endif()

add_ca_check(vkVectorAddition
  COMMAND vkVectorAddition
  DEPENDS vkVectorAddition)

install(TARGETS vkVectorAddition RUNTIME DESTINATION bin COMPONENT VKExamples)
