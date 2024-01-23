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

cmake_minimum_required(VERSION 3.26)
project(OCKTests)

set(COMPILE_FLAGS "-fsycl -Wall ${WIN_FLAG}")
set(LINK_FLAGS "-fsycl")

function(run_test_with_dpcpp)
# To compile in a single command:
#    clang++ -fsycl <file>.cpp -o <file>.fpga_emu
# CMake executes:
#    [compile] clang++ -fsycl -o <file>.cpp.o -c <file>.cpp
#    [link]    clang++ -fsycl <file>.cpp.o -o <file>.fpga_emu
  message("C++ Compiler: ${CMAKE_CXX_COMPILER}")
  add_executable(${TARGET} ${SOURCES})
  set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endfunction(run_test_with_dpcpp)

set(OPENCL_FLAGS "-lOpenCL")
function(run_cl_test_with_dpcpp)
# To compile in a single command:
#    clang -lOpenCL <file>.c -o <file>
# CMake executes:
#    [compile] clang -o <file>.c.o -c <file>.c
  add_executable(${TARGET} ${SOURCES})
  target_include_directories(${TARGET} PRIVATE ${OpenCL_INCLUDE_DIR})
  target_link_libraries(${TARGET} PRIVATE ${OpenCL_LIBRARY})
  set_target_properties(${TARGET} PROPERTIES OPENCL_FLAGS "${OPENCL_FLAGS}")
endfunction(run_cl_test_with_dpcpp)

# Set the output directory for the binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(TARGET simple-vector-add)
set(SOURCES simple-vector-add.cpp)

run_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})

set(TARGET vector_addition-load-store)
set(SOURCES vector_addition-load-store.cpp)

run_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})

set(TARGET vector_addition-predicated)
set(SOURCES vector_addition-predicated.cpp)

run_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})

set(TARGET vector_addition-masked-store)
set(SOURCES vector_addition-masked-store.cpp)

run_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})

set(TARGET vector_addition-tiled-load-store)
set(SOURCES vector_addition-tiled-load-store.cpp)

run_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})

set(TARGET clVectorAddition)
set(SOURCES clVectorAddition/main.c)

run_cl_test_with_dpcpp(TARGET ${TARGET} SOURCES ${SOURCES})


# enable testing functionality
enable_testing()

add_test(
  NAME simple_vector_add
  COMMAND ${TARGET}
)

add_test(
  NAME vector_addition-load-store
  COMMAND ${TARGET}
)

add_test(
  NAME vector_addition-predicated
  COMMAND ${TARGET}
)

add_test(
  NAME vector_addition-masked-store
  COMMAND ${TARGET}
)

add_test(
  NAME vector_addition-tiled-load-store
  COMMAND ${TARGET}
)

add_test(
  NAME syclAvgPooling
  COMMAND ${TARGET}
)

add_test(
  NAME clVectorAddition
  COMMAND ${TARGET}
)