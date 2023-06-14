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

get_property(CA_LIT_CONFIG_FILES GLOBAL PROPERTY CA_LIT_CONFIG_FILES)
list(LENGTH CA_LIT_CONFIG_FILES file_len)
math(EXPR file_last "${file_len} - 1")

set(CA_LIT_CONFIG_MAP "")

if (${file_last} GREATER -1)
  foreach(i RANGE 0 ${file_last} 2)
    list(GET CA_LIT_CONFIG_FILES ${i} main_config)
    math(EXPR i1 "${i} + 1")
    list(GET CA_LIT_CONFIG_FILES ${i1} site_out)
    set(map "map_config(path(r'${main_config}'), path(r'${site_out}'))")
    set(CA_LIT_CONFIG_MAP "${CA_LIT_CONFIG_MAP}\n${map}")
  endforeach()
endif()

# Note: not using add_ca_configure_file as it doesn't parse CA_LIT_CONFIG_MAP
# correctly
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ca-lit.in
  ${CMAKE_CURRENT_BINARY_DIR}/ca-lit)

set(SUITE ca-common)

# Provide common LIT utilities for test suites
add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.common.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.common.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.common.cfg")

if(TARGET ${SUITE}-lit)
  # Copy utilities into the binary directory
  add_custom_command(TARGET ${SUITE}-lit
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_BINARY_DIR}/utils)
endif()
