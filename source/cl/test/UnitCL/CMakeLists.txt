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

if(CA_CL_ENABLE_OFFLINE_KERNEL_TESTS AND
    NOT CA_RUNTIME_COMPILER_ENABLED AND NOT CA_EXTERNAL_CLC)
  message(FATAL_ERROR "CA_CL_ENABLE_OFFLINE_KERNEL_TESTS is set, but "
    "CA_RUNTIME_COMPILER_ENABLED=OFF and no external clc has been set. "
    "Either disable CA_CL_ENABLE_OFFLINE_KERNEL_TESTS, or enable "
    "CA_RUNTIME_COMPILER_ENABLED, or set CA_EXTERNAL_CLC.")
endif()

if(CMAKE_CROSSCOMPILING AND CA_CL_ENABLE_OFFLINE_KERNEL_TESTS AND
    NOT CMAKE_CROSSCOMPILING_EMULATOR AND NOT CA_EXTERNAL_CLC)
  message(FATAL_ERROR "Cross-compiling requires that qemu is installed, or "
    "that CA_EXTERNAL_CLC is set, or that CA_CL_ENABLE_OFFLINE_KERNEL_TESTS "
    "is disabled.")
endif()

# Add utility device name check function for each device to Device.h
set(device_check_boilerplate)
# Function to check the device name
set(device_check_template
"#define UCL_TARGET_@TARGET@_EXISTS

inline bool isDevice_@TARGET@(cl_device_id device) {
  return isDevice(device, \"@TARGET_NAME@\", CL_DEVICE_TYPE_ALL);
}\n")

foreach(TARGET ${MUX_TARGET_LIBRARIES})
  set(TARGET_NAME "${${TARGET}_DEVICE_NAMES}")
  string(CONFIGURE "${device_check_template}" device_check_str @ONLY)
  set(device_check_boilerplate
    "${device_check_boilerplate}\n${device_check_str}")
endforeach()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Device.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/Device.h
  @ONLY)

add_ca_cl_executable(UnitCL
  # General testing machinery
  include/Common.h source/Common.cpp
  include/EventWaitList.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/Device.h
  source/main.cpp
  include/ucl/assert.h
  include/ucl/callbacks.h source/ucl/callbacks.cpp
  include/ucl/checks.h
  include/ucl/environment.h source/ucl/environment.cpp
  include/ucl/file.h
  include/ucl/fixtures.h source/ucl/fixtures.cpp
  include/ucl/types.h
  include/ucl/version.h

  # Kernel testing machinery
  include/kts/arguments.h source/kts/arguments.cpp
  include/kts/execution.h source/kts/execution.cpp source/kts/fp16_execution.cpp
  include/kts/generator.h
  include/kts/type_info.h
  include/kts/precision.h source/kts/precision.cpp
  include/kts/printf.h
  include/kts/relationals.h source/kts/relationals.cpp
  include/kts/sub_group_helpers.h source/kts/sub_group_helpers.cpp

  # Core tests
  source/KernelArgumentTypes.cpp
  source/clBuildProgram.cpp
  source/clCompileProgram.cpp
  source/clCreateBuffer.cpp
  source/clCreateCommandQueue.cpp
  source/clCreateContext.cpp
  source/clCreateContextFromType.cpp
  source/clCreateImage.cpp
  source/clCreateKernel.cpp
  source/clCreateKernelsInProgram.cpp
  source/clCreateProgramWithBinary.cpp
  source/clCreateProgramWithBuiltInKernels.cpp
  source/clCreateProgramWithSource.cpp
  source/clCreateSampler.cpp
  source/clCreateSubBuffer.cpp
  source/clCreateSubDevices.cpp
  source/clCreateUserEvent.cpp
  source/clEnqueueBarrier.cpp
  source/clEnqueueBarrierWithWaitList.cpp
  source/clEnqueueCopyBuffer.cpp
  source/clEnqueueCopyBufferRect.cpp
  source/clEnqueueCopyBufferToImage.cpp
  source/clEnqueueCopyImage.cpp
  source/clEnqueueCopyImageToBuffer.cpp
  source/clEnqueueFillBuffer.cpp
  source/clEnqueueFillImage.cpp
  source/clEnqueueMapBuffer.cpp
  source/clEnqueueMapImage.cpp
  source/clEnqueueMarker.cpp
  source/clEnqueueMarkerWithWaitList.cpp
  source/clEnqueueMigrateMemObjects.cpp
  source/clEnqueueNDRangeKernel.cpp
  source/clEnqueueNativeKernel.cpp
  source/clEnqueueReadBuffer.cpp
  source/clEnqueueReadBufferRect.cpp
  source/clEnqueueReadImage.cpp
  source/clEnqueueTask.cpp
  source/clEnqueueUnMapMemObject.cpp
  source/clEnqueueWaitForEvents.cpp
  source/clEnqueueWriteBuffer.cpp
  source/clEnqueueWriteBufferRect.cpp
  source/clEnqueueWriteImage.cpp
  source/clFinish.cpp
  source/clFlush.cpp
  source/clGetCommandQueueInfo.cpp
  source/clGetContextInfo.cpp
  source/clGetDeviceIDs.cpp
  source/clGetDeviceInfo.cpp
  source/clGetEventInfo.cpp
  source/clGetEventProfilingInfo.cpp
  source/clGetExtensionFunctionAddress.cpp
  source/clGetExtensionFunctionAddressForPlatform.cpp
  source/clGetImageInfo.cpp
  source/clGetKernelArgInfo.cpp
  source/clGetKernelInfo.cpp
  source/clGetKernelWorkGroupInfo.cpp
  source/clGetMemObjectInfo.cpp
  source/clGetPlatformIDs.cpp
  source/clGetPlatformInfo.cpp
  source/clGetProgramBuildInfo.cpp
  source/clGetProgramInfo.cpp
  source/clGetSamplerInfo.cpp
  source/clGetSupportedImageFormats.cpp
  source/clLinkProgram.cpp
  source/clReleaseCommandQueue.cpp
  source/clReleaseContext.cpp
  source/clReleaseDevice.cpp
  source/clReleaseEvent.cpp
  source/clReleaseKernel.cpp
  source/clReleaseMemObject.cpp
  source/clReleaseProgram.cpp
  source/clReleaseSampler.cpp
  source/clRetainCommandQueue.cpp
  source/clRetainContext.cpp
  source/clRetainDevice.cpp
  source/clRetainEvent.cpp
  source/clRetainKernel.cpp
  source/clRetainMemObject.cpp
  source/clRetainProgram.cpp
  source/clRetainSampler.cpp
  source/clSetEventCallback.cpp
  source/clSetKernelArg.cpp
  source/clSetMemObjectDestructorCallback.cpp
  source/clSetUserEventStatus.cpp
  source/clUnloadCompiler.cpp
  source/clUnloadPlatformCompiler.cpp
  source/clWaitForEvents.cpp
  source/fastmath.cpp
  source/fuzz_test.cpp
  source/kernelFeatures.cpp
  source/limits.cpp
  source/macros.cpp
  source/printfBuiltin.cpp
  source/softmath.cpp

  # Regression tests
  source/arm64Regression.cpp

  # Execution tests
  source/ktst_barrier.cpp
  source/ktst_builtins.cpp
  source/ktst_compiler.cpp
  source/ktst_compiler_barrier.cpp
  source/ktst_conversions.cpp
  source/ktst_dimensions.cpp
  source/ktst_dma.cpp
  source/ktst_geometric.cpp
  source/ktst_native.cpp
  source/ktst_precision.cpp
  source/ktst_printf.cpp
  source/ktst_regression_01.cpp
  source/ktst_regression_02.cpp
  source/ktst_regression_03.cpp
  source/ktst_regression_04.cpp
  source/ktst_regression_05.cpp
  source/ktst_relationals.cpp
  source/ktst_spirv.cpp
  source/ktst_vecz_tasks_task_01.cpp
  source/ktst_vecz_tasks_task_02.cpp
  source/ktst_vecz_tasks_task_03.cpp
  source/ktst_vecz_tasks_task_04.cpp
  source/ktst_vecz_tasks_task_05.cpp
  source/ktst_vecz_tasks_task_06.cpp
  source/ktst_vecz_tasks_task_07.cpp
  source/ktst_vecz_tasks_task_08.cpp
  source/ktst_vecz_tasks_task_09.cpp
  source/ktst_vecz_tasks_task_10.cpp
  source/ktst_vecz_tasks_task_11.cpp
  source/ktst_vecz_tasks_task_12.cpp
  source/ktst_vecz_tasks_task_14.cpp
  source/ktst_vecz_tasks_task_15.cpp
  source/ktst_vload_vstore.cpp

  # 3.0 tests
  source/C11Atomics.cpp
  source/FeatureMacros.cpp
  source/clCloneKernel.cpp
  source/clCreateBufferWithProperties.cpp
  source/clCreateCommandQueueWithProperties.cpp
  source/clCreatePipe.cpp
  source/clCreateProgramWithIL.cpp
  source/clEnqueueSVMFree.cpp
  source/clEnqueueSVMMap.cpp
  source/clEnqueueSVMMemFill.cpp
  source/clEnqueueSVMMemcpy.cpp
  source/clEnqueueSVMMigrateMem.cpp
  source/clEnqueueSVMUnmap.cpp
  source/clGetDeviceAndHostTimer.cpp
  source/clGetHostTimer.cpp
  source/clGetKernelSubGroupInfo.cpp
  source/clGetPipeInfo.cpp
  source/clSVMAlloc.cpp
  source/clSVMFree.cpp
  source/clSetContextDestructorCallback.cpp
  source/clSetDefaultDeviceCommandQueue.cpp
  source/clSetKernelArgSVMPointer.cpp
  source/clSetKernelExecInfo.cpp
  source/clSetProgramReleaseCallback.cpp
  source/clSetProgramSpecializationConstant.cpp
  source/ctz.cpp
  source/sub_groups.cpp
  source/work_group_collective_functions.cpp

  # Extension tests
  include/cl_codeplay_kernel_exec_info.h
  include/cl_codeplay_wfv.h
  include/cl_intel_unified_shared_memory.h
  source/cl_codeplay_extra_build_options/cl_codeplay_extra_build_options.cpp
  source/cl_codeplay_kernel_debug/flags.cpp
  source/cl_codeplay_kernel_exec_info/clSetKernelExecInfoCODEPLAY.cpp
  source/cl_codeplay_kernel_exec_info/usm.cpp
  source/cl_codeplay_performance_counters/cl_codeplay_performance_counters.cpp
  source/cl_codeplay_wfv/wfv_binary.cpp
  source/cl_codeplay_wfv/wfv_build_options.cpp
  source/cl_codeplay_wfv/wfv_errors.cpp
  source/cl_codeplay_wfv/wfv_kernel_info.cpp
  source/cl_codeplay_wfv/wfv_status.cpp
  source/cl_codeplay_wfv/wfv_widths.cpp
  source/cl_intel_required_subgroup_size/cl_intel_required_subgroup_size.cpp
  source/cl_intel_unified_shared_memory/usm_allocate.cpp
  source/cl_intel_unified_shared_memory/usm_capabilties.cpp
  source/cl_intel_unified_shared_memory/usm_enqueue.cpp
  source/cl_intel_unified_shared_memory/usm_event_info.cpp
  source/cl_intel_unified_shared_memory/usm_free.cpp
  source/cl_intel_unified_shared_memory/usm_hints.cpp
  source/cl_intel_unified_shared_memory/usm_kernel.cpp
  source/cl_intel_unified_shared_memory/usm_kernel_exec_info.cpp
  source/cl_intel_unified_shared_memory/usm_mem_cpy.cpp
  source/cl_intel_unified_shared_memory/usm_mem_fill.cpp
  source/cl_intel_unified_shared_memory/usm_mem_info.cpp
  source/cl_intel_unified_shared_memory/usm_mem_set.cpp
  source/cl_khr_3d_image_writes/cl3DImageWriteExtensionTest.cpp
  source/cl_khr_create_command_queue/clCreateCommandQueueWithPropertiesKHR.cpp
  source/cl_khr_icd/clIcdGetPlatformIDsKHR.cpp
  source/cl_khr_il_program/clCreateProgramWithILKHR.cpp

  ${CA_EXTERNAL_UNITCL_SRC})

if(${OCL_EXTENSION_cl_khr_command_buffer})
  target_ca_sources(UnitCL PRIVATE
    include/cl_khr_command_buffer.h
    source/cl_khr_command_buffer/clCommandBarrierWithWaitListKHR.cpp
    source/cl_khr_command_buffer/clCommandCopyBufferKHR.cpp
    source/cl_khr_command_buffer/clCommandCopyBufferRectKHR.cpp
    source/cl_khr_command_buffer/clCommandCopyBufferToImageKHR.cpp
    source/cl_khr_command_buffer/clCommandCopyImageKHR.cpp
    source/cl_khr_command_buffer/clCommandCopyImageToBufferKHR.cpp
    source/cl_khr_command_buffer/clCommandFillBufferKHR.cpp
    source/cl_khr_command_buffer/clCommandFillImageKHR.cpp
    source/cl_khr_command_buffer/clCommandNDRangeKernelKHR.cpp
    source/cl_khr_command_buffer/clCreateCommandBufferKHR.cpp
    source/cl_khr_command_buffer/clEnqueueCommandBufferKHR.cpp
    source/cl_khr_command_buffer/clEnqueueCommandBufferKHRWithEvents.cpp
    source/cl_khr_command_buffer/clFinalizeCommandBufferKHR.cpp
    source/cl_khr_command_buffer/clGetCommandBufferInfoKHR.cpp
    source/cl_khr_command_buffer/clReleaseCommandBufferKHR.cpp
    source/cl_khr_command_buffer/clRetainCommandBufferKHR.cpp
    source/cl_khr_command_buffer/thread_safety.cpp)
endif()

if(${OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch})
  target_ca_sources(UnitCL PRIVATE
    include/cl_khr_command_buffer_mutable_dispatch.h
    source/cl_khr_command_buffer_mutable_dispatch/clEnqueueCommandBufferKHRMutability.cpp
    source/cl_khr_command_buffer_mutable_dispatch/clGetMutableCommandInfoKHR.cpp
    source/cl_khr_command_buffer_mutable_dispatch/clUpdateMutableCommandsKHR.cpp
    source/cl_khr_command_buffer_mutable_dispatch/thread_safety.cpp
    source/cl_khr_command_buffer_mutable_dispatch/usm_arg_update.cpp)
endif()

if(${OCL_EXTENSION_cl_khr_extended_async_copies})
  target_ca_sources(UnitCL PRIVATE
    source/cl_khr_extended_async_copies/extended_async.cpp)
endif()
#cl_khr_extended_async_copies

target_include_directories(UnitCL PRIVATE
  include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Need to enable extensions for hexfloats in gcc >= 7.3
set_target_properties(UnitCL PROPERTIES CXX_EXTENSIONS ON)

# Note: Linking Debug UnitCL can be slow, especially on MinGW. If you are
# needing to rebuild UnitCL repeatedly, adding an -O1 here may help.
target_compile_options(UnitCL PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:QCC>>:
    -Wno-deprecated-declarations  # Disable: warnings about deprecated decls
  >
  $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,6.9>>:
    # Enable the C++17 feature which passes alignment to operator new, this is
    # required for Arm cross-compile due to new cl_int4 causing compile errors
    # for being over-aligned in UnitCL. This diagnostic was not present before
    # this feature was added and the use of new cl_int4 or similar in UnitCL is
    # deeply embedded in the Execution framework, enabling this feature is the
    # path of least resistance to fixing Arm cross-compilation.
    -faligned-new
  >
$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,8.9>>:
    # GCC 9+ seem to make -Wformat-overflow very aggressive with potential
    # false positives. Set the heuristics to 0, but only for 9-10.x in case 11.x
    # improves the heuristics.
    -Wformat-overflow=0
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    -wd4996  # Disable warning on deprecated declarations
  >)

string(REPLACE ";" "," MuxTargetLibraries "${MUX_TARGET_LIBRARIES}")
target_compile_definitions(UnitCL PRIVATE
  CA_VERSION=${PROJECT_VERSION} MUX_TARGETS="${MuxTargetLibraries}"
  CA_HOST_CL_DEVICE_NAME_PREFIX="${CA_HOST_CL_DEVICE_NAME_PREFIX}"
  $<$<BOOL:${CA_CL_ENABLE_OFFLINE_KERNEL_TESTS}>:
    CA_CL_ENABLE_OFFLINE_KERNEL_TESTS
  >
  # TODO(CA-3968): Revert when fixed.
  UNITCL_CROSSCOMPILING=$<BOOL:${CMAKE_CROSSCOMPILING}>)
target_link_libraries(UnitCL PRIVATE kts cargo)

foreach(NAME ${MUX_TARGET_LIBRARIES})
  # If the addition of external tests is enabled, check the list of project
  # names and add the relevant source files to UnitCL.
  target_ca_sources(UnitCL PRIVATE ${${NAME}_EXTERNAL_UNITCL_SRC})
  target_include_directories(UnitCL PRIVATE ${${NAME}_EXTERNAL_UNITCL_INC})
endforeach()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/share/kernels)
add_subdirectory(kernels)
add_dependencies(UnitCL copy-kernels)

if(CA_CL_ENABLE_OFFLINE_KERNEL_TESTS)
  target_ca_sources(UnitCL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ktst_offline.cpp)
endif()

install(TARGETS UnitCL RUNTIME DESTINATION bin COMPONENT UnitCL)

file(COPY test_include DESTINATION ${PROJECT_BINARY_DIR}/share)


#[=======================================================================[.rst:
.. cmake:command:: add_ca_unitcl_check

  A CMake command which builds upon :cmake:command:`add_ca_cl_check` to make
  specifying UnitCL configurations more concise.

  Arguments:
    * ``name`` A unique name for the check, should include the name of the
      target being checked.

  Keyword Arguments:
    * ``COMPILER`` Flag to specify a test filter to only run compiler tests.
    * ``EXTENDED`` Flag to disable check if
      :cmake:variable:`CA_CL_ENABLE_EXTENDED_CHECKS` is set to ``OFF``.
    * ``ARGS`` List of additional arguments to be passed to UnitCL for
      execution.
    * ``FILTER`` Custom filter to use. In conjunction with ``COMPILER`` this
      is *appended* to the default filter.
    * ``GROUP`` Test group to add this check to.
    * ``ENVIRONMENT`` List of environment variables to set when executing
      UnitCL.
#]=======================================================================]
function(add_ca_unitcl_check name)
  cmake_parse_arguments(args "COMPILER;EXTENDED" "" "ARGS;FILTER;GROUP;ENVIRONMENT" ${ARGN})
  if(args_EXTENDED AND NOT CA_CL_ENABLE_EXTENDED_CHECKS)
    return()
  endif()
  if(args_COMPILER)
    # We run UnitCL in many compiler configurations, but many UnitCL tests are
    # not especially affected by compiler flags. Thus for the compiler
    # variations we set a filter to select the most compiler intensive tests.
    # This is not done because we expect tests to fail in other configurations
    # (if they do that should be considered a bug), but simply to accelerate
    # our testing by minimising duplication.
    # Allow users to tack on any extra tests to run. They may also disable any
    # tests that we run by default.
    set(filter "--gtest_filter=*Compile*:*Link*:*Build*:*Execution*:*print*:*WorkGroupCollective*:*SubGroup*:${args_FILTER}")
  elseif(args_FILTER)
    set(filter "--gtest_filter=${args_FILTER}")
  endif()
  if(args_ENVIRONMENT)
    set(environment ENVIRONMENT ${args_ENVIRONMENT})
  endif()
  add_ca_cl_check(${name} GTEST
    COMMAND UnitCL "--unitcl_platform=Codeplay Software Ltd."
    --gtest_output=xml:${PROJECT_BINARY_DIR}/${name}.xml ${args_ARGS} ${filter}
    ${environment} CLEAN ${PROJECT_BINARY_DIR}/${name}.xml NOGLOBAL DEPENDS UnitCL)
  if (args_GROUP)
    add_dependencies(${args_GROUP} check-${name})
  endif()
endfunction()

# Set up two non-overlapping groups of UnitCL tests. One runs the 'vecz' tests,
# the other runs everything else.
add_ca_check_group(UnitCL-group)
add_ca_check_group(UnitCL-group-vecz)

# Convenience function to add a UnitCL check in the default group
function(add_ca_default_unitcl_check name)
  add_ca_unitcl_check(${name} ${ARGV} GROUP check-UnitCL-group)
endfunction()

# Convenience function to add a UnitCL check in the vecz group
function(add_ca_vecz_unitcl_check name)
  add_ca_unitcl_check(${name} ${ARGV} GROUP check-UnitCL-group-vecz)
endfunction()


# Note that this is the vanilla UnitCL configuration, i.e. it should not have
# any tests filtered out or any special flags set. `-cl-wfv` defaults to `auto`
# now, but we don't "--vecz-check" here since it may bail out by design.
add_ca_default_unitcl_check(UnitCL)

# This only provides minimal additional coverage over the `-cl-opt-disable
# -g` test, so only include this in the extended set.
add_ca_default_unitcl_check(UnitCL-opt-disable COMPILER EXTENDED
  ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-opt-disable -cl-wfv=never")

# This only provides minimal additional coverage over the `-cl-wfv=always
# -g` test, so only include this in the extended set.
add_ca_default_unitcl_check(UnitCL-debug COMPILER EXTENDED
  ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-g -cl-wfv=never")

add_ca_default_unitcl_check(UnitCL-prevec COMPILER EXTENDED
  ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-vec=all -cl-wfv=never")

# Running without optimizations but with VECZ provides additional testing of
# VECZ by essentially testing a different set of inputs.  Note that unlike
# other VECZ UnitCL checks these do not use `--vecz-check`, that is because
# we don't expect VECZ to successfully vectorize all kernels in this
# circumstance as optimizations perform canonicalizations that we rely upon.
# However, VECZ should never crash or generate wrong code without the
# canonicalizations.
add_ca_default_unitcl_check(UnitCL-opt-disable-vecz COMPILER EXTENDED
  ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-opt-disable -cl-wfv=always")

add_ca_default_unitcl_check(UnitCL-opt-disable-vecz-boscc COMPILER EXTENDED
  ENVIRONMENT "CODEPLAY_VECZ_CHOICES=LinearizeBOSCC"
              "CA_EXTRA_COMPILE_OPTS=-cl-opt-disable -cl-wfv=always")

# Test VECZ Full Scalarization in combination with SLP and loop vectorization,
# because it stresses the scalarizer.
add_ca_default_unitcl_check(UnitCL-prevec-vecz-full-scalarization COMPILER
  ENVIRONMENT "CODEPLAY_VECZ_CHOICES=FullScalarization"
              "CA_EXTRA_COMPILE_OPTS=-cl-vec=all -cl-wfv=always"
  ARGS --vecz-check)

# Test auto-vectorization.
# At least one kernel is known not to vectorize in this config.
add_ca_default_unitcl_check(UnitCL-prevec-opt-disable COMPILER
  ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-vec=all -cl-opt-disable")

# Add this group to the global check target
add_dependencies(check-ca check-UnitCL-group)

if(NOT CA_CL_DISABLE_UNITCL_VECZ_CHECKS)
  add_ca_vecz_unitcl_check(UnitCL-vecz COMPILER
    ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-wfv=always"
    ARGS --vecz-check)

  # Test VECZ in combination with the loop vectorizer.
  add_ca_vecz_unitcl_check(UnitCL-prevec-vecz COMPILER EXTENDED
    ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-vec=all -cl-wfv=always"
    ARGS --vecz-check)

  # BOSCC specific debug info failures are quite specific, so only test that in
  # the extended test set.
  add_ca_vecz_unitcl_check(UnitCL-vecz-boscc-debug COMPILER EXTENDED
    ENVIRONMENT "CODEPLAY_VECZ_CHOICES=LinearizeBOSCC"
                "CA_EXTRA_COMPILE_OPTS=-cl-wfv=always -g"
    ARGS --vecz-check)

  add_ca_vecz_unitcl_check(UnitCL-vecz-boscc COMPILER
    ENVIRONMENT "CODEPLAY_VECZ_CHOICES=LinearizeBOSCC"
                "CA_EXTRA_COMPILE_OPTS=-cl-wfv=always"
    ARGS --vecz-check)

  # Test scalable vectorization on RVV.
  if(CA_CL_ENABLE_RVV_SCALABLE_VECZ_CHECK)
    # A RISCV-specific check which relies on the CA_RISCV_VF environment variable
    # to select a specific scalable vectorization factor. Perhaps if/when -cl-wfv
    # allows us to do the same we can make it a more general test.
    add_ca_vecz_unitcl_check(UnitCL-riscv-vecz-vf-nxv1 COMPILER
      ENVIRONMENT "CA_RISCV_VF=1,S")
  endif()

  # Test scalable vectorization in combination with vector predication
  if(CA_CL_ENABLE_RVV_SCALABLE_VP_VECZ_CHECK)
    # A RISCV-specific check which relies on the CA_RISCV_VF environment variable
    # to select a specific scalable vectorization factor in combination with
    # vector predication. Perhaps if/when -cl-wfv allows us to do the same we can
    # make it a more general test.
    add_ca_vecz_unitcl_check(UnitCL-riscv-vecz-vf-nxv1-vp COMPILER
      ENVIRONMENT "CA_RISCV_VF=1,S,VP")

    add_ca_vecz_unitcl_check(UnitCL-riscv-vecz-vf-nxv1-vvp COMPILER
      ENVIRONMENT "CA_RISCV_VF=1,S,VVP")
  endif()

  add_ca_vecz_unitcl_check(UnitCL-vecz-debug COMPILER
    ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-wfv=always -g"
    ARGS --vecz-check)

  # Test without wfv vectorization.
  add_ca_vecz_unitcl_check(UnitCL-vecz-never-opt-disable-debug COMPILER
    ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-opt-disable -g -cl-wfv=never")

  add_ca_vecz_unitcl_check(UnitCL-vecz-never COMPILER
    ENVIRONMENT "CA_EXTRA_COMPILE_OPTS=-cl-wfv=never")

  # Let's throw the kitchen sink at it, since that found at least one bug already..
  add_ca_vecz_unitcl_check(UnitCL-vecz-everything COMPILER
    ENVIRONMENT "CODEPLAY_VECZ_CHOICES=LinearizeBOSCC"
                "CA_EXTRA_COMPILE_OPTS=-cl-vec=all -cl-wfv=always"
    ARGS --vecz-check)

  # Add this group to the global check target
  add_dependencies(check-ca check-UnitCL-group-vecz)
endif()

if(OCL_EXTENSION_cl_intel_unified_shared_memory)
  add_ca_default_unitcl_check(UnitCL-USM ARGS --gtest_filter=*USM*)
endif()

if(CMAKE_CROSSCOMPILING)
  string(REPLACE ";" " " CTSEmulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
  # The subset of UnitCL tests which validate half precision math, this is not
  # added to the global check target.
  add_ca_check(UnitCL-half
    COMMAND ${PYTHON_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py -p gtest
      --qemu "${CTSEmulator}"
      --binary-path $<TARGET_FILE:UnitCL>
      --add-env-var GTEST_FILTER=*Half*
      --junit-result-file ${PROJECT_BINARY_DIR}/UnitCL-half.xml
      $<$<BOOL:${CA_CL_ENABLE_ICD_LOADER}>:--add-env-var>
      $<$<BOOL:${CA_CL_ENABLE_ICD_LOADER}>:OCL_ICD_FILENAMES=$<TARGET_FILE:CL>>
      $<$<BOOL:${CA_CL_ENABLE_INTERCEPT_LAYER}>:--add_env-var>
      $<$<BOOL:${CA_CL_ENABLE_INTERCEPT_LAYER}>:CLI_OpenCLFileName=$<TARGET_FILE:OpenCL>>
    CLEAN ${PROJECT_BINARY_DIR}/UnitCL-half.xml DEPENDS UnitCL
    NOEMULATE NOGLOBAL)
  # The subset of UnitCL tests which validate offline-only, this is not added
  # to the global check target.
  add_ca_check(UnitCL-offline
    COMMAND ${PYTHON_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py -p gtest
      --qemu "${CTSEmulator}"
      --binary-path $<TARGET_FILE:UnitCL>
      --add-env-var GTEST_FILTER=*Offline*Execution*
      --junit-result-file ${PROJECT_BINARY_DIR}/UnitCL-offline.xml
      $<$<BOOL:${CA_CL_ENABLE_ICD_LOADER}>:--add-env-var>
      $<$<BOOL:${CA_CL_ENABLE_ICD_LOADER}>:OCL_ICD_FILENAMES=$<TARGET_FILE:CL>>
      $<$<BOOL:${CA_CL_ENABLE_INTERCEPT_LAYER}>:--add_env-var>
      $<$<BOOL:${CA_CL_ENABLE_INTERCEPT_LAYER}>:CLI_OpenCLFileName=$<TARGET_FILE:OpenCL>>
    CLEAN ${PROJECT_BINARY_DIR}/UnitCL-offline.xml DEPENDS UnitCL
    NOEMULATE NOGLOBAL)
else()
  # Convenvenience to only run half tests to assist with a quick run
  add_ca_unitcl_check(UnitCL-half FILTER "*Half*")
endif()

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/test_include
  DESTINATION share
  COMPONENT UnitCL)

# We install the `${PROJECT_BINARY_DIR}/share/kernels` instead of
# `${CMAKE_CURRENT_SOURCE_DIR}/kernels` as other targets may have copied
# kernels to this folder.  The `install()` command supports calling scripts
# directly, but there does not appear to be a way of passing options to those
# scripts, so call the script via `execute_process()`.
# `${CMAKE_INSTALL_PREFIX}` is escaped in this command so that it gets replaced
# when the install command is called rather than at configure time. This makes
# it possible to call the install script manually with a different install
# prefix.
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -DINPUT_DIR=${PROJECT_BINARY_DIR}/share/kernels
    -DOUTPUT_DIR=\${CMAKE_INSTALL_PREFIX}/share/kernels
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallKernels.cmake)"
  DESTINATION share
  COMPONENT UnitCL)
