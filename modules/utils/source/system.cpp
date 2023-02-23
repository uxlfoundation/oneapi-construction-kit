// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  ::clock_gettime(CLOCK_REALTIME, &ts);
  tickCount = static_cast<uint64_t>(ts.tv_sec) * 1000000000 +
              static_cast<uint64_t>(ts.tv_nsec);
#elif defined(CA_PLATFORM_MAC)
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  tickCount = static_cast<uint64_t>(mts.tv_sec) * 1000000000 +
              static_cast<uint64_t>(mts.tv_nsec);
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
