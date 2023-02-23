# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_library(md_api STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/metadata.cpp)
target_link_libraries(md_api PUBLIC cargo)
target_include_directories(md_api
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata/detail)

add_ca_library(md_handler STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/handler/generic_metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/handler/vectorize_info_metadata.cpp)
target_link_libraries(md_handler PUBLIC cargo md_api)
target_include_directories(md_handler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CA_ENABLE_TESTS)
  # UnitMD
  add_ca_executable(UnitMD
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/allocators.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_map.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_value.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_stack.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_ctx.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/stack_serializer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/api.cpp)
  
  target_include_directories(UnitMD PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata/detail)

  target_link_libraries(UnitMD PRIVATE ca_gtest_main md_api)
  
  add_ca_check(UnitMD GTEST
      COMMAND UnitMD --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitMD.xml
      CLEAN ${PROJECT_BINARY_DIR}/UnitMD.xml
      DEPENDS UnitMD)
endif()
