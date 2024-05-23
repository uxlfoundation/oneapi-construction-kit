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

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ExtractReqsOpts.cmake")
find_package(SpirvTools COMPONENTS spirv-as)
if(TARGET spirv::spirv-as)
  target_compile_definitions(UnitCL PRIVATE UNITCL_SPIRV_ENABLED)
elseif(CA_CL_ENABLE_OFFLINE_KERNEL_TESTS)
  message(WARNING "SPIRV-Tools not found, offline UnitCL SPIR-V tests will not run. "
                  "To run offline SPIR-V tests get SPIRV-Tools from here: "
                  "https://github.com/KhronosGroup/SPIRV-Tools")
endif()

if(CMAKE_CROSSCOMPILING)
  if(TARGET clc_native)
    set(CLC_EXECUTABLE $<TARGET_FILE:clc_native>)
    set(CLC_TARGET clc_native)
  elseif(CMAKE_CROSSCOMPILING_EMULATOR)
    set(CLC_EXECUTABLE ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:clc>)
    set(CLC_TARGET clc)
  endif()
else()
  set(CLC_EXECUTABLE $<TARGET_FILE:clc>)
  set(CLC_TARGET clc)
endif()

if(CA_EXTERNAL_LLVM_SPIRV)
  execute_process(COMMAND ${CA_EXTERNAL_LLVM_SPIRV} --version
    OUTPUT_VARIABLE llvmSpirvVersion)
  string(REGEX MATCH "LLVM version ([0-9]+).([0-9]+).([0-9]+)" MVAR "${llvmSpirvVersion}")
  if(NOT CMAKE_MATCH_1 EQUAL LLVM_VERSION_MAJOR)
    message(WARNING "CA_EXTERNAL_LLVM_SPIRV must have same version as ${LLVM_VERSION_MAJOR}")
  endif()
  add_ca_executable_import(llvm-spirv "${CA_EXTERNAL_LLVM_SPIRV}")

  # spirv-dis is required for generating the spvasm files that we check in to
  # the repository. This tool is only used with the `regenerate-spir` and
  # `regenerate-spirv` targets when creating the spir and spirv files
  # associated with each kernel.
  find_package(SpirvTools COMPONENTS spirv-dis)
  if(NOT TARGET spirv::spirv-dis)
    message(WARNING "spirv-dis not found: spvasm will not be generated")
  endif()
endif()

# The kernels are stored in a separate file to help keep this file easier to
# read. We include them at this point. These are primarily `KERNEL_FILES`.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(kernel_source_list)

# This list is used to track all the kernel files so that we can copy them to
# the destination in one go.
set(ALL_KERNEL_FILES ${KERNEL_FILES})

# Check the list of project names and add the relevant kernel files to the
# KERNEL_FILES variable. These will be blank normally but will add the kernels
# defined by other projects if they are present.
foreach(NAME ${MUX_TARGET_LIBRARIES})
  # Add the target specific kernels to the `ALL_KERNEL_FILES` list for copying
  # later. Doing copying in one go simplifies things
  list(APPEND ALL_KERNEL_FILES ${${NAME}_UNITCL_KERNEL_FILES})
  # Add the universal kernels to the target list
  list(APPEND ${NAME}_UNITCL_KERNEL_FILES ${KERNEL_FILES})
  add_spirv(${NAME}_UNITCL_KERNEL_FILES ${NAME}_UNITCL_KERNEL_FILES)
endforeach()

# For the given list of kernels, check if they have spir or spirv variants and
# report back a list containing all of them.
add_spirv(ALL_KERNEL_FILES ALL_KERNEL_FILES)

set(TARGET_KERNELS "")

# For each of the listed kernel files we need to copy them across to the
# project directory. However, as an additional step, we need to assemble the
# spvasm files. If one of these are present we run the `spirv-as` tool to get
# the binary form and move that instead.
foreach(KERNEL ${ALL_KERNEL_FILES})
  get_filename_component(KERNEL_NAME ${KERNEL} NAME_WLE)
  get_filename_component(KERNEL_EXT ${KERNEL} LAST_EXT)

  # SPIR-V files are a special case, since they need to be assembled. The
  # assembled files have a different extension. If spirv-as isn't available
  # then the SPIR-V files are skipped entirely.
  if(${KERNEL_EXT} MATCHES ".spvasm(32|64)")
    if(NOT TARGET spirv::spirv-as)
      continue()
    endif()
    string(REGEX REPLACE "asm" "" output_ext ${KERNEL_EXT})
  else()
    set(output_ext "${KERNEL_EXT}")
  endif()

  set(OUTPUT_FILE
    "${PROJECT_BINARY_DIR}/share/kernels/${KERNEL_NAME}${output_ext}")

  # Create a command that copies the kernel to the build directory, assembling
  # it if needed. This code takes a shortcut by making non-spvasm kernels also
  # depend on spirv-as when it's present. Since spirv-as is unlikely to change
  # often, this shouldn't increase build times in the general case.
  if(TARGET spirv::spirv-as)
    add_custom_command(
      OUTPUT ${OUTPUT_FILE}
      COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${KERNEL}
        -DOUTPUT_DIR="${PROJECT_BINARY_DIR}/share/kernels"
        -DSPIRV_AS_PATH=$<TARGET_FILE:spirv::spirv-as>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CopyOrAssembleKernel.cmake
      DEPENDS ${KERNEL} spirv::spirv-as
        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CopyOrAssembleKernel.cmake
      WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}
        COMMENT "Copying or assembling ${KERNEL_NAME}${output_ext}")
  else()
    add_custom_command(
      OUTPUT ${OUTPUT_FILE}
      COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${KERNEL}
        -DOUTPUT_DIR="${PROJECT_BINARY_DIR}/share/kernels"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CopyOrAssembleKernel.cmake
      DEPENDS ${KERNEL}
        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CopyOrAssembleKernel.cmake
      WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}
        COMMENT "Copying or assembling ${KERNEL_NAME}${output_ext}")
  endif()

  list(APPEND TARGET_KERNELS ${OUTPUT_FILE})
endforeach()

add_custom_target(copy-kernels DEPENDS ${TARGET_KERNELS})

# Early exit when offline tests aren't required
if(NOT CA_CL_ENABLE_OFFLINE_KERNEL_TESTS)
  return()
endif()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/share/kernels_offline)
set(outputs)
set(ir_spir_outputs)
set(ir_spirv_outputs)

foreach(target ${MUX_TARGET_LIBRARIES})
  # Skip building any kernels for the cross target, the cross target
  # capabilities is a superset of all enabled core targets which can result
  # in attempts to compile a kernel multiple times with different
  # configurations, additionally the devices are compiler-only duplicates of
  # enabled core targets which is another layer of duplication. Avoid these
  # duplicates by not compiling kernels for cross devices.
  if(${target} STREQUAL cross)
    continue()
  endif()

  foreach(device_name ${${target}_DEVICE_NAMES})
    set(target_kernels_dir
      ${PROJECT_BINARY_DIR}/share/kernels_offline/${device_name})
    file(MAKE_DIRECTORY ${target_kernels_dir})
    file(MAKE_DIRECTORY ${target_kernels_dir}/spir/)
    file(MAKE_DIRECTORY ${target_kernels_dir}/spirv/)

    # List of type names for generating clGetKernelArgInfo tests
    set(cl_type_names char uchar short ushort int uint long ulong float)

    if(${target}_CAPABILITIES MATCHES "fp64")
      list(APPEND cl_type_names double)
    endif()

    if(${target}_CAPABILITIES MATCHES "fp16")
      list(APPEND cl_type_names half)
    endif()

    set(cl_type_vector_sizes 1 2 3 4 8 16)

    foreach(type ${cl_type_names})
      foreach(vector ${cl_type_vector_sizes})
        if(vector STREQUAL 1)
          set(fulltype "${type}")
        else()
          set(fulltype "${type}${vector}")
        endif()
        set(outfile
          ${CMAKE_CURRENT_BINARY_DIR}/offline.01_type_${fulltype}.cl)
        configure_file(
          ${CMAKE_CURRENT_SOURCE_DIR}/offline_type.cl.in ${outfile})
        list(APPEND ${target}_UNITCL_KERNEL_FILES ${outfile})
      endforeach()
    endforeach()

    # Create all commands to build kernels
    foreach(kernel ${${target}_UNITCL_KERNEL_FILES})
      # This loop processes three types of kernels: OpenCL-C (.cl) and SPIR-V
      # assembly (.spvasm32, .spvasm64). A target is set up to compile each
      # type of kernel down to a .bin executable. Targets are also set up to
      # compile .cl files to SPIR-V (provided the required dependencies are
      # present).

      # Split kernel name up: `d/a.b.c` => `d`, `a.b`, `.c`
      get_filename_component(kernel_dir ${kernel} DIRECTORY)
      get_filename_component(kernel_name ${kernel} NAME_WLE)
      get_filename_component(kernel_ext ${kernel} LAST_EXT)

      # Sanity-check kernel extension
      set(valid_extensions ".cl" ".bc32" ".bc64" ".spvasm32" ".spvasm64")
      list(FIND valid_extensions "${kernel_ext}" ext_idx)
      if(ext_idx EQUAL -1)
        message(FATAL_ERROR "'${kernel}' has an unrecognized extension")
      endif()

      # Skip kernels that don't match bit width. Note that this assumes that
      # `32bit` and `64bit` are mutually exclusive. While builtins are
      # constrained to be either 32-bit (exclusive) or 64-bit at the moment,
      # it's conceivable that one set of builtins could support both widths in
      # the future. This creates an ambiguity, since it's not clear whether
      # `foo.bin` is compiled from `foo.bc32` or `foo.bc64`.
      if((${kernel_ext} MATCHES "32"
            AND NOT ${target}_CAPABILITIES MATCHES "32bit") OR
          (${kernel_ext} MATCHES "64"
            AND NOT ${target}_CAPABILITIES MATCHES "64bit"))
        continue()
      endif()

      # Skip kernels that don't match capabilities.
      if(((${kernel_name} MATCHES "\.fp64" OR ${kernel_name} MATCHES "_double$")
            AND NOT ${target}_CAPABILITIES MATCHES "fp64") OR
         (${kernel_name} MATCHES "\.fp16"
	    AND NOT ${target}_CAPABILITIES MATCHES "fp16") OR
         ((${kernel_name} MATCHES "atom.*long" OR ${kernel_name} MATCHES "atom.*double")
            AND NOT ${target}_CAPABILITIES MATCHES "atomic64"))
        continue()
      endif()

      set(kernel_input_name ${kernel})
      set(kernel_output_name "${kernel_name}.bin")

      # When passing IR and cl kernels to `clc` we generate a `.bin` file with
      # the same name. To avoid conflicts we place all the SPIR-V binaries into
      # the spirv folder.
      if(${kernel_ext} MATCHES "\.spvasm(32|64)")
        set(output ${target_kernels_dir}/spirv/${kernel_output_name})
        # If we're compiling an offline SPIR-V kernel, we should depend on the
        # assembled binary instead
        string(REGEX REPLACE "asm" "" ext ${kernel_ext})
        set(kernel_input_name
          "${PROJECT_BINARY_DIR}/share/kernels/${kernel_name}${ext}")
      elseif(${kernel_ext} MATCHES "\.bc(32|64)")
        set(output ${target_kernels_dir}/spir/${kernel_output_name})
      else()
        set(output ${target_kernels_dir}/${kernel_output_name})
      endif()

      # If a .cl file exists that corresponds to the current kernel file,
      # then the .cl file may contain requirements and compilation options,
      # and is therefore a dependency. When ${kernel} is a .cl file, then
      # `${kernel} STREQUAL ${kernel_cl_file}`.
      set(kernel_cl_file "${kernel_dir}/${kernel_name}.cl")
      if(NOT EXISTS ${kernel_cl_file})
        set(kernel_cl_file "")
      endif()

      # Custom command that calls clc to compile the kernel to a .bin
      add_custom_command(OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND}
          -DINPUT_FILE=${kernel_input_name}
          -DINPUT_CL_FILE=${kernel_cl_file}
          -DOUTPUT_FILE=${output}
          -DCLC_EXECUTABLE='${CLC_EXECUTABLE}'
          -DDEVICE_NAME=${device_name}
          -DTARGET_CAPABILITIES="${${target}_CAPABILITIES}"
          -DIMAGE_SUPPORT=${CA_ENABLE_HOST_IMAGE_SUPPORT}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CompileKernelToBin.cmake
        DEPENDS ${CLC_TARGET} ${kernel_input_name} ${kernel_cl_file}
          ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CompileKernelToBin.cmake
          ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ExtractReqsOpts.cmake
        WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}
        COMMENT "Building CL binary ${kernel_name}.bin")

      list(APPEND outputs ${output})

      # Set up commands to generate SPIR-V files from .cl files.
      # These files are built in the source (not build) directory, and only as
      # part of the regenerate-spir and regenerate-spirv targets.
      if(NOT "${kernel_ext}" STREQUAL ".cl")
        # No more work to be done for .bc{32|64} and .spvasm{32|64} files
        continue()
      endif()

      # Compile kernels to .spvasm{32|64}
      if(CA_EXTERNAL_LLVM_SPIRV)
        # SPIR-V output file names
        set(spvasm32_kernel_name "${kernel_name}.spvasm32")
        set(spvasm64_kernel_name "${kernel_name}.spvasm64")

        # Some of the kernels may be for specific target platforms and might
        # live in different directories. We prepend the directory so we now
        # have the full output path for the file.
        set(spvasm32_output ${kernel_dir}/${spvasm32_kernel_name})
        set(spvasm64_output ${kernel_dir}/${spvasm64_kernel_name})

        add_custom_command(
          OUTPUT ${spvasm32_output} ${spvasm64_output}
          COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${kernel}
            -DCLANG_PATH=$<TARGET_FILE:clang>
            -DLLVM_SPIRV_PATH=$<TARGET_FILE:llvm-spirv>
            -DSPIRV_DIS_PATH=$<TARGET_FILE:spirv::spirv-dis>
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P
            ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CompileKernelToSPIRVAsm.cmake
          DEPENDS llvm-spirv ${kernel} spirv::spirv-dis clang
            ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CompileKernelToSPIRVAsm.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ExtractReqsOpts.cmake
          WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}
          COMMENT "Building SPIR-V ${kernel_name}.spvasm{32|64}")
        list(APPEND ir_spirv_outputs ${spvasm32_output} ${spvasm64_output})
      endif()
    endforeach()
  endforeach()
endforeach()

# To regenerate the SPIR-V files we need to have llvm-spirv
if (CA_EXTERNAL_LLVM_SPIRV)
  add_custom_target(regenerate-spirv DEPENDS ${ir_spirv_outputs})
endif()

add_custom_target(UnitCL-offline-execution-kernels DEPENDS ${outputs})
add_dependencies(UnitCL UnitCL-offline-execution-kernels)

# Install offline-compiled kernels using a script that only installs files
# that aren't stubs. This way dependency tracking still works in the build
# directory. The `install()` command supports calling scripts directly, but
# there does not appear to be a way of passing options to those scripts, so
# call the script via `execute_process()`.
# `${CMAKE_INSTALL_PREFIX}` is escaped in this command so that it gets replaced
# when the install command is called rather than at configure time. This makes
# it possible to call the install script manually with a different install
# prefix.
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -DINPUT_DIR=${PROJECT_BINARY_DIR}/share/kernels_offline
    -DOUTPUT_DIR=\${CMAKE_INSTALL_PREFIX}/share/kernels_offline
    -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/InstallKernels.cmake)"
  DESTINATION share
  COMPONENT UnitCL)
