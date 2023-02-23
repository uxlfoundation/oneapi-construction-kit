# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(UnitUR
  include/uur/checks.h
  include/uur/environment.h source/environment.cpp
  include/uur/fixtures.h
  include/uur/rect_helpers.h
  source/main.cpp
  source/urContextCreate.cpp
  source/urContextRetain.cpp
  source/urContextRelease.cpp
  source/urDeviceGet.cpp
  source/urEnqueueEventsWait.cpp
  source/urEnqueueEventsWaitWithBarrier.cpp
  source/urEnqueueKernelLaunch.cpp
  source/urEnqueueMemBufferCopy.cpp
  source/urEnqueueMemBufferCopyRect.cpp
  source/urEnqueueMemBufferFill.cpp
  source/urEnqueueMemBufferMap.cpp
  source/urEnqueueMemBufferRead.cpp
  source/urEnqueueMemBufferReadRect.cpp
  source/urEnqueueMemBufferWrite.cpp
  source/urEnqueueMemBufferWriteRect.cpp
  source/urEnqueueMemUnmap.cpp
  source/urEnqueueUSMMemcpy.cpp
  source/urEnqueueUSMMemset.cpp
  source/urEventWait.cpp
  source/urInit.cpp
  source/urKernelCreate.cpp
  source/urKernelRetain.cpp
  source/urKernelRelease.cpp
  source/urKernelSetArgMemObj.cpp
  source/urKernelSetArgValue.cpp
  source/urMemBufferCreate.cpp
  source/urMemFree.cpp
  source/urMemRetain.cpp
  source/urMemRelease.cpp
  source/urModuleCreate.cpp
  source/urModuleRetain.cpp
  source/urModuleRelease.cpp
  source/urPlatformGet.cpp
  source/urPlatformGetInfo.cpp
  source/urProgramCreate.cpp
  source/urProgramRetain.cpp
  source/urProgramRelease.cpp
  source/urQueueCreate.cpp
  source/urQueueFinish.cpp
  source/urQueueFlush.cpp
  source/urQueueRetain.cpp
  source/urQueueRelease.cpp
  source/urTearDown.cpp
  source/urUSMDeviceAlloc.cpp
  source/urUSMHostAlloc.cpp)

target_include_directories(UnitUR PRIVATE include)
target_link_libraries(UnitUR PRIVATE ca_gtest cargo UR)
