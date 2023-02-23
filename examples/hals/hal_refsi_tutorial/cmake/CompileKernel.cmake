# Copyright (C) Codeplay Software Limited. All Rights Reserved.

function(hal_refsi_tutorial_compile_kernel_source OBJECT SRC)
  set(INCLUDES ${ARGN})
  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)
  get_target_property(REFSIDRV_SRC_DIR refsidrv SOURCE_DIR)

  # TODO: Compile a kernel source file (${SRC}) into a kernel object (${OBJECT})
endfunction()

function(hal_refsi_tutorial_link_kernel BINARY)
  set(OBJECTS ${ARGN})
  get_property(EXTRA_OBJECTS GLOBAL PROPERTY HAL_REFSI_TUTORIAL_EXTRA_OBJECTS)
  foreach(OBJECT ${EXTRA_OBJECTS})
    list(APPEND OBJECTS ${OBJECT})
  endforeach()

  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)

  # TODO: Link multiple objects (${OBJECTS}) into a kernel executable (${BINARY})
endfunction()
