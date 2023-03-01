# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Create an internally cached variable to store a list of include directories
# for modules which do not export a target, such as header only libraries.
# These modules must append to this string so that the include directories can
# be propagated throughout the project.
set(MODULES_INCLUDE_DIRS # No default value
  CACHE INTERNAL "List of module include directories.")

# Create an internally cached variable to store all module libraries. Modules
# must append to this list so that the libraries can be propagated throughout
# the project.
set(MODULES_LIBRARIES # No default value
  CACHE INTERNAL "List of module libraries.")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cargo)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/loader)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tracer)
if(CA_RUNTIME_COMPILER_ENABLED)
  # multi_llvm must be before all other modules that want to use it.
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/multi_llvm)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/debug)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)
# vecz must be before mux for it to use vecz.
if(CA_RUNTIME_COMPILER_ENABLED)
  add_ca_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vecz)
endif()

# Fetch the official SPIR-V headers. These are required by some up our public
# compiler headers, which are also included by mux, so this must take place
# before including both mux and compiler modules.
include(FetchContent)
FetchContent_Declare(
  SPIRV-Headers
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
  GIT_TAG        sdk-1.3.239.0
)
FetchContent_MakeAvailable(SPIRV-Headers)

if(NOT spirv-headers_POPULATED)
  message(FATAL_ERROR "No SPIRV-Headers found.")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mux)
# compiler/builtins requires capabilities from ComputeMux targets.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/compiler)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/metadata)
if(CA_ENABLE_TESTS)
  # mux tests depend on builtins.
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mux/test)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/kts)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lit)

foreach(LIBRARY ${MODULES_LIBRARIES})
  target_include_directories(${LIBRARY} PUBLIC ${MODULES_INCLUDE_DIRS})
endforeach()
