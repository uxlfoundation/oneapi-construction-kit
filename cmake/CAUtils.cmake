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

# TODO Describe params and options
function(ca_message mode verbosity_level ...)
    set(CA_MESSAGE_VERBOSITY_LEVEL 0)
    if (${verbosity_level} STREQUAL  "CA_VERBOSE_SUMMARY")
        set(CA_MESSAGE_VERBOSITY_LEVEL 1)
    elseif(${verbosity_level} STREQUAL  "CA_VERBOSE_DETAIL")
        set(CA_MESSAGE_VERBOSITY_LEVEL 2)
    endif()
    list(REMOVE_AT ARGV 0)
    list(REMOVE_AT ARGV 0)
    if (${CA_MESSAGE_VERBOSITY_LEVEL} LESS_EQUAL ${OCK_CMAKE_VERBOSE_LEVEL})
      message(${mode} ${ARGV})
    endif()
endfunction()
