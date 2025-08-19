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
{% if cookiecutter.copyright_name != "" -%}
# Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
# Reserved.
{% endif -%}

project({{cookiecutter.target_name}})
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CA_EXTERNAL_MUX_TARGET_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/mux/{{cookiecutter.target_name}}"
  CACHE STRING "override" FORCE)
set(CA_EXTERNAL_MUX_COMPILER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/compiler/{{cookiecutter.target_name}}"
  CACHE STRING "override" FORCE)

set(CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR
   "${CMAKE_CURRENT_SOURCE_DIR}/hal_{{cookiecutter.target_name}}" CACHE STRING "External  oneAPI Construction Kit HAL")

set(CA_EXTERNAL_ONEAPI_CON_KIT_DIR
   "${CMAKE_CURRENT_SOURCE_DIR}/ONEAPI_KIT" CACHE STRING "External oneAPI Construction Kit")

add_subdirectory(${CA_EXTERNAL_ONEAPI_CON_KIT_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/oneAPIConstructionKit)
