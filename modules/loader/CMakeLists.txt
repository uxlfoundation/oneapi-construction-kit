# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/loader.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loader
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/loader.dir/source)
endif()

add_ca_library(loader STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/elf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/mapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/relocation_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/relocations.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/elf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/relocations.cpp)

target_include_directories(loader PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(loader PUBLIC cargo)
