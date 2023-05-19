# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
  source/urEventWait.cpp
  source/urInit.cpp
  source/urKernelCreate.cpp
  source/urKernelRetain.cpp
  source/urKernelRelease.cpp
  source/urKernelSetArgMemObj.cpp
  source/urKernelSetArgValue.cpp
  source/urMemBufferCreate.cpp
  source/urMemRetain.cpp
  source/urMemRelease.cpp
  source/urPlatformGet.cpp
  source/urPlatformGetInfo.cpp
  source/urProgramBuild.cpp
  source/urProgramCompile.cpp
  source/urProgramCreateWithIL.cpp
  source/urProgramLink.cpp
  source/urProgramRelease.cpp
  source/urProgramRetain.cpp
  source/urQueueCreate.cpp
  source/urQueueFinish.cpp
  source/urQueueFlush.cpp
  source/urQueueRetain.cpp
  source/urQueueRelease.cpp
  source/urTearDown.cpp
  source/urUSMDeviceAlloc.cpp
  source/urUSMFree.cpp
  source/urUSMHostAlloc.cpp)

target_include_directories(UnitUR PRIVATE include)
target_link_libraries(UnitUR PRIVATE ca_gtest cargo UR)
