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

macro(add_ca_api_subdirectory api)
  string(TOUPPER "${api}" apiToUpper)
  list(FIND CA_ENABLE_API "${api}" apiIndex)
  if(CA_ENABLE_API STREQUAL "" OR NOT ${apiIndex} EQUAL -1)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${api})
      message(STATUS "API enabled: ${api}")
      set(CA_${apiToUpper}_ENABLED ON CACHE INTERNAL "${api} enabled flag")
      add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${api})
    else()
      message(STATUS "API disabled: ${api} - source is not available!")
    endif()
  else()
    message(STATUS "API disabled: ${api}")
    set(CA_${apiToUpper}_ENABLED OFF CACHE INTERNAL "${api} enabled flag")
  endif()
endmacro()

# The modules source directory is not a subdirectory of source so we must also
# set the binary directory.
add_subdirectory(${PROJECT_SOURCE_DIR}/modules ${PROJECT_BINARY_DIR}/modules)

add_ca_api_subdirectory(cl)
add_ca_api_subdirectory(vk)
