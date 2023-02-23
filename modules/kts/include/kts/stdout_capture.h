// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef KTS_STDOUT_CAPTURE_H_INCLUDED
#define KTS_STDOUT_CAPTURE_H_INCLUDED

#include <string>

namespace kts {

/// @brief Handles capturing stdout to help verify device code output
/// from printf builtins.
struct StdoutCapture {
  /// @brief     Prevent stdout from reaching the display, but capture output.
  /// @return    None
  void CaptureStdout();
  /// @brief     Reenable the usual stdout.
  /// @return    None
  void RestoreStdout();
  /// @brief     Read-back the captured stdout buffer and delete the temp file.
  /// @return    Everything that was captured from stdout, as a string.
  std::string ReadBuffer();

 private:
  /// @brief Original file descriptor for stdout which needs to be restored
  int original_fd = -1;
  /// @brief Temporary file create to catch redirected stdout
  FILE *stdout_tmp = nullptr;
};
}  // namespace kts

#endif  // KTS_STDOUT_CAPTURE_H_INCLUDED
