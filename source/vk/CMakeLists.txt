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

if(NOT CA_RUNTIME_COMPILER_ENABLED)
  message(FATAL_ERROR
    "The vk API is not supported when CA_RUNTIME_COMPILER_ENABLED is OFF")
endif()

ca_option(CA_VK_LIBRARY_NAME STRING
  "Output name of the Vulkan library" "VK")

# Include our custom CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set useful variables used in subdirectories.
set(VK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_ca_library(VK SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/allocator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/buffer_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/command_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/descriptor_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/descriptor_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/descriptor_set_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/device_memory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/error.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/fence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/icd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/image_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/physical_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/pipeline_cache.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/query_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/semaphore.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/shader_module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/small_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/type_traits.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vk/unique_ptr.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/descriptor_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/descriptor_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/descriptor_set_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/device_memory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/error.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/physical_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pipeline_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pipeline_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/query_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/sampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/shader_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)

target_include_directories(VK PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(VK SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/external/Khronos/include)

# TODO: has some llvm dependencies via spirv-ll/module.h. This should be looked at
target_include_directories(VK SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIR})

# TODO(CA-1643): Add fully configurable extension options.
target_compile_definitions(VK PRIVATE
  CA_VK_KHR_get_physical_device_properties2=1
  CA_VK_KHR_storage_buffer_storage_class=1
  CA_VK_KHR_variable_pointers=1)
target_link_libraries(VK
  PRIVATE Threads::Threads builtins cargo mux compiler-static spirv-ll)
target_resources(VK NAMESPACES ${BUILTINS_NAMESPACES})

if(TARGET LLVMSupport)
  # Only link against LLVMSupport when NDEBUG is defined and the target exists.
  target_link_libraries(VK PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:ReleaseAssert>>:LLVMSupport>)
endif()

# Choose library linker options.
if(CA_PLATFORM_LINUX OR MINGW)
  # Export API entry point symbols for Linux, or if we're using a GNU compiler
  # on Windows.  I.e. a MinGW build on Windows uses the Linux symbol exports,
  # not the Windows ones.
  if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    set(VK_EXPORT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/source/export-linux.sym)
    # Retain only the symbols listed in the file filename, discarding all others
    set(VK_LINK_FLAGS
      "${VK_LINK_FLAGS} -Xlinker --retain-symbols-file=${VK_EXPORT_FILE}")
  endif()
  # Use internal library defined functions, required for vkGetInstanceProcAddr,
  # vkGetDeviceProcAddr, overriding new and delete.
  set(VK_LINK_FLAGS "${VK_LINK_FLAGS} -Xlinker -Bsymbolic")
elseif(CA_PLATFORM_WINDOWS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Set exports definition file for MSVC Windows (not MinGW, that takes the
  # Linux equivalent path) and link flags to use it.
  set(VK_EXPORT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/source/export-windows.def)
  set(VK_LINK_FLAGS "${VK_LINK_FLAGS} /DEF:${VK_EXPORT_FILE}")
else()
  message(WARNING "Unhandled build environment.")
endif()

# Specify chosen library linker options, use the specified output name and
# version of the Vulkan library.
set_target_properties(VK PROPERTIES LINK_FLAGS "${VK_LINK_FLAGS}"
  OUTPUT_NAME ${CA_VK_LIBRARY_NAME} SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# Create directory for the ICD json manifest file.
set(VKICDManifestDir share/vulkan/icd.d)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${VKICDManifestDir})

# Generate ICD json manifest file.
set(VKICDManifestScript ${VK_SOURCE_DIR}/scripts/VKICDManifest.py)
set(VKICDManifestFile ${PROJECT_BINARY_DIR}/${VKICDManifestDir}/VK.json)
add_custom_command(TARGET VK PRE_BUILD
  COMMAND ${PYTHON_EXECUTABLE} ${VKICDManifestScript}
  "$<TARGET_FILE:VK>" "${VKICDManifestFile}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${VKICDManifestScript} vulkan.h
  COMMENT "Generate VK ICD manifest.")

if(COMMAND add_coverage_modules)
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/VK.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vk
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/VK.dir/source)
endif()

if(CA_ENABLE_TESTS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
endif()
if(CA_ENABLE_EXAMPLES)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/external/Khronos/include/vulkan
  DESTINATION include COMPONENT VKHeaders)
install(TARGETS VK
  ARCHIVE DESTINATION lib COMPONENT VK
  LIBRARY DESTINATION lib COMPONENT VK
  RUNTIME DESTINATION bin COMPONENT VK)
install(FILES ${VKICDManifestFile}
  DESTINATION ${VKICDManifestDir} COMPONENT VK)
