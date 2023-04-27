# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# ComputeAorta supports inclusion of the OpenCL-ICD-Loader and the
# OpenCL-Intercept-Layer in the build, however both projects create different
# OpenCL CMake targets which is problematic as the OpenCL-Intercept-Layer
# requires the OpenCL-ICD-Loader to operate. As such, there are two paths for
# building the OpenCL-ICD-Loader.
#
# 1.  When CA_CL_ENABLE_ICD_LOADER is enabled and the
#     CA_CL_ENABLE_INTERCEPT_LAYER is disabled, the OpenCL-ICD-Loader is
#     included in the ComputeAorta build tree using add_subdirectory().
# 2.  When both CA_CL_ENABLE_ICD_LOADER and CA_CL_ENABLE_INTERCEPT_LAYER are
#     enabled, the OpenCL-Intercept-Layer is included in the ComputeAorta build
#     tree using add_subdirectory() and the OpenCL-ICD-Loader is built using
#     ExternalProject_Add().
#
# In both scenarios, any application which needs to will link against the
# OpenCL target provided by either the OpenCL-ICD-Loader or the
# OpenCL-Intercept-Layer.

if(CA_CL_ENABLE_ICD_LOADER)
  if(CA_CL_EXTERNAL_ICD_LOADER_SOURCE_DIR)
    set(opencl_icd_loader_SOURCE_DIR ${CA_CL_EXTERNAL_ICD_LOADER_SOURCE_DIR})
  else()
    include(FetchContent)

    FetchContent_Declare(
      OpenCL_ICD_LOADER
      GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-ICD-Loader.git
      GIT_TAG        v2021.04.29
    )

    # Fetch ICD from github. This also sets opencl_icd_loader_SOURCE_DIR
    FetchContent_Populate(OpenCL_ICD_LOADER)  
  endif()
  if(NOT EXISTS ${opencl_icd_loader_SOURCE_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "${opencl_icd_loader_SOURCE_DIR} does not exist or does "
      "not contain a CMakeLists.txt, to resolve:" "
    * run: git submodule update --init
    * set CA_CL_EXTERNAL_ICD_LOADER_SOURCE_DIR correctly")
  endif()

  if(CA_CL_ENABLE_INTERCEPT_LAYER)
    # 2. Build the OpenCL-ICD-Loader as an ExternalProject, the OpenCL target
    # is provided by the OpenCL-Intercept-Layer.

    if(CA_CL_EXTERNAL_INTERCEPT_LAYER_SOURCE_DIR)
      set(openclinterceptlayer_SOURCE_DIR
        ${CA_CL_EXTERNAL_INTERCEPT_LAYER_SOURCE_DIR})
    else()
      # First of all use FetchContent to pull it from github
      include(FetchContent)

      FetchContent_Declare(
        OpenCLInterceptLayer
        GIT_REPOSITORY https://github.com/intel/opencl-intercept-layer.git
        GIT_TAG        v3.0.3
      )

      # Populates it and sets openclinterceptlayer_SOURCE_DIR
      FetchContent_Populate(OpenCLInterceptLayer)
    endif()
    if(NOT EXISTS ${openclinterceptlayer_SOURCE_DIR}/CMakeLists.txt)
      message(FATAL_ERROR "${openclinterceptlayer_SOURCE_DIR} does not exist or "
        "does not contain a CMakeLists.txt, to resolve:" "
    * run: git submodule update --init
    * set CA_CL_EXTERNAL_INTERCEPT_LAYER_SOURCE_DIR correctly
    * or clone https://github.com/intel/openCL-intercept-layer at the path:
      source/cl/external/OpenCL-Intercept-Layer")
    endif()

    # Create configure, build, and install steps for OpenCL-ICD-Loader which
    # will run during the ComputeAorta build. Introduces the OpenCL-ICD-Loader
    # target to explicitly build the external project.
    include(ExternalProject)
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
      set(OpenCLICDLoaderExtraCMakeArgs
        -DOPENCL_ICD_LOADER_SECURE_GETENV=OFF
        -DOPENCL_ICD_LOADER_REQUIRE_WDK=OFF)
    endif()

    ExternalProject_Add(OpenCL-ICD-Loader PREFIX OpenCL-ICD-Loader
      SOURCE_DIR ${opencl_icd_loader_SOURCE_DIR}
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/build
      INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/install
      CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DOPENCL_ICD_LOADER_HEADERS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-Headers
      ${OpenCLICDLoaderExtraCMakeArgs})

    # Explicitly install the OpenCL-ICD-Loader's install directory in a
    # subdirectory of the main install directory so that it does not overwrite
    # the OpenCL-Intercept-Layer library. The trailing / is required.
    install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader/install/
      DESTINATION lib/OpenCL COMPONENT OpenCL-CTS)

    # The OpenCL-Intercept-Layer requires C++ language extension with GCC.
    set(CMAKE_CXX_EXTENSIONS ON)
    add_subdirectory(${openclinterceptlayer_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-Intercept-Layer)
    set(CMAKE_CXX_EXTENSIONS OFF)
    add_dependencies(OpenCL OpenCL-ICD-Loader)
    set_ca_target_output_directory(OpenCL)

  else()
    # 1. Don't build the OpenCL-Intercept-Layer, the OpenCL target is provided
    # by the OpenCL-ICD-Loader.

    # Enable C language extensions which we otherwise disable.
    set(CMAKE_C_EXTENSIONS ON)
    # Set the OpenCL include directories.
    set(OPENCL_ICD_LOADER_HEADERS_DIR
      ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-Headers
      CACHE PATH "Path to OpenCL Headers" FORCE)
    if(CA_PLATFORM_WINDOWS)
      # Disable integrity level checks so environment variables work on Windows.
      set(OPENCL_ICD_LOADER_SECURE_GETENV OFF CACHE BOOL
        "Enable getenv integrity level checks." FORCE)
      # Disable requiring WDK for Windows builds.
      set(OPENCL_ICD_LOADER_REQUIRE_WDK OFF CACHE BOOL
        "Build with D3DKMT support, which requires the Windows WDK." FORCE)
    endif()

    add_subdirectory(${opencl_icd_loader_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/OpenCL-ICD-Loader)
    set_ca_target_output_directory(OpenCL)

  endif()

  # Many of our test programs still use older, deprecated API entry points even
  # when compiled as OpenCL 3.0 programs, so disable deprecation warnings.
  if(CA_PLATFORM_WINDOWS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_target_properties(OpenCL PROPERTIES INTERFACE_COMPILE_OPTIONS
      /wd4996)
  else()
    set_target_properties(OpenCL PROPERTIES INTERFACE_COMPILE_OPTIONS
      -Wno-deprecated-declarations)
  endif()

elseif(CA_CL_ENABLE_INTERCEPT_LAYER)
  message(FATAL_ERROR "\
CA_CL_ENABLE_INTERCEPT_LAYER is ON but CA_CL_ENABLE_ICD_LOADER is OFF, to use
the OpenCL-Intercept-Layer the OpenCL-ICD-Loader is required, set
CA_CL_ENABLE_ICD_LOADER to ON or set CA_CL_ENABLE_INTERCEPT_LAYER to OFF.
")
endif()
