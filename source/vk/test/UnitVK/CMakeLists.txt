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

option(UNITVK_USE_LOADER "Use the system's Vulkan loader." OFF)

find_package(SpirvTools COMPONENTS spirv-as)

# The glsl.cmake script defines the UVK_GLSL_FILES variable which contains a list
# of GLSL compute shader test input filenames.
include(${CMAKE_CURRENT_SOURCE_DIR}/source/shaders/glsl.cmake)


# The spvasm.cmake script defines the UVK_SPVASM_FILES variable which contains a
# list of SPIR-V assembly test input filenames.
include(${CMAKE_CURRENT_SOURCE_DIR}/source/shaders/spvasm.cmake)

# The cl.cmake script defines the UVK_CL_FILES variable which contains a
# list of OpenCL C test input filenames.
include(${CMAKE_CURRENT_SOURCE_DIR}/source/kernels/cl.cmake)

# Start the list of GLSL builtins afresh
unset(UVK_GLSL_SHADER_NAMES CACHE)

foreach(glsl ${UVK_GLSL_FILES})
  get_filename_component(name ${glsl} NAME_WE)
  if("${UVK_GLSL_SHADER_NAMES}" MATCHES "${name}")
    message(FATAL_ERROR "${name} already added to list of GLSL shaders!")
  endif()
  list(APPEND UVK_GLSL_SHADER_NAMES ${name})
endforeach()

if(TARGET spirv::spirv-as)
  foreach(spvasm ${UVK_SPVASM_FILES})
    get_filename_component(name ${spvasm} NAME_WE)
    if("${UVK_GLSL_SHADER_NAMES}" MATCHES "${name}")
      message(FATAL_ERROR "${name} already added to list of GLSL shaders!")
    endif()
    list(APPEND UVK_GLSL_SHADER_NAMES ${name})
  endforeach()
  find_package(Clspv)

  foreach(filename ${UVK_CL_FILES})
    string(REGEX REPLACE "\\.cl" "" name ${filename})
    string(REGEX REPLACE "\\." "_" name_esc ${name})
    if(Clspv_FOUND)
      set(input ${CMAKE_CURRENT_SOURCE_DIR}/source/kernels/${filename})
      set(output ${CMAKE_CURRENT_SOURCE_DIR}/source/shaders/${name_esc}.spvasm)
      # Use -pod-ubo and -cluster-pod-kernel-args to put all primitives into a
      # single uniform buffer.
      file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
      add_custom_command(OUTPUT ${output}
        COMMAND clspv -pod-ubo -cluster-pod-kernel-args -S ${input} -o ${output}
        DEPENDS ${input} COMMENT "Building SPIR-V file ${relOut}")
      list(APPEND SPVASM_FILES ${output})
    endif()
    list(APPEND UVK_SHARED_SHADER_NAMES ${name})
    list(APPEND UVK_GLSL_SHADER_NAMES ${name_esc})
  endforeach()
  if(Clspv_FOUND)
    add_custom_target(kts-spvasm DEPENDS ${SPVASM_FILES})
  endif()
  list(APPEND SHARED_TESTS ${KTS_SHARED_TEST_FILES})
endif()

set(UVK_GLSL_SHADER_NAMES ${UVK_GLSL_SHADER_NAMES}
  CACHE INTERNAL "UnitVK GLSL extended instruction shaders")

# List of names of .ll files to be translated into .bc files, then turned into
# .h which contain the binary data in an unsigned char array. The generated .h
# files are then included in the generated ShaderCode.cpp file. To gain access
# to the binary data in tests include the generated ShaderCode.h header and
# call the uvk::getShader() function passing it a uvk::Shader::<name> as the
# parameter.
set(UVK_SHADER_NAMES
  nop
  mov
  spec_const
  push_constant
  mov_1d_buffer
  fill_buffer_2_elems
  runtime_array
  num_work_groups
  work_group_id
  local_invocation_id
  global_invocation_id
  local_invocation_index
  ${UVK_GLSL_SHADER_NAMES}
  glsl_round
  glsl_step # <- TODO: not (yet) used
  all
  any
  bitcount
  dot
  fmod
  isinf
  isnan
  chain
  delay
  write_back
  turns)

if(${UNITVK_USE_LOADER})
  # Find Vulkan loader lib
  include(FindVulkan)
  if(NOT ${VULKAN_FOUND})
    set(VULKAN_LIBRARY VK)
  endif()
else()
  set(VULKAN_LIBRARY VK)
endif()

# Check the list of project names and add the relevant source files to the
# EXTERNAL_SRC variable. This will be blank normally but will inject tests
# defined by other projects.
foreach(NAME ${MUX_TARGET_LIBRARIES})
  set(CA_EXTERNAL_UNITVK_SRC
    ${CA_EXTERNAL_UNITVK_SRC}
    ${${NAME}_EXTERNAL_UNITVK_SRC})
endforeach()

add_ca_executable(UnitVK
  ${CMAKE_CURRENT_SOURCE_DIR}/include/GLSLTestDefs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ktst_clspv_common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts_vk.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts_vk.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/UnitVK.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AbacusInstructions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AllocateCommandBuffers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AllocateDescriptorSets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/AllocateMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/BeginCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/BindBufferMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/BindImageMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdBindDescriptorSets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdBindPipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdCopyBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdDispatch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdFillBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdPipelineBarrier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdPushConstants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdResetEvent.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdSetEvent.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdUpdateBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CmdWaitEvents.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateBufferView.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateCommandPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateComputePipelines.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateDescriptorPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateDescriptorSetLayout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateDevice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateEvent.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateFence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateImage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateInstance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreatePipelineCache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreatePipelineLayout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateQueryPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateSampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateSemaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/CreateShaderModule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DestroyNullHandle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/DeviceWaitIdle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/EndCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/EnumerateInstanceLayerProperties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/EnumeratePhysicalDevices.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/EnumerateDeviceLayerProperties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionAbs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionBarrier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionClamp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionCross.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionExp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionFMA.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionFP.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionMatrix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionMinMax.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionMix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionMod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionNorms.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionPacking.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionPow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionQuantize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionRoots.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionRounding.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionSign.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionSignificantBit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionStep.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionTrigonConv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionTrigonFuncs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionVecCalcs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/FlushMappedMemoryRanges.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetBufferMemoryRequirements.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetDeviceQueue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetDeviceMemoryCommitment.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetFenceStatus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetPhysicalDeviceFeatures.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetPhysicalDeviceProperties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetPhysicalDeviceMemoryProperties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetPhysicalDeviceQueueFamilyProperties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/GetPipelineCacheData.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/InvalidateMappedMemoryRanges.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/MapMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/MergePipelineCaches.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/QueueSubmit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/QueueWaitIdle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ResetCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ResetCommandPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ResetDescriptorPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ResetEvent.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ResetFence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/Semaphores.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/SimultaneousUse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/UnitVK.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/UpdateDescriptorSets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/WaitForFences.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_basic.cpp
  ${CA_EXTERNAL_UNITVK_SRC})

target_include_directories(UnitVK PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/source/shaders)
target_link_libraries(UnitVK PRIVATE ${VULKAN_LIBRARY} kts cargo)

if(UNITVK_USE_LOADER)
  target_compile_definitions(UnitVK PRIVATE UNITVK_USE_LOADER)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/shaders)

target_ca_sources(UnitVK PUBLIC
  ${UVK_SHADER_CODE_HEADER}
  ${UVK_SHADER_CODE_SOURCE})

if(TARGET spirv::spirv-as)
  # Add tests using clspv generated SPIR-V
  target_ca_sources(UnitVK PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_dma.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_regression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_01.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_02.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_03.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_04.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_05.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_06.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_07.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_08.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_09.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_vecz_tasks_task_10.cpp)
endif()

if(NOT TARGET spirv::spirv-as)
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionFP.cpp
    PROPERTIES COMPILE_DEFINITIONS "IGNORE_SPIRV_TESTS")
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ExecutionMod.cpp
    PROPERTIES COMPILE_DEFINITIONS "IGNORE_SPIRV_TESTS")
endif()

add_dependencies(UnitVK UnitVKShaderCode VK)
if(TARGET tidy-UnitVK)
  # It is the second UnitVK tidy target (tidy-UnitVK_) that requires shaders,
  # so set the dependency there after checking if the *first* exists (because
  # `_` is really an internal implementation detail and could change, this way
  # there will be a noisy error if that happens).
  add_dependencies(tidy-UnitVK_ UnitVKShaderCode)
endif()

# Add UnitVK tests to the check target.
add_ca_check(UnitVK GTEST
  COMMAND UnitVK --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitVK.xml
  CLEAN ${PROJECT_BINARY_DIR}/UnitVK.xml
  DEPENDS UnitVK)

install(TARGETS UnitVK RUNTIME DESTINATION bin COMPONENT UnitVK)
