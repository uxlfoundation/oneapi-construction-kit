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

set(SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py)
add_custom_target(check
    COMMAND ${Python3_EXECUTABLE} ${SCRIPT_PATH} -L lib -b bin --strict --timeout 00:01:00
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL)

add_dependencies(check ClikExamples)
if(NOT CLIK_HAL_NAME STREQUAL "")
  add_dependencies(check hal_${CLIK_HAL_NAME})
endif()
