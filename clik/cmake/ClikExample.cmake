# Copyright (C) Codeplay Software Limited. All Rights Reserved.

function(add_clik_example)
  set(options)
  set(one_value_args
    TARGET
  )
  set(multi_value_args
    SOURCES
    KERNELS
  )
  cmake_parse_arguments(CLIK_EXAMPLE
    "${options}"
    "${one_value_args}"
    "${multi_value_args}"
    ${ARGN}
  )

  add_executable(${CLIK_EXAMPLE_TARGET} ${CLIK_EXAMPLE_SOURCES})

  # Make the example target depend on the generated kernel targets.
  if(CLIK_EXAMPLE_KERNELS)
    foreach(KERNEL IN ITEMS ${CLIK_EXAMPLE_KERNELS})
      add_dependencies(${CLIK_EXAMPLE_TARGET} ${KERNEL}_binary)
    endforeach()
    target_include_directories(${CLIK_EXAMPLE_TARGET} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
    )
  endif()

  target_link_libraries(${CLIK_EXAMPLE_TARGET} PRIVATE option_parser)
  install(TARGETS ${CLIK_EXAMPLE_TARGET} RUNTIME DESTINATION bin COMPONENT ClikExamples)
  set_target_properties(${CLIK_EXAMPLE_TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
  add_dependencies(ClikExamples ${CLIK_EXAMPLE_TARGET})

endfunction()

add_custom_target(ClikExamples)
