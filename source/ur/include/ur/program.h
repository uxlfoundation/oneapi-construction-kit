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
#include "ur/device.h"
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
    device_program_t(ur_device_handle_t device) {
      module = device->target->createModule(num_errors, log);
    }

    device_program_t() {}

    /// @brief Number of errors that occur during compilation.
    uint32_t num_errors = 0;
    /// @brief Log of compilation output.
    std::string log;
    /// @brief Module object that stores the program compiled to an intermediate
    /// state, and acts as our interface to the compiler library.
    std::unique_ptr<compiler::Module> module;
    /// @brief The device specific mux executable representing a binary file for
    /// the compiled program.
    mux_executable_t mux_executable = nullptr;
    /// @brief Info about the program that can be queried out by the runtime.
    ur::program_info_t program_info;
  };

  /// @brief Constructor to construct program with an IL binary.
  ///
  /// @param[in] context Context to which the program will belong.
  /// @param[in] source IL binary the program will own.
  ur_program_handle_t_(ur_context_handle_t context,
                       cargo::dynamic_array<uint32_t> source)
      : context(context), source(std::move(source)) {
    initDevicePrograms();
  }

  /// @brief Constructor to create an empty program, for linking.
  ur_program_handle_t_(ur_context_handle_t context) : context(context) {
    initDevicePrograms();
  }

  ur_program_handle_t_(const ur_program_handle_t_ &) = delete;
  ur_program_handle_t_ &operator=(const ur_program_handle_t_ &) = delete;
  ~ur_program_handle_t_();

  /// @brief Factory method for creating programs.
  ///
  /// @param[in] context Context to which the program will belong.
  /// @param[in] il IL binary.
  /// @param[in] length Length of `il` in bytes.
  ///
  /// @return Program object or error code if something went wrong.
  static cargo::expected<ur_program_handle_t, ur_result_t> create(
      ur_context_handle_t context, const void *il, uint32_t length);

  /// @brief Factory method for creating empty programs, used in linking.
  ///
  /// @param[in] context Context to which the program will belong.
  ///
  /// @return Program object or error code if something went wrong.
  static cargo::expected<ur_program_handle_t, ur_result_t> create(
      ur_context_handle_t context);

  /// @brief Initialize a device program in `device_program_map` for each device
  /// in `context`.
  void initDevicePrograms();

  /// @brief Set `options` to the incoming value and have each device program in
  /// `device_program_map` parse the options string according to `mode`.
  ///
  /// @param[in] in_options Options string to copy and parse.
  /// @param[in] mode Compiler enum denoting how to parse `in_options`.
  ur_result_t setOptions(cargo::string_view in_options,
                         compiler::Options::Mode mode);

  /// @brief Compile the program for each device in `context`.
  ur_result_t compile();

  /// @brief Produces device binaries from the program for each device in
  /// `context`.
  ur_result_t finalize();

  /// @brief Performs the `compile` and `finalize` operations.
  ur_result_t build();

  /// @brief Links the input programs together and performs the `finalize`
  /// operation.
  ///
  /// @param[in] input_programs List of compiled programs to link together.
  ur_result_t link(cargo::array_view<const ur_program_handle_t> input_programs);

  /// @brief Retrieves kernel data struct for a given entry point name.
  ///
  /// This will search each compiled binary in the program, so it may return
  /// data relevent to any device the program has been compiled for.
  ///
  /// @param name Name of the entry point to search for.
  cargo::expected<const ur::kernel_data_t &, ur_result_t> getKernelData(
      const cargo::string_view name);

  /// @brief Context to which the program belongs.
  ur_context_handle_t context = nullptr;
  /// @brief Map from device indices in the order they appear in `context` to
  /// device specific programs.
  std::unordered_map<ur_device_handle_t, device_program_t> device_program_map;
  /// @brief The program's source IL.
  cargo::dynamic_array<uint32_t> source;
  /// @brief The options string passed to the last compile, link or build
  /// operation performed on this program.
  cargo::dynamic_array<char> options;
};

#endif  // UR_PROGRAM_H_INCLUDED
