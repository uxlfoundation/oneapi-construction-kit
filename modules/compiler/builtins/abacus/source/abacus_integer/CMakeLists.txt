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

set(abacus_integer_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/abs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/abs_diff.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/add_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clamp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clz.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hadd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad24.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad_hi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/max.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/min.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mul24.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mul_hi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/popcount.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rhadd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rotate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/upsample.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ctz.cpp
  PARENT_SCOPE)
