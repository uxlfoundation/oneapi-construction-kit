// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <utils/system.h>

#if defined(CA_PLATFORM_LINUX) || defined(CA_PLATFORM_ANDROID)
#include <time.h>
#elif defined(CA_PLATFORM_MAC)
#include <mach/clock.h>
#include <mach/mach.h>
#elif defined(CA_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(CA_PLATFORM_QNX)
// No QNX support
#else
#error Current platform not supported!
#endif

namespace utils {
uint64_t timestampMicroSeconds() { return timestampNanoSeconds() / 1000; }

uint64_t timestampNanoSeconds() {
  uint64_t tickCount = 0;

  // WARNING: To change the system timing calls below coordinate first with the
  // ComputeCpp team to synchronize with their tracing. If both teams use the
  // same time calls, then the tracing information of both teams can be combined
  // without problems.
#if defined(CA_PLATFORM_LINUX) || defined(CA_PLATFORM_ANDROID)
  timespec ts;
  if (::clock_gettime(CLOCK_REALTIME, &ts) == 0) {
    tickCount = static_cast<uint64_t>(ts.tv_sec) * 1000000000 +
                static_cast<uint64_t>(ts.tv_nsec);
  }
#elif defined(CA_PLATFORM_MAC)
  kern_result_t res;
  clock_serv_t cclock;
  mach_timespec_t mts;
  res = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  if (res == KERN_SUCCESS) {
    res = clock_get_time(cclock, &mts);
    if (res == KERN_SUCCESS) {
      tickCount = static_cast<uint64_t>(mts.tv_sec) * 1000000000 +
                  static_cast<uint64_t>(mts.tv_nsec);
    }
    mach_port_deallocate(mach_task_self(), cclock);
  }
#elif defined(CA_PLATFORM_WINDOWS)
  LARGE_INTEGER counter;
  LARGE_INTEGER frequency;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&frequency);
  tickCount = static_cast<uint64_t>(counter.QuadPart) * 1000000000 /
              static_cast<uint64_t>(frequency.QuadPart);
#elif defined(CA_PLATFORM_QNX)
// No QNX support
#else
#error Current platform not supported!
#endif

  return tickCount;
}
}  // namespace utils
