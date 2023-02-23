# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(veczc ${CMAKE_CURRENT_SOURCE_DIR}/source/veczc.cpp)
target_compile_options(veczc PRIVATE ${VECZ_COMPILE_OPTIONS})
target_compile_definitions(veczc PRIVATE ${VECZ_COMPILE_DEFINITIONS})
target_include_directories(veczc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(llvm_libs all ${LLVM_TARGETS_TO_BUILD})
# LLVM 8 adds these invalid libraries to the list, remove them to avoid
# attempting to link against LTO-NOTFOUND and OptRemarks-NOTFOUND.
list(REMOVE_ITEM llvm_libs LTO OptRemarks)
target_link_libraries(veczc PUBLIC vecz multi_llvm compiler-utils ${llvm_libs})
install(TARGETS veczc RUNTIME DESTINATION bin COMPONENT VECZ)

add_ca_executable(dx2llvm ${CMAKE_CURRENT_SOURCE_DIR}/source/dx2llvm.cpp)
target_compile_options(dx2llvm PRIVATE ${VECZ_COMPILE_OPTIONS})
target_compile_definitions(dx2llvm PRIVATE ${VECZ_COMPILE_DEFINITIONS})
target_include_directories(dx2llvm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dx2llvm PUBLIC vecz
  LLVMBitReader LLVMBitWriter LLVMCore)
install(TARGETS dx2llvm RUNTIME DESTINATION bin COMPONENT VECZ)
