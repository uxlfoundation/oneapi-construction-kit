# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32 AND NOT CA_BUILD_32_BITS)
  set(CMAKE_CL_64 true)
endif()

if(CA_CL_ENABLE_ICD_LOADER)
  set(CLConform_HEADERS ${CL_INCLUDE_DIR})
  set(CLConform_LIBRARIES OpenCL)
else()
  set(CLConform_LIBRARIES CL)
endif()
set(BENCHCL_LIBRARY CL)
set(CLConf_OUT_DIR bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UnitCL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiDevice)
if(CA_RUNTIME_COMPILER_ENABLED)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/GeneratedTests)
endif()
if(TARGET ca-benchmark)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/BenchCL)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FuzzCL)

# A release does not include the OpenCL 1.2 CTS
if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin AND EXISTS
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-CTS/test_conformance/CMakeLists.txt)
  # Backup flags to be restored later.
  set(BACKUP_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
  set(BACKUP_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

  if(ANDROID AND ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    # Android NDK r14 clang toolchain adds this flag which causes the CTS
    # build to always fail. Disable it so that we can build the CTS.
    string(REPLACE "-Werror=format-security" ""
      CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR
      CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    # The CTS uses C++11 code in some places, e.g. unique_ptr, but violates
    # C++11 narrowing rules according to Clang, so tell Clang to ignore that.
    string(APPEND CMAKE_CXX_FLAGS " -Wno-c++11-narrowing")
  endif()

  if(NOT CA_CL_CTS_ENABLE_WARNINGS AND MSVC)
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  endif()

  if(CMAKE_HOST_UNIX AND CA_PLATFORM_WINDOWS AND
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Include a special header for MinGW cross-compile builds from Linux to
    # work-around headers being case-sensitive on Linux but not Windows.  This
    # is such an obscure configuration that I'm not sure that we can upstream
    # the change.  Don't use target_include_directories to avoid needing to set
    # it on all CTS targets.
    string(APPEND CMAKE_C_FLAGS " -I${CMAKE_CURRENT_SOURCE_DIR}/include")
    string(APPEND CMAKE_CXX_FLAGS " -I${CMAKE_CURRENT_SOURCE_DIR}/include")
  endif()

  # The CTS makes use of hex floats, which is only part of C++ since C++17.
  # Compiling .c files as C99 and not C++ would likely also fix the issue.
  set(OLD_CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})
  set(CMAKE_CXX_STANDARD 11)
  set(OLD_CMAKE_CXX_EXTENSIONS ${CMAKE_CXX_EXTENSIONS})
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    set(CMAKE_CXX_EXTENSIONS ON)
  endif()

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-CTS/test_conformance)

  set(CMAKE_CXX_EXTENSIONS ${OLD_CMAKE_CXX_EXTENSIONS})
  set(CMAKE_CXX_STANDARD ${OLD_CMAKE_CXX_STANDARD})

  if(NOT CA_CL_CTS_ENABLE_WARNINGS)
    set_property(TARGET
      conformance_test_thread_dimensions conformance_test_api
      conformance_test_relationals conformance_test_conversions
      conformance_test_integer_ops conformance_test_image_streams
      conformance_test_vecstep conformance_test_headers
      conformance_test_opencl_h conformance_test_cl_gl_h_c99
      conformance_test_basic conformance_test_buffers
      conformance_test_cl_gl_h conformance_test_allocations
      conformance_test_contractions conformance_test_cl_fill_images
      conformance_test_compiler conformance_test_bruteforce
      conformance_test_spir conformance_test_spirv_new conformance_test_printf
      conformance_test_kernel_image_methods conformance_test_samplerless_reads
      conformance_test_multiples conformance_test_half
      conformance_test_cl_h_c99 conformance_test_device_partition
      conformance_test_cl_platform_h conformance_test_select
      conformance_test_opencl_h_c99 conformance_test_geometrics
      conformance_test_events conformance_test_vecalign
      conformance_test_profiling conformance_test_computeinfo
      conformance_test_cl_get_info conformance_test_cl_read_write_images
      conformance_test_commonfns conformance_test_atomics
      conformance_test_cl_copy_images conformance_test_mem_host_flags
      conformance_test_cl_platform_h_c99 conformance_test_cl_h
      PROPERTY COMPILE_FLAGS -w)  # Disable all warnings
  endif()

  # Unzip the spir test inputs to support using the no-unzip flag to avoid
  # filesystem data races when running the OpenCL-CTS through city runner.
  set(spirTestNames
    api atomics basic binary_type commonfns compile_and_link conversions
    enum_values geometrics half images_kernel_read_write images_samplerlessRead
    integer_ops kernel_attributes kernel_image_methods math_brute_force
    printf profiling relationals sampler_enumeration select vec_align vec_step)
  set(spirTestSourceDir
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-CTS/test_conformance/spir)
  # Depend on conformance_test_spir to ensure the zip file exist.
  add_custom_target(OpenCLCTS-spir-unzip ALL DEPENDS conformance_test_spir)
  add_dependencies(OpenCLCTS OpenCLCTS-spir-unzip)
  foreach(testName ${spirTestNames})
    set(zipFile ${spirTestSourceDir}/${testName}.zip)
    # Unzip the zip files and add them to the OpenCLCTS install target.
    add_custom_command(TARGET OpenCLCTS-spir-unzip POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E tar x ${zipFile}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    install(DIRECTORY ${PROJECT_BINARY_DIR}/bin/${testName}
      DESTINATION bin COMPONENT OpenCLCTS)
  endforeach()

  set(CMAKE_C_FLAGS ${BACKUP_CMAKE_C_FLAGS})
  set(CMAKE_CXX_FLAGS ${BACKUP_CMAKE_CXX_FLAGS})

  if(CA_RUNTIME_COMPILER_ENABLED AND TARGET check)
    # Specify the subset of CTS tests to run as checks.
    set(CTSTests cl12/basic cl12/compile cl12/spirv_new)

    set(CTSSummaryPath ${PROJECT_SOURCE_DIR}/scripts/jenkins/cts_summary)
    # TODO fix this in CA-2695
    if(LLVM_VERSION_MAJOR GREATER 10)
      set(CTSSubTestsCodeplayWimpy
        ${CTSSummaryPath}/opencl_conformance_tests_subtests_codeplay_wimpy_LLVM11.csv)
    else()
      set(CTSSubTestsCodeplayWimpy
        ${CTSSummaryPath}/opencl_conformance_tests_subtests_codeplay_wimpy.csv)
    endif()

    if(CA_CL_ENABLE_INTERCEPT_LAYER)
      # Set the path to the OpenCL-ICD-Loader for the OpenCL-Intercept-Layer.
      set(IcdLoaderInstallDir
        ${PROJECT_BINARY_DIR}/source/cl/external/OpenCL-ICD-Loader/install)
      if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        set(OpenCLFileName ${IcdLoaderInstallDir}/lib/libOpenCL.so)
      elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
        set(OpenCLFileName ${IcdLoaderInstallDir}/bin/OpenCL.dll)
      endif()
    endif()

    if(CA_CL_CTS_ENABLE_CHECK)
      if(CMAKE_CROSSCOMPILING)
        # If no emulator is specified during cross-compile checks are disabled.
        if(CMAKE_CROSSCOMPILING_EMULATOR)
          # We are cross-compiling and an emulator was specified, add the
          # check-cl-cts target invoking run_cities.py with the qemu profile.
          string(REPLACE ";" " " CTSEmulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
          add_ca_check(cl-cts
            COMMAND ${PYTHON_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py
              --profile qemu --qemu "${CTSEmulator}"
              --binary-path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
              --test-source ${CTSSubTestsCodeplayWimpy}
              --add-env-var "OCL_ICD_FILENAMES=$<TARGET_FILE:CL>"
              --add-env-var "CLI_OpenCLFileName=${OpenCLFileName}"
              --junit-result-file ${PROJECT_BINARY_DIR}/cl-cts.xml ${CTSTests}
            CLEAN ${PROJECT_BINARY_DIR}/cl-cts.xml
            DEPENDS OpenCLCTS NOEMULATE)
          add_ca_check(cl-cts-vecz-never
            COMMAND ${PYTHON_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py
              --profile qemu --qemu "${CTSEmulator}"
              --binary-path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
              --test-source ${CTSSubTestsCodeplayWimpy}
              --add-env-var "OCL_ICD_FILENAMES=$<TARGET_FILE:CL>"
              --add-env-var "CLI_OpenCLFileName=${OpenCLFileName}"
              --add-env-var "CA_EXTRA_COMPILE_OPTS=-cl-wfv=never"
              --junit-result-file ${PROJECT_BINARY_DIR}/cl-cts.xml ${CTSTests}
            CLEAN ${PROJECT_BINARY_DIR}/cl-cts.xml
            DEPENDS OpenCLCTS NOEMULATE)
        endif()
      else()
        # We are compiling a native build, add the check-cl-cts target invoking
        # run_cities.py with the default profile.
        add_ca_check(cl-cts
          COMMAND ${PYTHON_EXECUTABLE}
          ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py
            --binary-path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            --test-source ${CTSSubTestsCodeplayWimpy}
            --add-env-var "OCL_ICD_FILENAMES=$<TARGET_FILE:CL>"
            --add-env-var "CLI_OpenCLFileName=${OpenCLFileName}"
            --add-env-var "CL_PLATFORM_INDEX=0"
            --junit-result-file ${PROJECT_BINARY_DIR}/cl-cts.xml ${CTSTests}
          CLEAN ${PROJECT_BINARY_DIR}/cl-cts.xml
          DEPENDS OpenCLCTS)
        add_ca_check(cl-cts-vecz-never
          COMMAND ${PYTHON_EXECUTABLE}
          ${PROJECT_SOURCE_DIR}/scripts/testing/run_cities.py
            --binary-path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            --test-source ${CTSSubTestsCodeplayWimpy}
            --add-env-var "OCL_ICD_FILENAMES=$<TARGET_FILE:CL>"
            --add-env-var "CLI_OpenCLFileName=${OpenCLFileName}"
            --add-env-var "CL_PLATFORM_INDEX=0"
            --add-env-var "CA_EXTRA_COMPILE_OPTS=-cl-wfv=never"
            --junit-result-file ${PROJECT_BINARY_DIR}/cl-cts.xml ${CTSTests}
          CLEAN ${PROJECT_BINARY_DIR}/cl-cts.xml
          DEPENDS OpenCLCTS)
      endif()
    endif()
  endif()
endif()
