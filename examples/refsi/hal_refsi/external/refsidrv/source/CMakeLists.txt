# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_library(refsi_common STATIC
  common/elf_loader.cpp
  common/common_devices.cpp
)

target_link_libraries(refsi_common PUBLIC riscv-sim)

target_include_directories(refsi_common PUBLIC
    ${REFSIDRV_SOURCE_DIR}/include
    ${REFSIDRV_SOURCE_DIR}/include/common
)

add_library(refsidrv SHARED
  refsidrv/refsi_accelerator.cpp
  refsidrv/refsi_command_processor.cpp
  refsidrv/refsi_device.cpp
  refsidrv/refsi_device_m.cpp
  refsidrv/refsi_device_g.cpp
  refsidrv/refsi_memory.cpp
  refsidrv/refsi_memory_window.cpp
  refsidrv/refsi_perf_counters.cpp
  refsidrv/refsidrv.cpp
  refsidrv/kernel_dma.cpp
  refsidrv/slim_sim.cpp
  refsidrv/debugger.cpp
  refsidrv/trap_handlers.cpp
)

target_compile_definitions(refsidrv PRIVATE -DBUILD_REFSI_DLL)

target_link_libraries(refsidrv PRIVATE riscv-sim refsi_common hal_common)

target_include_directories(refsidrv PUBLIC
    ${REFSIDRV_SOURCE_DIR}/include
)

target_include_directories(refsidrv PRIVATE
    ${REFSIDRV_SOURCE_DIR}/include/refsidrv
)

install(TARGETS refsidrv
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  )

set_target_properties(refsidrv PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
