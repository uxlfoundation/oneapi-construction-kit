# Copyright (C) Codeplay Software Limited. All Rights Reserved.


find_package(LLVMTool COMPONENTS FileCheck)
if(NOT LLVMTool_FOUND)
  message(WARNING "GeneratedTests disabled: FileCheck not found")
  return()
endif()

# Required files that need to be copied
set(LIT_FILES LICENSE.txt README.md)

set(TARGET_LIT_FILES "")

file(GLOB_RECURSE GENERATED_LIT_FILES RELATIVE
  ${CMAKE_CURRENT_SOURCE_DIR} Tests/*)

# Copy the test and any other files that are needed
foreach(FILE ${LIT_FILES} ${GENERATED_LIT_FILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
    ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    COPYONLY)
  list(APPEND TARGET_LIT_FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE})
endforeach()

# Generate the configuration file
add_ca_configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg
  DEFINED
  oclc_EXECUTABLE=$<TARGET_FILE:oclc>
  FileCheck_EXECUTABLE=$<TARGET_FILE:LLVM::FileCheck>
  DEPENDS oclc)

add_custom_target(GeneratedTests ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg)

if(CA_CL_ENABLE_GENERATEDTESTS_CHECK)
  add_ca_check(GeneratedTests
    COMMAND lit -v ${CMAKE_CURRENT_BINARY_DIR}
    --xunit-xml-output=${PROJECT_BINARY_DIR}/GeneratedTests.xml
    CLEAN ${PROJECT_BINARY_DIR}/GeneratedTests.xml
    DEPENDS GeneratedTests)
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION share COMPONENT OCLTest)
