// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UR_KERNEL_H_INCLUDED
#define UR_KERNEL_H_INCLUDED

#include <unordered_map>

#include "cargo/expected.h"
#include "cargo/string_view.h"
#include "compiler/module.h"
#include "mux/mux.h"
#include "ur/base.h"

namespace ur {
/// @brief Helper type representing kernel metadata.
struct kernel_data_t {
  /// @brief Helper type representing kernel argument information.
  struct argument_info_t final {
    /// @brief The type of the argument.
    compiler::ArgumentKind kind;
    /// @brief The string representation of the argument type.
    std::string type_name;
    /// @brief The name of the argument.
    std::string name;
  };

  /// @brief Name of the kernel.
  std::string name;
  /// @brief Number of arguments to the kernel
  uint32_t num_arguments;
  /// @brief Attributes set on the kernel.
  std::string attributes;
  /// @brief The types of the arguments in the order they appear.
  cargo::dynamic_array<argument_info_t> argument_types;
  /// @brief Optional info about each argument.
  cargo::optional<cargo::small_vector<argument_info_t, 8>> argument_info;
};

}  // namespace ur

/// @brief Compute Mux specific implementation of the opaque
/// ur_kernel_handle_t_ API object.
struct ur_kernel_handle_t_ : ur::base {
  struct argument_data_t {
    struct {
      void *data;
      size_t size;
    } value;
    ur_mem_handle_t mem_handle;
  };

  /// @brief Constructor to construct kernel.
  ///
  /// @param[in] program Program to create the kernel.
  /// @param[in] kernel_name Name of the kernel in `program`.
  ur_kernel_handle_t_(ur_program_handle_t program,
                      cargo::string_view kernel_name)
      : program(program), kernel_name(kernel_name) {}
  ur_kernel_handle_t_(const ur_kernel_handle_t_ &) = delete;
  ur_kernel_handle_t_ &operator=(const ur_kernel_handle_t_ &) = delete;
  ~ur_kernel_handle_t_();

  /// @brief Factory method for creating kernel objects.
  ///
  /// @param[in] program Program to create the kernel from.
  /// @param[in] kernel_name Name of the kernel in `program` to create.
  ///
  /// @return Kernel object or an error code if something went wrong.
  static cargo::expected<ur_kernel_handle_t, ur_result_t> create(
      ur_program_handle_t program, cargo::string_view kernel_name);

  /// @brief Program from which this program was created.
  ur_program_handle_t program = nullptr;
  /// @brief The name of the kernel in the source.
  cargo::string_view kernel_name;
  /// @brief The arguments to the kernel in the order they appear.
  cargo::dynamic_array<argument_data_t> arguments;
  /// @brief Device specific kernel map, one for each device in the context
  /// increasing in the order of the devices in the context.
  std::unordered_map<ur_device_handle_t, mux_kernel_t> device_kernel_map;
};

#endif  // UR_KERNEL_H_INCLUDED
