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
/// @brief Definitions of the OpenCL program API.

#ifndef CL_PROGRAM_H_INCLUDED
#define CL_PROGRAM_H_INCLUDED

#include <CL/cl.h>
#include <cargo/expected.h>
#include <cargo/optional.h>
#include <cargo/string_view.h>
#include <cl/base.h>
#include <cl/binary/binary.h>
#include <cl/binary/kernel_info.h>
#include <cl/binary/program_info.h>
#include <cl/kernel.h>
#include <extension/config.h>

#include <unordered_map>

namespace cl {
using pfn_notify_program_t = void(CL_CALLBACK *)(cl_program program,
                                                 void *user_data);

/// @brief Enumeration to specify the creation type of an OpenCL program.
enum class program_type : uint8_t {
  NONE,     ///< An unknown program type (likely something bad happened).
  OPENCLC,  ///< A program created with clCreateProgramWithSource.
  BINARY,   ///< A program created with clCreateProgramWithBinary.
  LINK,     ///< A program created with clLinkProgram.
  SPIR,     ///< A program created with clCreateProgramWithBinary when a SPIR
            ///< binary was used.
  SPIRV,    ///< A program created with clCreateProgramWithILKHR when a SPIR-V
            ///< binary was used.
  BUILTIN,  ///< A program created with clCreateProgramWithBuiltInKernels.
};

/// @brief Enumeration to specify the work-item ordering within a work-group
/// for an OpenCL program.
enum class program_work_item_order : uint8_t {
  /// @brief Loop over X, then Y, then Z.
  xyz,
  /// @brief Loop over X, then Z, then Y.
  xzy,
  /// @brief Loop over Y, then X, then Z.
  yxz,
  /// @brief Loop over Y, then Z, then X.
  yzx,
  /// @brief Loop over Z, then X, then Y.
  zxy,
  /// @brief Loop over Z, then Y, then X.
  zyx
};

/// @brief The type of device program.
enum class device_program_type {
  NONE,            ///< The device program does not represent any program.
  BINARY,          ///< The device program stores binary program state.
  BUILTIN,         ///< The device program stores builtin kernels.
  COMPILER_MODULE  ///< The device program encapsulates a compiler module.
};

/// @brief An object which manages mux_kernel_t's, either created from a
/// mux_executable_t, or builtin kernels.
class mux_kernel_cache {
 public:
  /// @brief Creates a mux_kernel_cache object that manages built-in kernels.
  mux_kernel_cache();

  /// @brief Creates a mux_kernel_cache object that manages a Mux executable.
  mux_kernel_cache(mux::unique_ptr<mux_executable_t>);

  /// @brief Lookup cache or create a Mux kernel.
  ///
  /// @param[in] device CL device containing the Mux device and allocator to
  /// use when creating kernels.
  /// @param[in] name Name of the kernel to lookup.
  ///
  /// @return Returns the relevant Mux kernel, or the mux_result_t if there
  /// was a failure to create the kernel.
  cargo::expected<mux_kernel_t, mux_result_t> getOrCreateKernel(
      cl_device_id device, const std::string &name);

 private:
  bool use_builtin_kernels;
  mux::unique_ptr<mux_executable_t> mux_executable;

  // We need to guard against creating kernels in parallel, to avoid
  // corrupting the kernel map.
  std::mutex kernel_map_mutex;
  std::unordered_map<std::string, mux::unique_ptr<mux_kernel_t>> kernel_map;
};

/// @brief A class which encapsulates device specific program information, such
/// as a compiler module, or a device specific binary executable loaded from
/// disk.
struct device_program {
  /// @brief Binary program state.
  struct Binary {
    /// @brief Constructor.
    ///
    /// @param executable Mux executable to create kernels from.
    /// @param binary OpenCL binary that this Mux executable was loaded from.
    Binary(mux::unique_ptr<mux_executable_t> executable,
           cargo::dynamic_array<uint8_t> binary)
        : kernels(std::move(executable)), binary(std::move(binary)) {}

    /// @brief An object that manages Mux kernels created from the Mux
    /// executable created when the binary is loaded.
    mux_kernel_cache kernels;

    /// @brief Binary used to create the Mux executable. Cached so it can be
    /// returned as part of clGetProgramInfo.
    cargo::dynamic_array<uint8_t> binary;
  };

  /// @brief Builtin Kernels program state.
  struct Builtin {
    /// @brief A list of built-in kernel programs. These are the kernels
    /// requested by the OpenCL API user.
    cargo::small_vector<std::string, 8> kernel_names;

    /// @brief Built-in kernel definition strings. These are used to determine
    /// the arguments to built-in kernels. They come from the Mux device.
    cargo::small_vector<std::string, 8> kernel_decls;

    /// @brief An object that manages built-in Mux kernels.
    mux_kernel_cache kernels;
  };

  /// @brief Compiler module program state.
  struct CompilerModule {
    /// @brief Clears the state of the compiler module, ready for a new
    /// compilation process.
    void clear();

    /// @brief Compiler module. This is guaranteed to be a valid pointer if
    /// `type == device_program_type::COMPILER_MODULE`.
    std::unique_ptr<compiler::Module> module;

    /// @brief An object that manages Mux kernels created from the Mux
    /// executable created when the module is finalized and deferred compilation
    /// is not supported.
    cargo::optional<mux_kernel_cache> kernels;

    /// @brief Cached copy of an OpenCL binary. Populated lazily during
    /// binarySerialize.
    cargo::optional<cargo::dynamic_array<uint8_t>> cached_binary;

    /// @brief This function checks whether a Mux binary exists and if so,
    /// returns it. Otherwise, it creates a Mux binary from the compiler module
    /// and caches it.
    ///
    /// @return An array_view to the Mux binary, or a compiler failure if there
    /// was an error generating the binary.
    cargo::expected<cargo::array_view<const uint8_t>, compiler::Result>
    getOrCreateMuxBinary();

   private:
    /// @brief Cached copy of a module binary created by Module::createBinary.
    cargo::optional<cargo::dynamic_array<uint8_t>> cached_mux_binary;
  };

  /// @brief Default constructor.
  device_program();

  /// @brief Destructor.
  ~device_program();

  /// @brief Initialize this device program as a binary.
  ///
  /// @param executable Mux executable to create kernels from.
  /// @param binary_buffer OpenCL binary that this Mux executable was loaded
  /// from.
  void initializeAsBinary(mux::unique_ptr<mux_executable_t> executable,
                          cargo::dynamic_array<uint8_t> binary_buffer);

  /// @brief Initialize this device program as a collection of builtin kernels.
  void initializeAsBuiltin();

  /// @brief Initialize this device program as a compiler module.
  ///
  /// @param target Compiler target to create the compiler module from.
  void initializeAsCompilerModule(compiler::Target *target);

  /// @brief Report a compiler error.
  ///
  /// @param error Error string to report.
  void reportError(cargo::string_view error);

  /// @brief Checks if the device program is executable (i.e. fully built
  /// module, binary, or built-in kernels).
  ///
  /// @return Returns true if the device program is executable.
  bool isExecutable() const;

  /// @brief Finalizes the device program.
  ///
  /// @param device CL device this program is associated with.
  /// @return Returns true if finalization was successful.
  bool finalize(cl_device_id device);

  /// @brief Creates a MuxKernelWrapper from this device program.
  ///
  /// This will contain either a pre-compiled kernel or a deferred compiled
  /// kernel depending on the `type` of this device program.
  ///
  /// @param device CL device this program is associated with.
  /// @param kernel_name
  /// @return Returns a MuxKernelWrapper object, or a CL error on failure.
  cargo::expected<std::unique_ptr<MuxKernelWrapper>, cl_int> createKernel(
      cl_device_id device, const std::string &kernel_name);

  /// @brief Calculates the size of the binary representation of this device
  /// program.
  ///
  /// @return The size of the binary representation of this device program in
  /// bytes. Returns 0 bytes if an error occurred.
  size_t binarySize();

  /// @brief Serializes the binary representation of this device program and
  /// returns an array view to a cached copy of the binary.
  ///
  /// Note that this array view is valid until this device program is either
  /// cleared with `initializeAsBinary` or `initializeAsCompilerModule` or
  /// destroyed.
  ///
  /// @return An array view of binary representation of this device program in
  /// bytes. See the description for the lifetime of the data itself. Returns
  /// an empty array view if an error occurred.
  cargo::array_view<uint8_t> binarySerialize();

  /// @brief Initialises this device program from a binary representation
  /// previously created with `binarySerialize`.
  ///
  /// @param device CL device this program is associated with.
  /// @param compiler_target Compiler target to initialize the compiler module
  /// from. This is only required if the serialized binary was of type
  /// `device_program_type::COMPILER_MODULE` and the module itself had been
  /// compiled and/or linked but not finalized.
  /// @param buffer The serialized binary to read.
  ///
  /// @return Returns true if deserialization was successful, false otherwise.
  /// Any errors will be reported to the `compiler_log` member.
  bool binaryDeserialize(cl_device_id device, compiler::Target *compiler_target,
                         cargo::array_view<const uint8_t> buffer);

  /// @brief Returns the cl_program_binary_type that this device program
  /// represents.
  cl_program_binary_type getCLProgramBinaryType() const;

 private:
  /// @brief Clears the device_program state depending on the value of `type`.
  void clear();

 public:
  /// @brief Compilation options.
  std::string options;

  /// @brief Number of compiler errors.
  uint32_t num_errors;

  /// @brief Compilation log.
  std::string compiler_log;

  /// @brief Program information.
  cargo::optional<binary::ProgramInfo> program_info;

  /// @brief Printf descriptor information.
  std::vector<builtins::printf::descriptor> printf_calls;

  /// @brief Current program type.
  device_program_type type;
  union {
    Binary binary;
    Builtin builtin;
    CompilerModule compiler_module;
  };
};
}  // namespace cl

/// @addtogroup cl
/// @{

struct _cl_program final : public cl::base<_cl_program> {
  /// @brief RAII type for handling program callbacks.
  struct callback {
    /// @brief Constructor, store the callback to be invoked later.
    ///
    /// @param[in] program _cl_program object pointer.
    /// @param[in] pfn_notify Register callback function for notification.
    /// @param[in] user_data Client's user data (pass through).
    callback(cl_program program, cl::pfn_notify_program_t pfn_notify,
             void *user_data)
        : program(program), pfn_notify(pfn_notify), user_data(user_data) {}

    /// @brief  Destructor, invokes the callback function.
    ~callback() {
      if (pfn_notify != nullptr) {
        pfn_notify(program, user_data);
      }
    }

    cl_program program;
    cl::pfn_notify_program_t pfn_notify;
    void *user_data;
  };

  /// @brief OpenCL C program state.
  struct OpenCLC {
    /// @brief OpenCL C source code string.
    std::string source;
  };

  /// @brief Built in kernel program state.
  struct BuiltInKernel {
    /// @brief A string of the builtin kernels within the program.
    std::string names;
  };

  /// @brief SPIR-V program state.
  struct SPIRV {
#if defined(CL_VERSION_3_0)
    /// @brief Set a constant to be specialized during translation.
    ///
    /// @param[in] spec_id ID of the constant to be specialized.
    /// @param[in] spec_size Size in bytes of the constant.
    /// @param[in] spec_value Pointer to `spec_size` bytes of constant data.
    ///
    /// @return Returns an OpenCL error code.
    /// @retval `CL_SUCCESS` if the constant can be specialized.
    /// @retval `CL_INVALID_SPEC_ID` if `spec_id` is not specializable.
    /// @retval `CL_INVALID_VALUE` if `spec_size` is not correct.
    /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failed.
    cl_int setSpecConstant(cl_uint spec_id, size_t spec_size,
                           const void *spec_value);
#endif

    /// @brief Get information to specialize a SPIR-V module's constants.
    ///
    /// @return Optionally returns the specialization information if present.
    cargo::optional<const compiler::spirv::SpecializationInfo &> getSpecInfo();

    /// @brief IL program input (probably SPIR-V) binary.
    cargo::dynamic_array<uint32_t> code;
#if defined(CL_VERSION_3_0)
    /// @brief Map of specialization constants which can be specialized.
    compiler::spirv::SpecializableConstantsMap specializable;

   private:
    /// @brief Specialization constant value data.
    cargo::small_vector<uint8_t, 32> specData;
    /// @brief Constant specialization information.
    compiler::spirv::SpecializationInfo specInfo;
#endif
  };

 private:
  /// @brief Private constructor, use _cl_program::create() instead.
  _cl_program(cl_context context);

  _cl_program(const _cl_program &) = delete;
  _cl_program(_cl_program &&) = delete;

 public:
  /// @brief Destructor.
  ~_cl_program();

  /// @brief Create program with source.
  ///
  /// @param[in] context OpenCL context to create the program within.
  /// @param[in] count Number of items in the `strings` and `lengths` arrays.
  /// @param[in] strings Array of source code strings.
  /// @param[in] lengths Array of string size in bytes of each string in
  /// `strings`.
  ///
  /// @return Returns a program object on success, an OpenCL error on failure.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurred.
  static cargo::expected<std::unique_ptr<_cl_program>, cl_int> create(
      cl_context context, cl_uint count, const char *const *strings,
      const size_t *lengths);

  /// @brief Create program with intermediate language.
  ///
  /// @param[in] context OpenCL context to create the program within.
  /// @param[in] il Intermediate language binary data.
  /// @param[in] length Length in bytes of `il`.
  ///
  /// @return Returns a program object on success, an OpenCL error on failure.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurred.
  /// @retval `CL_INVALID_VALUE` if the SPIR-V module is invalid.
  static cargo::expected<std::unique_ptr<_cl_program>, cl_int> create(
      cl_context context, const void *il, size_t length);

  /// @brief Create program with binary.
  ///
  /// @param[in] context OpenCL context to create the program within.
  /// @param[in] num_devices Number of items in the `device_list`, `lengths`,
  /// `binaries`, and `binary_status` arrays.
  /// @param[in] device_list Array of devices to create the program for.
  /// @param[in] lengths Array of binary size in bytes of each binary in
  /// `binaries`.
  /// @param[in] binaries Array of program binaries.
  /// @param[out] binary_status Array of binary status to be set for each
  /// binary in `binaries`.
  ///
  /// @return Returns a program object on success, an OpenCL error on failure.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurred.
  static cargo::expected<std::unique_ptr<_cl_program>, cl_int> create(
      cl_context context, cl_uint num_devices, const cl_device_id *device_list,
      const size_t *lengths, const unsigned char *const *binaries,
      cl_int *binary_status);

  /// @brief Create program with built-in kernels.
  ///
  /// @param[in] context OpenCL context to create the program within.
  /// @param[in] num_devices Number of devices in `device_list`.
  /// @param[in] device_list Array of devices to create the program for.
  /// @param[in] kernel_names Semi-colon separated list of kernel names.
  ///
  /// @return Returns a program object on success, an OpenCL error on failure.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurred.
  /// @retval `CL_INVALID_VALUE` if a kernel name is not valid.
  static cargo::expected<std::unique_ptr<_cl_program>, cl_int> create(
      cl_context context, cl_uint num_devices, const cl_device_id *device_list,
      const char *kernel_names);

  /// @brief Create program by linking multiple existing programs.
  ///
  /// @param[in] context OpenCL context to create the program within.
  /// @param[in] devices List of devices to create the program for.
  /// @param[in] options Linker options to use when creating the program.
  /// @param[in] input_programs List of programs to be linked together.
  ///
  /// @return Returns a program object on success, an OpenCL error on failure.
  /// @retval `CL_INVALID_OPERATION` 1.) If the compilation or build of a
  /// program executable for any of the devices listed in device_list by a
  /// previous call to clCompileProgram() or clBuildProgram() for `program` has
  /// not completed. 2.) If the rules for devices containing compiled binaries
  /// or libraries as described in input_programs argument above are not
  /// followed.
  /// @retval `CL_LINKER_NOT_AVAILABLE` if a linker is not available i.e.
  /// `CL_DEVICE_LINKER_AVAILABLE` specified in the table of allowed values for
  /// `param_name` for clGetDeviceInfo() is set to `CL_FALSE`.
  /// @retval `CL_LINK_PROGRAM_FAILURE` if there is a failure to link the
  /// compiled binaries and/or libraries.
  static cargo::expected<std::unique_ptr<_cl_program>, cl_int> create(
      cl_context context, cargo::array_view<const cl_device_id> devices,
      cargo::string_view options,
      cargo::array_view<const cl_program> input_programs);

  /// @brief Compile the program for each device.
  ///
  /// @param[in] devices List of devices to compile the program for.
  /// @param[in] input_headers List of input headers to be included.
  ///
  /// @return Return an OpenCL error code.
  /// @retval `CL_SUCCESS` when compilation was successful.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failed.
  /// @retval `CL_INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `CL_COMPILE_PROGRAM_FAILURE` when compilation failed.
  cl_int compile(cargo::array_view<const cl_device_id> devices,
                 cargo::array_view<compiler::InputHeader> input_headers);

  /// @brief Link the program for each device.
  ///
  /// @param[in] devices Devices to link the program for.
  /// @param[in] input_programs List of compiled binaries or libraries to link.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` when linking was successful.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failed.
  /// @retval `CL_INVALID_LINKER_OPTIONS` when invalid options were set.
  /// @retval `CL_LINK_PROGRAM_FAILURE` when linking failed.
  cl_int link(cargo::array_view<const cl_device_id> devices,
              cargo::array_view<const cl_program> input_programs);

  /// @brief Finalize the program and create an executable for each device.
  ///
  /// @param[in] devices List of device to finalize the program for.
  ///
  /// @return Return true on success, false on failure.
  bool finalize(cargo::array_view<const cl_device_id> devices);

  /// @brief Query the program for a named kernel.
  ///
  /// @param[in] name Name of the kernel to query.
  ///
  /// @return Returns the kernel description if there is one.
  cargo::optional<const cl::binary::KernelInfo *> getKernelInfo(
      cargo::string_view name) const;

  /// @brief Query the program for the number of kernels it contains.
  ///
  /// @return Return the number of kernels in the program.
  size_t getNumKernels() const;

  /// @brief Query the program for the kernel name at index.
  ///
  /// @param[in] kernel_index Index of the kernel to get the name of.
  ///
  /// @return Return string containing the name of the kernel.
  const char *getKernelNameByOffset(const size_t kernel_index) const;

  /// @brief Query the program to determine if it targets the device.
  ///
  /// @param[in] device Device to query the program with.
  ///
  /// @return Return true if the program contains the device, false otherwise.
  bool hasDevice(const cl_device_id device) const;

  /// @brief Query the program to determine if an option was set.
  ///
  /// @param[in] device Device to query if an option was set.
  /// @param[in] option Option to query the program with.
  ///
  /// @return Return true if the option was set, false otherwise.
  bool hasOption(cl_device_id device, const char *option);

  /// @brief Sets build flags on the compiler binary based on options string.
  ///
  /// @param[in] devices List of device to set options for.
  /// @param[in] options String of options to set on the program.
  /// @param[in] mode Option parsing mode, either
  /// `_cl_program::option_mode::COMPILE` when invoked from `clBuildProgram` and
  /// `clCompileProgram` or `_cl_program::option_mode::LINK` when invoked from
  /// `clLinkProgram`.
  ///
  /// @return Return an OpenCL error code.
  /// @retval `CL_SUCCESS` when options are set.
  /// @retval `CL_INVALID_COMPILER_OPTIONS` when invoked with `mode =
  /// _cl_program::option_mode::COMPILE` and an invalid option was set.
  /// @retval `CL_INVALID_LINKER_OPTIONS` when invoked with `mode =
  /// _cl_program::option_mode::LINK` and an invalid option was set.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurs.
  cl_int setOptions(cargo::array_view<const cl_device_id> devices,
                    cargo::string_view options,
                    const compiler::Options::Mode mode);

  /// @brief Context which the program belongs to.
  cl_context context;

  /// @brief Map OpenCL devices to program binaries.
  std::unordered_map<cl_device_id, cl::device_program> programs;

  /// @brief Program state retained for use in OpenCL entry points.
  ///
  /// Access of these union members **must** always occur *after* checking
  /// _cl_program::type is set to the associated value:
  ///
  /// * To access _cl_program::openclc, _cl_program::type must be
  ///   cl::program_type::OPENCLC.
  /// * To access _cl_program::builtInKernel, _cl_program::type must be
  ///   cl::program_type::BUILTIN.
  /// * To access _cl_program::spirv, _cl_program::type must be
  ///   cl::program_type::SPIRV.
  // TODO(CA-652): Ideally this should be replaced by a variant to abstract
  // away the sharp corners of using a union in this way but we don't currently
  // have one available to use.
  union {
    OpenCLC openclc;
    BuiltInKernel builtInKernel;
    SPIRV spirv;
  };

  /// @brief Atomic count of the number of retained kernel objects.
  ///
  /// If a single kernel is retained multiple times then this counter gets
  /// incremented multiple times (and decremented when the kernels are
  /// released).  This counter only exists so that if clBuildProgram() or
  /// clCompileProgram() are called on a cl_program that has already been
  /// built/compiled and still has attached kernels then an error code can be
  /// returned (as expected by the OpenCL specification).
  ///
  /// @note This number only relates to externally retained kernels, not
  /// internally retained ones.  Thus the count may reach zero while some
  /// kernels still exist internally in the runtime.
  std::atomic<int> num_external_kernels;

  /// @brief The type of the program
  cl::program_type type;

#ifdef OCL_EXTENSION_cl_codeplay_wfv
  /// @brief The work-item ordering of the program.
  std::unordered_map<cl_device_id, cl::program_work_item_order> work_item_order;
#endif

#if defined(CL_VERSION_3_0)
  /// @brief Program object contains non-trivial constructor(s).
  const cl_bool scope_global_ctors_present = CL_FALSE;
  /// @brief Program object contains non-trivial destructor(s).
  const cl_bool scope_global_dtors_present = CL_FALSE;
  /// @brief Total storage in bytes used by program variables in the global
  /// address space.
  const size_t global_variable_total_size = 0;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL program object from OpenCL C source.
///
/// @param[in] context Context the program object belongs to.
/// @param[in] count Number of strings in `strings`.
/// @param[in] strings List of strings.
/// @param[in] lengths List of lengths of each string in `strings`.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new program object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithSource.html
CL_API_ENTRY cl_program CL_API_CALL CreateProgramWithSource(
    cl_context context, cl_uint count, const char *const *strings,
    const size_t *lengths, cl_int *errcode_ret);

/// @brief Create an OpenCL program object from a binary.
///
/// @param[in] context Context the program object belongs to.
/// @param[in] num_devices Number of devices in `device_list` to target.
/// @param[in] device_list List of devices to target.
/// @param[in] lengths List of lengths of binary buffers in `device_list`.
/// @param[out] binaries Return list of binaries, must not be null.
/// @param[out] binary_status Return list of statuses if not null.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new program object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithBinary.html
CL_API_ENTRY cl_program CL_API_CALL CreateProgramWithBinary(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const size_t *lengths, const unsigned char *const *binaries,
    cl_int *binary_status, cl_int *errcode_ret);

/// @brief Create an OpenCL program object with built-in kernels.
///
/// @param context Context the program object belongs to.
/// @param num_devices Number of devices in `device_list` to target.
/// @param device_list List of devices to target.
/// @param kernel_names Semi-colon separated string of kernel names.
/// @param errcode_ret Return error code if not null.
///
/// @return Return the new program object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithBuiltInKernels.html
CL_API_ENTRY cl_program CL_API_CALL CreateProgramWithBuiltInKernels(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *kernel_names, cl_int *errcode_ret);

/// @brief Increment the program objects reference count.
///
/// @param program Program to increment the reference count of.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainProgram.html
CL_API_ENTRY cl_int CL_API_CALL RetainProgram(cl_program program);

/// @brief Decrement the program object reference count.
///
/// @param program Program to decrement the reference county of.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseProgram.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseProgram(cl_program program);

/// @brief Compile the program object.
///
/// @param program Progrem to compile
/// @param num_devices Number of devices in `device_list` to target.
/// @param device_list List of devices to target.
/// @param options String of compiler options.
/// @param num_input_headers Number of headers in `input_headers`.
/// @param input_headers List of input headers.
/// @param header_include_names List of include header names.
/// @param pfn_notify Build notification callback.
/// @param user_data User data to be passed to `pfn_notify`.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCompileProgram.html
CL_API_ENTRY cl_int CL_API_CALL CompileProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_headers,
    const cl_program *input_headers, const char *const *header_include_names,
    cl::pfn_notify_program_t pfn_notify, void *user_data);

/// @brief Link the program object.
///
/// @param context Context the linked program belongs to.
/// @param num_devices Number of devices in `device_list`.
/// @param device_list List of devices to target.
/// @param options String of linker options.
/// @param num_input_programs Number of input programs in `input_programs`.
/// @param input_programs List of input programs to link.
/// @param pfn_notify Link notification callback.
/// @param user_data User data to be passed to `pfn_notify`.
/// @param errcode_ret Return error code if not null.
///
/// @return Return the new linked program object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clLinkProgram.html
CL_API_ENTRY cl_program CL_API_CALL LinkProgram(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_programs,
    const cl_program *input_programs, cl::pfn_notify_program_t pfn_notify,
    void *user_data, cl_int *errcode_ret);

/// @brief Build, or compile and link, the program object.
///
/// @param program Program object to build.
/// @param num_devices Number of devices in `devices_list`.
/// @param device_list List of devices to target.
/// @param options String of build options.
/// @param pfn_notify Build notification callback.
/// @param user_data User data to be passed to `pfn_notify`.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clBuildProgram.html
CL_API_ENTRY cl_int CL_API_CALL BuildProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl::pfn_notify_program_t pfn_notify, void *user_data);

/// @brief Query the program object form information.
///
/// @param program Program object to query for information.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of `param_value` storage.
/// @param param_value Pointer to value to store information in.
/// @param param_value_size_ret Return size in bytes required for
/// `param_value`.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetProgramBuildInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetProgramInfo(cl_program program,
                                               cl_program_info param_name,
                                               size_t param_value_size,
                                               void *param_value,
                                               size_t *param_value_size_ret);

/// @brief Query the program for the latest build information.
///
/// @param program Program object to query for build information.
/// @param device Device the build information pertains to.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of `param_value` storage.
/// @param param_value Pointer to value to store query in.
/// @param param_value_size_ret Return size in bytes required for
/// `param_value`.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetProgramBuildInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetProgramBuildInfo(
    cl_program program, cl_device_id device, cl_program_build_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret);

/// @brief Unload the compiler.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/clUnloadCompiler.html
CL_API_ENTRY cl_int CL_API_CALL UnloadCompiler();

/// @brief Unload the platform compiler.
///
/// The function allows the implementation to release the resources allocated
/// by the OpenCL compiler for platform. This is a hint from the application
/// and does not guarantee that the compiler will not be used in the future or
/// that the compiler will actually be unloaded by the implementation. Calls to
/// `clBuildProgram`, `clCompileProgram` or `clLinkProgram` after
/// `clUnloadPlatformCompiler` will reload the compiler, if necessary, to build
/// the appropriate program executable.
///
/// @param platform Platform to unload the compiler from.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clUnloadPlatformCompiler.html
CL_API_ENTRY cl_int CL_API_CALL UnloadPlatformCompiler(cl_platform_id platform);

/// @}
}  // namespace cl

#endif  // CL_PROGRAM_H_INCLUDED
