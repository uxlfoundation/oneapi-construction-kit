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

#include <tracer/tracer.h>
#include <utils/system.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#if defined(__linux__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#include <direct.h>
#include <windows.h>
#elif defined(__APPLE__) || defined(__QNX__) || defined(__MCOS_POSIX__)
// These platforms are known to be unsupported, and have a stub implementation.
#endif
#include <atomic>
#include <mutex>

namespace {

#if defined(__linux__)
const int pid = static_cast<int>(syscall(SYS_getpid));
thread_local const int tid = static_cast<int>(syscall(SYS_gettid));
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
const int pid = static_cast<int>(GetCurrentProcessId());
thread_local const int tid = static_cast<int>(GetCurrentThreadId());
#elif defined(__APPLE__) || defined(__QNX__) || defined(__MCOS_POSIX__)
// These platforms are known to be unsupported, and have a stub implementation.
#else
#error Platform not supported!
#endif

#if defined(__linux__)
struct TracerVirtualMemFileImpl {
  explicit TracerVirtualMemFileImpl()
      : export_file(std::getenv("CA_TRACE_FILE")) {
    const uint64_t start = tracer::getCurrentTimestamp();

    if ((nullptr == export_file) || (0 == std::strlen(export_file))) {
      return;
    }
    tmp_name = "/tmp/ca_" + std::to_string(pid) + ".tracer";
    const int f = open(tmp_name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (f == 0) {
      (void)fprintf(stderr, "Could not open %s temp file for tracing.\n",
                    tmp_name.c_str());
      return;
    }

    int requested_mb = 1024;

    const char *mb_str = std::getenv("CA_TRACE_FILE_BUFFER_MB");

    if (nullptr != mb_str && std::strlen(mb_str)) {
      /* seems good to have a max, 75GB */
      constexpr int max_mb = 76800;
      requested_mb = std::min(std::stoi(mb_str), max_mb);
      requested_mb = std::max(requested_mb, 0);
    }

    // MB to bytes
    const size_t bytes = 1048576 * requested_mb;
    max_offset = bytes;

    /* resize the file */
    lseek64(f, bytes, SEEK_SET);
    auto written = write(f, "", 1);
    if (0 == written) {
      (void)fprintf(stderr, "Failed to resize %s.\n", tmp_name.c_str());
      (void)remove(tmp_name.c_str());
      return;
    }
    lseek64(f, 0, SEEK_SET);

    void *mapping = mmap(0, bytes, PROT_WRITE, MAP_SHARED, f, 0);
    map = static_cast<char *>(mapping);
    close(f);

    if (map == MAP_FAILED) {
      (void)fprintf(stderr, "Failed to map tmp file:%s.\n", strerror(errno));
      (void)remove(tmp_name.c_str());
    }

    // Buffer to keep things stack allocated.
    char buf[256]{};

    int consumed = std::snprintf(buf, sizeof(buf),
                                 "{\n\t\"otherData\":{},\n\t\"traceEvents\":[");
    writeToMemMap(buf, consumed);
    std::memset((void *)buf, 0, sizeof(buf));

    // Insert dummy value so we can ignore JSON's comma requirements.
    const uint64_t end = tracer::getCurrentTimestamp();

    consumed = std::snprintf(
        buf, sizeof(buf),
        "\n\t\t{\"name\":\"%s\", "
        "\"cat\":\"%s\",\"ph\":\"X\",\"pid\":%d,\"tid\":%d,\"ts\":%" PRIu64
        ","
        "\"dur\":%" PRIu64 "}",
        "tracer-startup", "tracer-startup", pid, tid, start, end - start);

    writeToMemMap(buf, consumed);
  };

  ~TracerVirtualMemFileImpl() {
    if (0 != map) {
      const char *ending = "\n\t]\n}\n";

      if (std::strlen(ending) + offset > max_offset) {
        (void)fprintf(stderr,
                      "Trace overflow, failed to write data, increase "
                      "CA_TRACE_FILE_BUFFER_MB");
      }

      writeToMemMap(ending, std::strlen(ending));

      // Copy mem-mapped file into a proper file.
      // This reduces the footprint, and stops to some file issues, with editors
      // opening up empty and/or very large files.
      FILE *file = fopen(export_file, "w");

      if (nullptr != file) {
        const size_t idx = fwrite(map, sizeof(map[0]), offset.load(), file);
        fclose(file);

        if (idx != offset.load()) {
          (void)fprintf(stderr, "Trace file could not be shrunk down.");
        }
      }

      munmap(map, max_offset);
      map = nullptr;
      remove(tmp_name.c_str());
    }
  }

  void doTrace(const char *name, const char *category, uint64_t start,
               uint64_t end) {
    // Buffer to keep stack allocated.
    char buf[256]{};
    const int consumed = std::snprintf(
        buf, sizeof(buf),
        ",\n\t\t{\"name\":\"%s\", "
        "\"cat\":\"%s\",\"ph\":\"X\",\"pid\":%d,\"tid\":%d,\"ts\":%" PRIu64
        ","
        "\"dur\":%" PRIu64 "}",
        name, category, pid, tid, start, end - start);
    writeToMemMap(buf, consumed);
  }

 private:
  void writeToMemMap(const char *buf, int size) {
    if (map == nullptr || size <= 0) {
      return;
    }

    const uint64_t insert_pt = offset.fetch_add(size);

    if ((insert_pt + size) < max_offset) {
      std::memcpy((void *)&map[insert_pt], (void *)buf, size);
    }
  }

  uint64_t max_offset{0};
  char *map{nullptr};
  const char *export_file{nullptr};
  std::string tmp_name{""};
  std::atomic<uint64_t> offset{0};
};

#elif defined(_WIN32)
/*
 *  TracerFileImpl creates a file and appends to it while holding a mutex.
 */
struct TracerFileImpl {
  explicit TracerFileImpl() {
    uint64_t start = tracer::getCurrentTimestamp();

    const char *env = std::getenv("CA_TRACE_FILE");

    // If we couldn't find an env variable for the user folder or the returned
    // value was an empty string, bail out.
    if ((nullptr == env) || (0 == std::strlen(env))) {
      return;
    }

    // KLOCWORK "SV.TAINTED.PATH_TRAVERSAL" possible false positive
    // Opening a file based on an environment variable is a security issue.
    // Here, it's mitigated by the fact that tracer is a debug feature, not a
    // release feature.
    file = fopen(env, "w");

    if (nullptr == file) {
      (void)fprintf(stderr, "Could not open '%s' for tracing.\n", env);
      return;
    }

    // Now we have a file handle, we need to output the start of the JSON
    // tracing format.
    fprintf(file, "{\n\t\"otherData\":{},\n\t\"traceEvents\":[");

    // Insert dummy value so we can ignore JSON's comma requirements.
    uint64_t end = tracer::getCurrentTimestamp();

    fprintf(file,
            "\n\t\t{\"name\":\"%s\", "
            "\"cat\":\"%s\",\"ph\":\"X\",\"pid\":%d,\"tid\":%d,\"ts\":%" PRIu64
            ","
            "\"dur\":%" PRIu64 "}",
            "tracer-startup", "tracer-startup", pid, tid, start, end - start);
  }

  ~TracerFileImpl() {
    if (nullptr != file) {
      // Lastly we close our traceEvents member, and the main JSON object.
      fprintf(file, "\n\t]\n}\n");

      // And close the file.
      fclose(file);
    }
  }

  void doTrace(const char *name, const char *category, uint64_t start,
               uint64_t end) {
    std::lock_guard<std::mutex> lock(mtx);

    if (nullptr != file) {
      fprintf(
          file,
          ",\n\t\t{\"name\":\"%s\", "
          "\"cat\":\"%s\",\"ph\":\"X\",\"pid\":%d,\"tid\":%d,\"ts\":%" PRIu64
          ","
          "\"dur\":%" PRIu64 "}",
          name, category, pid, tid, start, end - start);
    }
  }

  std::mutex mtx;
  FILE *file{nullptr};
};
#elif defined(__APPLE__) || defined(__QNX__) || defined(__MCOS_POSIX__)
// These platforms are known to be unsupported, and have a stub implementation.
struct TracerVirtualMemFileImpl {
  void doTrace(const char *, const char *, uint64_t, uint64_t) {}
};
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(__QNX__) || \
    defined(__MCOS_POSIX__)
TracerVirtualMemFileImpl trace_impl;
#elif defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
TracerFileImpl trace_impl;
#endif

}  // namespace

uint64_t tracer::getCurrentTimestamp() {
  return utils::timestampMicroSeconds();
}

void tracer::recordTrace(const char *name, const char *category, uint64_t start,
                         uint64_t end) {
  trace_impl.doTrace(name, category, start, end);
}
