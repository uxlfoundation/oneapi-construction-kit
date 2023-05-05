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

set(abacus_relational_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/all.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/any.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bitselect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isfinite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isgreater.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isgreaterequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isinf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isless.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/islessequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/islessgreater.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnormal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnotequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isordered.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isunordered.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/select.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/signbit.cpp
  PARENT_SCOPE)
