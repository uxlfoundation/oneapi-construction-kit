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

function(add_baked_data TARGET NAME HEADER SRC)
  set(DATA_SRC ${SRC})
  if (NOT IS_ABSOLUTE ${DATA_SRC})
    set(DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${DATA_SRC})
  endif()
  add_bin2h_target(${NAME} ${DATA_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${HEADER})

  add_dependencies(${TARGET} ${NAME})

  target_include_directories(${TARGET} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
  )
endfunction()
