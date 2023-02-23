# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If coverage is enabled we add the modules so that the XML file can be
# generated automatically.
if(CA_ENABLE_COVERAGE)
  add_coverage_modules(
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-loader.dir/source

    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-loader.dir/source
  )
endif()

set(COMPILER_HEADERS
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/context.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/kernel.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/limits.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/module.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/target.h)

set(COMPILER_LOADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/loader.cpp)

add_ca_library(compiler-loader STATIC
  ${COMPILER_HEADERS} ${COMPILER_LOADER_SOURCES})
target_include_directories(compiler-loader PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/modules/compiler/include)
target_include_directories(compiler-loader SYSTEM PUBLIC
  ${PROJECT_SOURCE_DIR}/modules/spirv-ll/external/SPIRV-Headers/include)
target_link_libraries(compiler-loader PUBLIC mux cargo)
target_compile_definitions(compiler-loader PRIVATE
  $<$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>:
    CA_COMPILER_ENABLE_DYNAMIC_LOADER
  >)

# If the compiler is enabled.
if (CA_RUNTIME_COMPILER_ENABLED)
  target_link_libraries(compiler-loader PUBLIC
    # Link the dynamic loader on Linux if dynamic loading is enabled.
    $<$<AND:$<PLATFORM_ID:Linux>,$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>>:dl>
    # Link the static version of the compiler if dynamic loading is disabled.
    $<$<NOT:$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>>:compiler-static>)
endif()

# We need to directly link against the builtins to ensure header paths are
# satisfied for an offline only build or for a build using the dynamic loader.
target_link_libraries(compiler-loader PUBLIC builtins)
