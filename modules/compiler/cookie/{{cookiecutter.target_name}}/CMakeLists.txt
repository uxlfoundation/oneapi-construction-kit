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
{% if cookiecutter.copyright_name != "" -%}
# Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
# Reserved.
{% endif -%}

# If the {{cookiecutter.target_name}} target is disabled, then the compiler should be too.
if(NOT TARGET {{cookiecutter.target_name}})
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
{% if "clmul"  in cookiecutter.feature.split(";") -%}
  add_subdirectory(extension)
{% endif %}

  set({{cookiecutter.target_name_capitals}}_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/{{cookiecutter.target_name}}_pass_machinery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/{{cookiecutter.target_name}}_pass_registry.def
  ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
${CMAKE_CURRENT_SOURCE_DIR}/source/passes/refsi_wrapper_pass.cpp
${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/refsi_wrapper_pass.h
{% endif %} 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/target.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h 
  )

  add_ca_library(compiler-{{cookiecutter.target_name}} STATIC ${{"{"}}{{cookiecutter.target_name_capitals}}_SOURCES})

  ca_option(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE BOOL "Enable ComputeAorta {{cookiecutter.target_name_capitals}} demo mode" OFF)
  if(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  target_compile_definitions(compiler-{{cookiecutter.target_name}} PRIVATE
    CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE=$<BOOL:${{"{"}}CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE}>)
endif()

  target_include_directories(compiler-{{cookiecutter.target_name}} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-{{cookiecutter.target_name}} PUBLIC
    compiler-base
    utils
    compiler-pipeline
    compiler-binary-metadata
    compiler-linker-utils
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMVectorize LLVMipo multi_llvm)
  
  add_mux_compiler_target(compiler-{{cookiecutter.target_name}}
    COMPILER_INFO {{cookiecutter.target_name}}::{{cookiecutter.target_name.capitalize()}}Info::get
    HEADER_FILE {{cookiecutter.target_name}}/info.h)

  if(CA_ENABLE_TESTS)
    add_subdirectory(test)
  endif()
endif()
