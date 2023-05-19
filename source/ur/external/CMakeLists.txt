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

include(FetchContent)

FetchContent_Declare(
  UNIFIED_RUNTIME
  GIT_REPOSITORY https://github.com/oneapi-src/unified-runtime.git
  GIT_TAG        b2946f1f0b64641a0393b9d119fb022ccf26bc74
)

# Fetch unified runtime from github. This also sets unified_runtime_SOURCE_DIR
FetchContent_Populate(UNIFIED_RUNTIME)  

add_library(unified-runtime-headers INTERFACE)
target_include_directories(unified-runtime-headers INTERFACE
  ${unified_runtime_SOURCE_DIR}/include)
