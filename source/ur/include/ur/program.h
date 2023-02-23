// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UR_PROGRAM_H_INCLUDED
#define UR_PROGRAM_H_INCLUDED

#include <unordered_map>

#include "cargo/array_view.h"
#include "cargo/expected.h"
#include "cargo/small_vector.h"
#include "cargo/string_view.h"
#include "mux/mux.h"
#include "ur/base.h"
#include "ur/kernel.h"

namespace ur {
/// @brief Helper type representing program metadata.
struct program_info_t {
  /// @brief Lookup kernel by index into list of kernels in program.
  ///
  /// @param[in] kernel_index Index of kernel in list of kernels belonging to
  /// the program.
  ///
  /// @return kernel object or nullptr if `kernel_index` is out of range.
  kernel_data_t *getKernel(size_t kernel_index);

  /// @brief Lookup kernel by name in list of kernels in program.
  ///
  /// @param[in] name Kernel name.
  ///
  /// @return kernel object or nullptr if no kernel with name `name` exists in
  /// the program.
  kernel_data_t *getKernel(cargo::string_view name);

  /// @brief Metadata about each kernel in the program.
  cargo::small_vector<kernel_data_t, 8> kernel_descriptions;
};
}  // namespace ur

/// @brief Compute Mux specific implementation of the opaque
/// ur_program_handle_t_ API object.
struct ur_program_handle_t_ : ur::base {
  /// @brief Helper type representing device specific instance of the program.
  struct device_program_t {
    /// @brief Number of errors that occur during compilation.
    uint32_t num_errors = 0;
    /// @brief Log of compilation output.
    std::string log;
    /// @brief The device specific mux executable representing a binary file for
    /// the compiled program.
    mux_executable_t mux_executable = nullptr;
  };

  /// @brief Constructor to construct program.
  ///
  /// @param[in] context Context to which the program will belong.
  ur_program_handle_t_(ur_context_handle_t context) : context(context) {}
  ur_program_handle_t_(const ur_program_handle_t_ &) = delete;
  ur_program_handle_t_ &operator=(const ur_program_handle_t_ &) = delete;
  ~ur_program_handle_t_();

  /// @brief Factory method for creating programs.
  ///
  /// @param[in] context Context to which the program will belong.
  /// @param[in] modules The list of modules to link into the program.
  /// @param[in] linker_options Options to pass to the linker when `modules` are
  /// linked.
  ///
  /// @return Program object or error code if something went wrong.
  static cargo::expected<ur_program_handle_t, ur_result_t> create(
      ur_context_handle_t context,
      cargo::array_view<const ur_module_handle_t> modules,
      cargo::string_view linker_options);

  /// @brief Context to which the program belongs.
  ur_context_handle_t context = nullptr;
  /// @brief Map from device indices in the order they appear in `context` to
  /// device specific programs.
  std::unordered_map<ur_device_handle_t, device_program_t> device_program_map;
  /// @brief Metadata about the program.
  ur::program_info_t program_info;
};

#endif  // UR_PROGRAM_H_INCLUDED
