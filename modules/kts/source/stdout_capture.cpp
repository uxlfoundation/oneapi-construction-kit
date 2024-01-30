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
//
#if defined(_MSC_VER)
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#endif

#include <gtest/gtest.h>

#include "kts/stdout_capture.h"

namespace {
/// @brief Create a temporary file.
///
/// For MinGW use custom code, otherwise just use std::tmpfile.
///
/// This is necessary with MinGW because it uses the MSVC std::tmpnam for
/// std::tmpfile, and this creates names in the drive root that
/// non-administrator users cannot write to. I don't know why this is not a
/// problem when using MSVC directly.
///
/// @return A handle to a temporary file that will be automatically removed or
/// nullptr if a file cannot be created.
std::FILE *create_tmpfile() {
#if defined(__MINGW32__) || defined(__MINGW64__)
  // We want to store our temporary file in %TEMP%.
  const char *dir = std::getenv("TEMP");
  if (!dir) {
    std::fflush(stdout);  // Try to not lose the error message in the log.
    std::fprintf(stderr, "Could not find '%%TEMP%%'\n");
    return nullptr;
  }

// Because the use of `_tempnam` and `std::fopen` are not atomic there is a
// chance that the two parallel UnitCL's could get the same temporary name and
// only one will succeed in opening a file with that name.  So put the PID into
// the temporary file name to prevent collisions.
#define BUF_SIZE (13 + 20 + 1) /* UnitCL_Printf + -9223372036854775808 + \0 */
  char unique_name[BUF_SIZE];
  long pid = static_cast<long>(getpid());
  int count = std::snprintf(unique_name, BUF_SIZE, "UnitCL_Printf%ld", pid);
  if ((count < 0) || (count >= BUF_SIZE)) {
    std::fflush(stdout);  // Try to not lose the error message in the log.
    std::fprintf(stderr, "Could not create a temporary file name.\n");
    return nullptr;
  }
#undef BUF_SIZE

  // Use the Windows specific _tempnam to create a temporary file name based
  // on a location of our choice.
  char *name = _tempnam(dir, unique_name);
  if (!name) {
    std::fflush(stdout);  // Try to not lose the error message in the log.
    std::fprintf(stderr, "Could not create a temporary name.\n");
    return nullptr;
  }

  // Open the file, mode 'wb+' to match std::tmpfile, additionally 'T'
  // (equivalent to _O_SHORTLIVED) to say that the file can stay in memory if
  // possible, and 'D' (equivalent to _O_TEMPORARY) to say that the file can
  // be deleted when it is closed.
  std::FILE *file = std::fopen(name, "wb+TD");
  std::free(name);
  if (!file) {
    std::fflush(stdout);  // Try to not lose the error message in the log.
    std::fprintf(stderr, "Could not open temporary file.  Errno %d: %s\n",
                 errno, std::strerror(errno));
    return nullptr;
  }

  return file;
#else
  return std::tmpfile();
#endif
}
}  // namespace

namespace kts {

void StdoutCapture::CaptureStdout() {
  ASSERT_EQ(0, std::fflush(stdout));

  stdout_tmp = create_tmpfile();
  ASSERT_TRUE(stdout_tmp != NULL);

  original_fd = DUP(fileno(stdout));
  const int redirect_fd = DUP2(fileno(stdout_tmp), fileno(stdout));
  ASSERT_NE(-1, original_fd);
  ASSERT_NE(-1, redirect_fd);
}

void StdoutCapture::RestoreStdout() {
  ASSERT_EQ(0, std::fflush(stdout));

  const int restored_fd = DUP2(original_fd, fileno(stdout));
  ASSERT_NE(-1, restored_fd);  // If this has failed stdout is not restored.
  original_fd = -1;
}

std::string StdoutCapture::ReadBuffer() {
  std::fseek(stdout_tmp, 0, SEEK_SET);

  // Read in the buffer.
  std::string str("");
  char buf[255];
  while (std::fgets(buf, 255, stdout_tmp)) {
    str.append(buf);
  }

  // Now that we have read our output we can close the temporary file (it
  // will be deleted automatically).
  std::fclose(stdout_tmp);
  stdout_tmp = NULL;

  return str;
}

}  // namespace kts
