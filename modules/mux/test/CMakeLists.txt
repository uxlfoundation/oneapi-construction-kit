# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(UnitMux
  ${CMAKE_CURRENT_SOURCE_DIR}/c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetDeviceInfos.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateDevices.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyDevice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetSupportedImageFormats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateImage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyImage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxAllocateMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxBindBufferMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxBindImageMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateMemoryFromHost.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxFlushMappedMemoryFromDevice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxFlushMappedMemoryToDevice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxMapMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxUnmapMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxFreeMemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandBeginQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandCopyBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandCopyBufferRegions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandEndQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandFillBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandReadBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandReadBufferRegions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandResetQueryPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandUserCallback.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandWriteBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandWriteBufferRegions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxResetCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxFinalizeCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCloneCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyCommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateSemaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateFence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyFence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxResetFence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxResetSemaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroySemaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetQueue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDispatch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxTryWait.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxWaitAll.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateQueryPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetQueryCounterRequiredPasses.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetSupportedQueryCounters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyQueryPool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCommandNDRange.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/application.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/builtin_kernel_application.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxGetQueryPoolResults.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxUpdateDescriptors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateKernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyKernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxCreateExecutable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxDestroyExecutable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxQuerySubGroupSizeForLocalSize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/muxQueryLocalSizeForSubGroupCount.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>
  )

target_include_directories(UnitMux PRIVATE ${MUX_SOURCE_DIR}/include)
target_link_libraries(UnitMux PRIVATE mux ca_gtest_main compiler-loader)
target_resources(UnitMux NAMESPACES ${BUILTINS_NAMESPACES})

add_ca_check(UnitMux GTEST
  COMMAND UnitMux --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitMux.xml
  CLEAN ${PROJECT_BINARY_DIR}/UnitMux.xml
  DEPENDS UnitMux)

install(TARGETS UnitMux RUNTIME DESTINATION bin COMPONENT Mux)
