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

# Compile common functions that are needed to execute kernels.
# Kernels will be linked against these objects.
set(IO_OBJ ${CMAKE_BINARY_DIR}/source/common/io.o)
hal_refsi_tutorial_compile_kernel_source(${IO_OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/io.c)
add_custom_target(refsi_tutorial_device_common DEPENDS ${IO_OBJ})
set_property(GLOBAL APPEND PROPERTY HAL_REFSI_TUTORIAL_EXTRA_OBJECTS ${IO_OBJ})
