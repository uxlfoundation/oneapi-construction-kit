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

include(Bin2H)

set(RISCVLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/external/memmove.c)

# bakery.cpp accesses generated headers so copy to binary dir to compile later
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/source/bakery.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/source/bakery.cpp
  @ONLY)

# find path to RISC-V clang
set(LLVM_CLANG "${CA_LLVM_INSTALL_DIR}/bin/clang${CA_HOST_EXECUTABLE_SUFFIX}")
if(NOT EXISTS ${LLVM_CLANG})
  message(FATAL_ERROR "${LLVM_CLANG} does not exist.")
endif()
# find path to RISC-V LLVM-AR
set(LLVM_AR "${CA_LLVM_INSTALL_DIR}/bin/llvm-ar${CA_HOST_EXECUTABLE_SUFFIX}")
if(NOT EXISTS ${LLVM_AR})
  message(FATAL_ERROR "${LLVM_AR} does not exist.")
endif()

# Create custom commands to allow building risc-v object
# files for for 32 or 64 bit
function(riscv_custom_lib_command_gen WIDTH)
  # get compile flags based on target bitwidth
  # The front-end ABI defaults to the ilp32/lp64, and ilp32d/lp64d when
  # hardware support for extension D. The backend ABI defaults to ilp32/lp64.
  # Since we assume double support, explicitly use the double ABIs.
  if (WIDTH STREQUAL "64")
    set(TARGET_FLAGS "-c" "-O3" "-march=rv64g" "-mabi=lp64d" "--target=riscv64-elf")
  else()
    set(TARGET_FLAGS "-c" "-O3" "-march=rv32g" "-mabi=ilp32d" "--target=riscv32-elf")
  endif()

  set(RISCV_OBJECTS "")
  foreach(RISCV_SOURCE ${RISCVLIB_SOURCES})
    # Create object name from c
    get_filename_component(RISCV_BASENAME ${RISCV_SOURCE} NAME)
    string(REGEX REPLACE "^(.*)\.c" "\\1_${WIDTH}\.o" SUB_SOURCE_OBJ "${RISCV_BASENAME}")
    # compile source to build runtime library object file
    set(TARGET_ELF
      "${CMAKE_CURRENT_BINARY_DIR}/${SUB_SOURCE_OBJ}")
    add_custom_command(
      OUTPUT "${TARGET_ELF}"
      COMMAND ${LLVM_CLANG} ${TARGET_FLAGS} --output=${TARGET_ELF} ${RISCV_SOURCE}
      DEPENDS ${RISCVLIB_SOURCE}
      VERBATIM)
       
    list(APPEND RISCV_OBJECTS ${TARGET_ELF})
  endforeach()

  # create static library from object file
  set(TARGET_A ${CMAKE_CURRENT_BINARY_DIR}/riscvlib${WIDTH}.a)
  add_custom_command(
    OUTPUT "${TARGET_A}"
    COMMAND ${LLVM_AR} rc ${TARGET_A} ${RISCV_OBJECTS}
    DEPENDS ${RISCV_OBJECTS}
    VERBATIM)
endfunction()

# function to create a target for a bin2h file
# we use this to encode the library
function(add_bin2h_target target input output)
  add_bin2h_command(${target} ${input} ${output})
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_target(${target}
    DEPENDS ${input}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating H file ${relOut}")
endfunction()

# Generate the custom commands to build for 64 and 32 bit libs
riscv_custom_lib_command_gen("64")  # 64-bit
riscv_custom_lib_command_gen("32")  # 32-bit

# Add bin2h targets for 32 and 64 bit libs
add_bin2h_target(riscvgenheader32
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib32.a
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib32.h)
add_bin2h_target(riscvgenheader64
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib64.a
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib64.h)

# Add libraries based off bakery file and generated headers
add_ca_library(riscvrt STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib64.h
  ${CMAKE_CURRENT_BINARY_DIR}/riscvlib32.h
  ${CMAKE_CURRENT_BINARY_DIR}/source/bakery.cpp)

# Add extra dependencies to any clang-tidy checks, so that the required headers
# are auto-generated.
if(COMMAND add_ca_tidy)
  if(TARGET tidy-riscvrt)
    set_property(TARGET tidy-riscvrt APPEND PROPERTY SOURCES
                ${CMAKE_CURRENT_BINARY_DIR}/riscvlib32.h
                ${CMAKE_CURRENT_BINARY_DIR}/riscvlib64.h)
    add_dependencies(tidy-riscvrt riscvgenheader32 riscvgenheader64)
  endif()
endif()
