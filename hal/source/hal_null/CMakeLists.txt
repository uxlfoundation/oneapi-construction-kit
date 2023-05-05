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

include(GNUInstallDirs) # Needed for CMAKE_INSTALL_LIBDIR

add_library(hal_null SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_null.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/hal_null.cpp)

target_include_directories(hal_null PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(hal_null PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

install(
  TARGETS hal_null
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries(hal_null PUBLIC hal_common)
target_compile_options(hal_null PRIVATE -DBUILD_HAL_DLL)
