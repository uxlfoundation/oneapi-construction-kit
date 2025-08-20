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

set(abacus_common_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/clamp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/degrees.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/max.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/min.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/radians.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sign.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/smoothstep.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/step.cpp
  PARENT_SCOPE)
