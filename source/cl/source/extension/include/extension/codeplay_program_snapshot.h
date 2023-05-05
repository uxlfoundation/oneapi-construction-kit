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

/// @file
///
/// @brief Extension cl_codeplay_program_snapshot API.

#ifndef EXTENSION_CODEPLAY_PROGRAM_SNAPSHOT_H_INCLUDED
#define EXTENSION_CODEPLAY_PROGRAM_SNAPSHOT_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Handles OpenCL extension queries for cl_codeplay_program_snapshot.
///
/// This extension provides a snapshot mechanism allowing user code
/// to capture program objects at different stages of compilation.
///
/// The snapshot mechanism is the primary method of debugging compiler
/// transformations. This is done by dumping the program object (taking a
/// snapshot) at a specific stage of compilation in order to inspect the effects
/// of compiler passes. So allowing the user to more effectively debug and tune
/// their code.
class codeplay_program_snapshot final : public extension {
 public:
  /// @brief Default constructor.
  codeplay_program_snapshot();

  /// @brief Destructor.
  virtual ~codeplay_program_snapshot() override;

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes the following extension functions
  /// query-able:
  /// * "clRequestProgramSnapshotListCODEPLAY"
  /// * "clRequestProgramSnapshotCODEPLAY"
  ///
  /// @see clGetExtensionFunctionAddressForPlatform.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_PROGRAM_SNAPSHOT_H_INCLUDED
