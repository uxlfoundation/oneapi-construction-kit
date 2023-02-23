# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_compiler_extension(riscv-clmul
EXTENSIONS riscv_clmul
INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/include/extension/riscv_clmul.h
${CMAKE_CURRENT_SOURCE_DIR}/source/riscv_clmul.cpp)

add_ca_force_header(PREFIX "riscv_clmul"
DEVICE_NAME "${CA_{{cookiecutter.target_name_capitals}}_DEVICE}"
PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/extension/builtins.h")
