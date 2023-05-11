# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "spirv-ll lit tests disabled: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

find_package(LLVMTool COMPONENTS opt)
if(NOT TARGET LLVM::opt)
  message(WARNING "spirv-ll lit tests disabled: opt not found")
  return()
endif()

find_package(LLVMTool COMPONENTS FileCheck)
if(NOT TARGET LLVM::FileCheck)
  message(WARNING "spirv-ll lit tests disabled: FileCheck not found")
  return()
endif()

find_package(SpirvTools COMPONENTS spirv-as)
if(NOT TARGET spirv::spirv-as)
  message(WARNING "spirv-ll some lit tests unsupported: spirv-as not found")
  return()
endif()

# Create the test/test inputs directories and set the relevent variables
set(INPUTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  SPIRV_LL=$<TARGET_FILE:spirv-ll-tool>
  OPT=$<TARGET_FILE:LLVM::opt>
  TRIPLE=${TRIPLE})

add_subdirectory(glsl)
add_subdirectory(spvasm)

add_ca_lit_testsuite(spirv-ll
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ca-common-lit spirv-ll-lit-glsl spirv-ll-lit-spvasm
          ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
