// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UR_MODULE_H_INCLUDED
#define UR_MODULE_H_INCLUDED

#include "cargo/dynamic_array.h"
#include "cargo/expected.h"
#include "cargo/string_view.h"
#include "ur/base.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_module_handle_t_ API object.
struct ur_module_handle_t_ : ur::base {
  /// @brief Constructor for creating module.
  ///
  /// @param[in] context The context to which this module will belong.
  /// @param[in] source The spirv source to create the module from.
  /// @param[in] options Options to create the module with.
  ur_module_handle_t_(ur_context_handle_t context,
                      cargo::dynamic_array<uint32_t> source,
                      cargo::dynamic_array<char> options)
      : context(context),
        source(std::move(source)),
        options(std::move(options)) {}
  ur_module_handle_t_(const ur_module_handle_t_ &) = delete;
  ur_module_handle_t_ &operator=(const ur_module_handle_t_ &) = delete;

  /// @brief Factory method for creating modules.
  ///
  /// @param[in] context Context to which this module will belong.
  /// @param[in] source The source spir-v to create the module from.
  /// @param[in] length the length of `source` in bytes.
  /// @param[in] compilation_options Compiler options to pass to the spirv
  /// compiler.
  static cargo::expected<ur_module_handle_t, ur_result_t> create(
      ur_context_handle_t context, const void *source, uint32_t length,
      cargo::string_view compilation_options);

  /// @brief The context to which this module belongs.
  ur_context_handle_t context = nullptr;
  /// @brief The source spir-v.
  cargo::dynamic_array<uint32_t> source;
  /// @brief The options the module was created with.
  cargo::dynamic_array<char> options;
};

#endif  // UR_MODULE_H_INCLUDED
