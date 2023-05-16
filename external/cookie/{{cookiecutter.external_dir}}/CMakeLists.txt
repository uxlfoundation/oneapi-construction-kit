# Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
# Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
# Reserved.
{% endif -%}

project({{cookiecutter.target_name}})
cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)

set(CA_EXTERNAL_MUX_TARGET_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/mux/{{cookiecutter.target_name}}"
  CACHE STRING "override" FORCE)
set(CA_EXTERNAL_MUX_COMPILER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/compiler/{{cookiecutter.target_name}}"
  CACHE STRING "override" FORCE)

set(CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR
   "${CMAKE_CURRENT_SOURCE_DIR}/hal_{{cookiecutter.target_name}}" CACHE STRING "External ComputeAorta HAL")

set(CA_EXTERNAL_ONEAPI_CON_KIT_DIR
   "${CMAKE_CURRENT_SOURCE_DIR}/ONEAPI_KIT" CACHE STRING "External OneAPI Construction Kit")

add_subdirectory(${CA_EXTERNAL_ONEAPI_CON_KIT_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/ComputeAorta)
