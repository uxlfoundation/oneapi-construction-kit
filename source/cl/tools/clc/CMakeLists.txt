# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(clc
  ${CMAKE_CURRENT_SOURCE_DIR}/clc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/clc.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)
target_compile_options(clc PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)
target_compile_definitions(clc PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  CLC_VERSION="${PROJECT_VERSION}"
  CLC_LLVM_VERSION="${LLVM_PACKAGE_VERSION}")
target_include_directories(clc PUBLIC
  ${PROJECT_SOURCE_DIR}/source/cl/source/compiler/include)
target_link_libraries(clc PRIVATE CL-binary compiler-static builtins cargo)
target_resources(clc NAMESPACES ${BUILTINS_NAMESPACES})

install(TARGETS clc RUNTIME DESTINATION bin COMPONENT clc)
