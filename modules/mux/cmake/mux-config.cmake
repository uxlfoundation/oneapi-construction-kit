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

#[=======================================================================[.rst:
A `CMake script`_ for creating the ``config.h`` header used to provide
an API for selecting mux devices.

Script should be invoked using ``-P`` as part of an internal
:cmake-variable:`CMAKE_COMMAND`. The following CMake input variables are
required to be set:

* :cmake:variable:`MUX_TARGET_LIBRARIES` - Input variable containing a list of
  Mux targets.

* :cmake:variable:`MUX_CONFIG_HEADER` - Input variable containing the path to
  the header file to be written to.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

# Make sure this is actually a list, it may be passed in with space separators
# due to CMake deconstructing the list into a space separated string.
if(NOT ${MUX_TARGET_LIBRARIES} STREQUAL "")
  string(REPLACE "," ";" MUX_TARGET_LIBRARIES ${MUX_TARGET_LIBRARIES})
endif()

file(WRITE ${MUX_CONFIG_HEADER} "\
// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the \"License\") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an \"AS IS\" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// AUTO-GENERATED FILE - DO NOT EDIT (CMake will replace it if you do!)
// To make changes to this file update modules/mux/cmake/mux-config.cmake

#ifndef MUX_CONFIG_H_INCLUDED
#define MUX_CONFIG_H_INCLUDED

#include <mux/mux.h>
")

foreach(TARGET ${MUX_TARGET_LIBRARIES})
  file(APPEND ${MUX_CONFIG_HEADER} "\
#include <${TARGET}/${TARGET}.h>
")
endforeach()

file(APPEND ${MUX_CONFIG_HEADER} "
#ifdef __cplusplus
extern \"C\" {
#endif  // __cplusplus
")

file(APPEND ${MUX_CONFIG_HEADER} "
enum mux_target_index_e {
")
set(COUNTER 0)
foreach(TARGET ${MUX_TARGET_LIBRARIES})
  file(APPEND ${MUX_CONFIG_HEADER} "\
  mux_target_index_${TARGET} = ${COUNTER},
")
  math(EXPR COUNTER "${COUNTER} + 1")
endforeach()
list(LENGTH MUX_TARGET_LIBRARIES MUX_TARGET_COUNT)
file(APPEND ${MUX_CONFIG_HEADER} "\
  mux_target_count = ${MUX_TARGET_COUNT}
};
")

file(APPEND ${MUX_CONFIG_HEADER} "
enum mux_target_id_e {
")
set(COUNTER 1)
foreach(TARGET ${MUX_TARGET_LIBRARIES})
  file(APPEND ${MUX_CONFIG_HEADER} "\
  mux_target_id_${TARGET} = ${COUNTER},
")
  math(EXPR COUNTER "${COUNTER} + 1")
endforeach()
file(APPEND ${MUX_CONFIG_HEADER} "\
  mux_target_id_device_mask = 0x000000ff,
  mux_target_id_hook_mask = 0x0000ff00,
  mux_target_id_mask = 0x0000ffff
};
")

file(APPEND ${MUX_CONFIG_HEADER} "
typedef mux_result_t (*muxGetDeviceInfos_t)(
    uint32_t device_types, uint64_t device_infos_length,
    mux_device_info_t* out_device_infos, uint64_t* out_device_infos_length);

typedef mux_result_t (*muxCreateDevices_t)(uint64_t devices_length,
                                            mux_device_info_t* device_infos,
                                            mux_allocator_info_t allocator,
                                            mux_device_t* out_devices);

muxGetDeviceInfos_t* muxGetGetDeviceInfosHooks();
muxCreateDevices_t* muxGetCreateDevicesHooks();
")

file(APPEND ${MUX_CONFIG_HEADER} "
#ifdef __cplusplus
#define STRINGIFY_HELPER(TOKEN) #TOKEN
#define STRINGIFY(TOKEN) STRINGIFY_HELPER(TOKEN)

")
foreach(TARGET_LOWER ${MUX_TARGET_LIBRARIES})
  string(TOUPPER ${TARGET_LOWER} TARGET)
  file(APPEND ${MUX_CONFIG_HEADER} "\
static_assert(MUX_VERSION == ${TARGET}_VERSION,
    \"MUX_VERSION \"
    STRINGIFY(MUX_MAJOR_VERSION) \".\"
    STRINGIFY(MUX_MINOR_VERSION) \".\"
    STRINGIFY(MUX_PATCH_VERSION) \" \"
    \"does not match ${TARGET}_VERSION \"
    STRINGIFY(${TARGET}_MAJOR_VERSION) \".\"
    STRINGIFY(${TARGET}_MINOR_VERSION) \".\"
    STRINGIFY(${TARGET}_PATCH_VERSION) \", \"
    \"see '<mux>/CHANGES.md' for more information.\");
")
endforeach()
file(APPEND ${MUX_CONFIG_HEADER} "\

#undef STRINGIFY
#undef STRINGIFY_HELPER
#endif  // __cplusplus
")

file(APPEND ${MUX_CONFIG_HEADER} "
#ifdef MUX_CONFIG_INSTANTIATE_IMPLEMENTATION
static muxGetDeviceInfos_t mux_get_device_infos_hooks[] = {
")
foreach(TARGET ${MUX_TARGET_LIBRARIES})
  file(APPEND "${MUX_CONFIG_HEADER}" "\
    &${TARGET}GetDeviceInfos,
")
endforeach()
  file(APPEND ${MUX_CONFIG_HEADER} "\
};

static muxCreateDevices_t mux_create_devices_hooks[] = {
")
foreach(TARGET ${MUX_TARGET_LIBRARIES})
  file(APPEND "${MUX_CONFIG_HEADER}" "\
    &${TARGET}CreateDevices,
")
endforeach()
file(APPEND ${MUX_CONFIG_HEADER} "\
};

muxGetDeviceInfos_t* muxGetGetDeviceInfosHooks() {
  return mux_get_device_infos_hooks;
}

muxCreateDevices_t* muxGetCreateDevicesHooks() {
  return mux_create_devices_hooks;
}
#endif  // MUX_CONFIG_INSTANTIATE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // MUX_CONFIG_H_INCLUDED
")
