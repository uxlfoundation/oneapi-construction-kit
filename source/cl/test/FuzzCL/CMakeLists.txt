# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_library(Fuzz STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/FuzzCL/context.h)

target_include_directories(Fuzz
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(Fuzz PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>)

target_link_libraries(Fuzz PUBLIC cargo)

# Offline compile kernels
if(CA_CL_ENABLE_OFFLINE_KERNEL_TESTS)
  add_dependencies(Fuzz FuzzCLKernels)
  if(CMAKE_CROSSCOMPILING)
    if(TARGET clc_native)
      set(CLC_EXECUTABLE clc_native)
      set(CLC_TARGET clc_native)
    elseif(CMAKE_CROSSCOMPILING_EMULATOR)
      set(CLC_EXECUTABLE ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:clc>)
      set(CLC_TARGET clc)
    endif()
  else()
    set(CLC_EXECUTABLE clc)
    set(CLC_TARGET clc)
  endif()
  set(kernel ${CMAKE_CURRENT_SOURCE_DIR}/kernels/kernel.cl)
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bin/FuzzCL_kernels)
  foreach(target ${MUX_TARGET_LIBRARIES})
    foreach(device_name ${${target}_DEVICE_NAMES})
      set(output ${PROJECT_BINARY_DIR}/bin/FuzzCL_kernels/${device_name})
      add_custom_command(OUTPUT ${output}
        COMMAND ${CLC_EXECUTABLE}
          -d ${device_name}
          -cl-kernel-arg-info ${CLC_FILE_OPTIONS}
          -o ${output} -- ${kernel}
        DEPENDS ${CLC_TARGET} ${kernel}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Building FuzzCL offline kernel")
      list(APPEND kernels ${output})
    endforeach()
  endforeach()
  add_custom_target(FuzzCLKernels DEPENDS ${kernels})
  install(DIRECTORY
    ${PROJECT_BINARY_DIR}/bin/FuzzCL_kernels
    DESTINATION bin COMPONENT Fuzz)
else()
  # if offline kernels is disabled, remove the kernel folder
  file(REMOVE_RECURSE ${PROJECT_BINARY_DIR}/bin/FuzzCL_kernels)
endif()

# Add the libfuzzer entry point
if(CA_USE_SANITIZER STREQUAL "Fuzzer")
  add_ca_cl_executable(FuzzGenCorpus
    ${CMAKE_CURRENT_SOURCE_DIR}/source/libfuzzer_target.cpp)

  target_compile_options(FuzzGenCorpus PRIVATE -fsanitize=fuzzer)
  set_target_properties(FuzzGenCorpus PROPERTIES LINK_FLAGS -fsanitize=fuzzer)

  target_link_libraries(FuzzGenCorpus PRIVATE Fuzz)
  install(TARGETS FuzzGenCorpus RUNTIME DESTINATION bin COMPONENT Fuzz)
endif()

# Add FuzzCL
add_ca_cl_executable(FuzzCL
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fuzz.cpp)

target_link_libraries(FuzzCL PRIVATE Fuzz)
install(TARGETS FuzzCL RUNTIME DESTINATION bin COMPONENT Fuzz)
