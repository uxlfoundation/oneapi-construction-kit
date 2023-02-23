// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Extension cl_codeplay_program_snapshot API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
