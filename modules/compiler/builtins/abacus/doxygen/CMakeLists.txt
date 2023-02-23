find_package(Doxygen)

if(DOXYGEN_FOUND)
  message(STATUS "Doxygen found, generating Abacus documentation!")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY)

  add_custom_target(abacus_documentation ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)

  add_dependencies(abacus_documentation abacus_generate)
else()
  message(WARNING "Doxygen not found, cannot generate Abacus documentation!")
endif()
