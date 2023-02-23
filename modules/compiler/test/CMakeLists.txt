# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(UnitCompiler
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)

target_include_directories(UnitCompiler PRIVATE
  ${PROJECT_SOURCE_DIR}/modules/mux/include
  ${PROJECT_SOURCE_DIR}/modules/compiler/include)

target_link_libraries(UnitCompiler PRIVATE cargo
  compiler-static mux ca_gtest_main)

target_resources(UnitCompiler NAMESPACES ${BUILTINS_NAMESPACES})

add_ca_check(UnitCompiler GTEST
  COMMAND UnitCompiler --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitCompiler.xml
  CLEAN ${PROJECT_BINARY_DIR}/UnitCompiler.xml
  DEPENDS UnitCompiler)

add_subdirectory(passes)

install(TARGETS UnitCompiler RUNTIME DESTINATION bin COMPONENT compiler)
