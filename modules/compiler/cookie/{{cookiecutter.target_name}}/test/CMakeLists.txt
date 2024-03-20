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

{% if "clmul" in cookiecutter.feature.split(";") or "refsi_wrapper_pass" in cookiecutter.feature.split(";") -%}
set(SUITE "{{cookiecutter.target_name}}")
add_ca_configure_lit_site_cfg(
  ${SUITE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg")

if(TARGET ${SUITE}-lit)
  add_ca_lit_testsuite(${SUITE}
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS muxc ${SUITE}-lit)
endif()
{% else -%}
# Use add_ca_configure_lit_site_cfg and add_ca_lit_testsuite to add lit tests
{% endif -%}
