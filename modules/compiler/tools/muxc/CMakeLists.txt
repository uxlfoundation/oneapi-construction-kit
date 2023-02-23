# Copyright (C) Codeplay Software Limited. All Rights Reserved.

llvm_map_components_to_libnames(llvm_libs all ${LLVM_TARGETS_TO_BUILD})

add_ca_executable(muxc
  ${CMAKE_CURRENT_SOURCE_DIR}/muxc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/muxc.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)

target_compile_options(muxc PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)
target_compile_definitions(muxc PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  muxc_VERSION="${PROJECT_VERSION}"
  muxc_LLVM_VERSION="${LLVM_PACKAGE_VERSION}"
  muxc_CL_STD_30=$<BOOL:${muxc_CL_STD_30}>)
target_include_directories(muxc SYSTEM PUBLIC ${LLVM_INCLUDE_DIR})
target_include_directories(muxc PUBLIC
  ${PROJECT_SOURCE_DIR}/modules/compiler/source/base/include
  ${PROJECT_SOURCE_DIR}/source/cl/source/compiler/include
  ${PROJECT_SOURCE_DIR}/modules/multi_llvm/include)
target_link_libraries(muxc PRIVATE
  compiler-static mux builtins cargo LLVMBitWriter LLVMIRReader)
target_resources(muxc NAMESPACES ${BUILTINS_NAMESPACES})

install(TARGETS muxc RUNTIME DESTINATION bin COMPONENT muxc)
