// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler program module API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_MODULE_H_INCLUDED
#define COMPILER_MODULE_H_INCLUDED

#include <builtins/printf.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <compiler/kernel.h>
#include <compiler/result.h>
#include <mux/mux.hpp>
#include <spirv/unified1/spirv.hpp>

#include <array>
#include <unordered_map>

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief OpenCL C standard to target.
enum class Standard {
  /// @brief Target OpenCL C 1.1
  OpenCLC11,
  /// @brief Target OpenCL C 1.2
  OpenCLC12,
  /// @brief Target OpenCL C 3.0
  OpenCLC30,
};

namespace SnapshotStage {
/// @brief The snapshot is taken at the same stage as the binary normally
///        returned by clGetProgramInfo.
constexpr const char COMPILE_DEFAULT[] = "cl_snapshot_compilation_default";
/// @brief The snapshot is taken out of the front-end compilation stage.
constexpr const char COMPILE_FRONTEND[] = "cl_snapshot_compilation_front_end";
/// @brief The snapshot is taken after the linking stage(clLinkProgram).
constexpr const char COMPILE_LINKING[] = "cl_snapshot_compilation_linking";
/// @brief The snapshot is taken after SIMD preparation passes.
constexpr const char COMPILE_SIMD_PREP[] =
    "cl_snapshot_compilation_simd_prepare";
/// @brief The snapshot is taken after CFG scalarization.
constexpr const char COMPILE_SCALARIZED[] =
    "cl_snapshot_compilation_scalarized";
/// @brief The snapshot is taken after CFG linearization.
constexpr const char COMPILE_LINEARIZED[] =
    "cl_snapshot_compilation_linearized";
/// @brief The snapshot is taken after any SIMD packetization.
constexpr const char COMPILE_SIMD_PACKETIZED[] =
    "cl_snapshot_compilation_simd_packetized";
/// @brief The snapshot is taken before SPIR is turned into device IR
constexpr const char COMPILE_SPIR[] = "cl_snapshot_compilation_spir";
/// @brief The snapshot is taken after OpenCL builtins are materialized.
constexpr const char COMPILE_BUILTINS[] =
    "cl_snapshot_compilation_builtins_materialized";
}  // namespace SnapshotStage

/// @brief Output formats supported by snapshots
enum class SnapshotFormat {
  DEFAULT = 0,
  TEXT,
  BINARY,
};

/// @brief Early vectorization mode to apply.
enum class PreVectorizationMode {
  NONE,
  LOOP,
  SLP,
  ALL,
  DEFAULT = NONE,
};

/// @brief Vectorization mode to apply.
enum class VectorizationMode {
  NEVER,
  ALWAYS,
  AUTO,
  DEFAULT = AUTO,
};

enum class WorkItemOrder {
  XYZ,
  XZY,
  YXZ,
  YZX,
  ZXY,
  ZYX,
  INVALID,
  DEFAULT = XYZ,
};

/// @brief Module state.
enum class ModuleState : uint8_t {
  NONE = 0,
  COMPILED_OBJECT = 1,
  LIBRARY = 2,
  INTERMEDIATE = 3,
  EXECUTABLE = 4
};

/// @brief Options to be passed to the compiler.
struct Options {
  Options()
      : standard(Standard::OpenCLC12),
        fp32_correctly_rounded_divide_sqrt(false),
        mad_enable(false),
        no_signed_zeros(false),
        unsafe_math_optimizations(false),
        denorms_may_be_zero(false),
        finite_math_only(false),
        warn_ignore(false),
        warn_error(false),
        kernel_arg_info(false),
        debug_info(false),
        opt_disable(false),
        fast_math(false),
        soft_math(false),
        scalable_vectors(false),
        prevec_mode(PreVectorizationMode::DEFAULT),
        vectorization_mode(VectorizationMode::DEFAULT),
        llvm_stats(false),
        single_precision_constant(false) {}

  /// @brief List of preprocessor macro definition.
  std::vector<std::string> definitions;
  /// @brief List of enabled runtime extensions.
  std::vector<std::string> runtime_extensions;
  /// @brief List of enabled compiler extensions.
  std::vector<std::string> compiler_extensions;
  /// @brief List of include directory search paths.
  std::vector<std::string> include_dirs;
  /// @brief Semi-colon separated list of device specific options
  ///
  /// Each option is a comma separated pair of `argument,value` where the
  /// value is optional if the argument is a flag. Should match syntax of
  /// `mux_executable_options_s::device_options`.
  std::string device_args;
  /// @brief OpenCL standard to target.
  Standard standard;
  // @brief Whether fp32 divide and sqrt must be correctly rounded.
  bool fp32_correctly_rounded_divide_sqrt;
  /// @brief Enable less precise floating point math.
  bool mad_enable;
  /// @brief Allow ignoring the sign of floating point zeroes.
  bool no_signed_zeros;
  /// @brief Enable unsafe floating point math.
  bool unsafe_math_optimizations;
  /// @brief Denormal floating point numbers may be Flushed To Zero.
  bool denorms_may_be_zero;
  /// @brief Allow assuming that floating point results are finite.
  bool finite_math_only;
  /// @brief Ignore all warnings.
  bool warn_ignore;
  /// @brief Treat all warning as errors.
  bool warn_error;
  /// @brief Emit OpenCL kernel argument meta data.
  bool kernel_arg_info;
  /// @brief Enable emitting debug info.
  bool debug_info;
  /// @brief Disable all optimizations.
  bool opt_disable;
  /// @brief Enable Fast Math mode.
  bool fast_math;
  /// @brief Enable Soft Math mode.
  bool soft_math;
  /// @brief Allow the generation of scalable vectors.
  bool scalable_vectors;
  /// @brief Early Vectorization mode (loop/SLP vectorization).
  PreVectorizationMode prevec_mode;
  /// @brief Enable Vectorization mode.
  VectorizationMode vectorization_mode;
  /// @brief Enable LLVM stats reporting
  bool llvm_stats;
  /// @brief Path to kernel source code to write out.
  std::string source_file;
  /// @brief Path to kernel source code that was read in.
  std::string source_file_in;
  /// @brief Treat double constants as single-precision constants
  bool single_precision_constant;
  /// @brief List of local sizes that kernel compilation pre-caching has been
  /// requested for.
  ///
  /// We treat these sizes the same way we treat the size a user can specify
  /// with the `reqd_work_group_size` function attribute, in that enqueuing a
  /// kernel built with one of the local sizes in this list will be quicker.
  std::vector<std::array<size_t, 3>> precache_local_sizes;

  /// @brief Enumeration of option parsing modes.
  enum class Mode {
    BUILD,
    COMPILE,
    LINK,
  };
};

/// @brief An input header for program compilation.
struct InputHeader {
  /// @brief The header source code.
  cargo::string_view source;
  /// @brief The include name of the header.
  cargo::string_view name;
};

/// @brief Snapshot callback handler.
typedef void (*compiler_snapshot_callback_t)(size_t snapshot_size,
                                             const char *snapshot_data,
                                             void *callback_data,
                                             void *user_data);

/// @brief Argument types for serialization
enum class ArgumentKind : uint32_t {
  UNKNOWN,
  POINTER,
  INT1,
  INT1_2,
  INT1_3,
  INT1_4,
  INT1_8,
  INT1_16,
  INT8,
  INT8_2,
  INT8_3,
  INT8_4,
  INT8_8,
  INT8_16,
  INT16,
  INT16_2,
  INT16_3,
  INT16_4,
  INT16_8,
  INT16_16,
  INT32,
  INT32_2,
  INT32_3,
  INT32_4,
  INT32_8,
  INT32_16,
  INT64,
  INT64_2,
  INT64_3,
  INT64_4,
  INT64_8,
  INT64_16,
  HALF,
  HALF_2,
  HALF_3,
  HALF_4,
  HALF_8,
  HALF_16,
  FLOAT,
  FLOAT_2,
  FLOAT_3,
  FLOAT_4,
  FLOAT_8,
  FLOAT_16,
  DOUBLE,
  DOUBLE_2,
  DOUBLE_3,
  DOUBLE_4,
  DOUBLE_8,
  DOUBLE_16,
  STRUCTBYVAL,
  IMAGE2D,
  IMAGE3D,
  IMAGE2D_ARRAY,
  IMAGE1D,
  IMAGE1D_ARRAY,
  IMAGE1D_BUFFER,
  SAMPLER,
};

/// @brief Enumation of standard address space values. The values correspond to
/// LLVM address space values. We cannot use an enum class here, as LLVM may
/// emit non-standard address spaces.
enum AddressSpace : uint32_t {
  PRIVATE = 0,
  GLOBAL = 1,
  CONSTANT = 2,
  LOCAL = 3,
};

/// @brief Kernel argument access specifier.
enum class KernelArgAccess { NONE, READ_ONLY, WRITE_ONLY, READ_WRITE };

/// @brief Kernel argument type qualifier.
struct KernelArgType {
  enum Enum {
    NONE = 0,
    CONST = (1 << 0),
    RESTRICT = (1 << 1),
    VOLATILE = (1 << 2)
  };
};

/// @brief Struct to hold type and related metadata.
struct ArgumentType {
  /// @brief Default constructor.
  ArgumentType()
      : kind(ArgumentKind::UNKNOWN),
        address_space(0),
        vector_width(1),
        dereferenceable_bytes() {}

  /// @brief Non-pointer constructor.
  ///
  /// @param kind The argument kind.
  ArgumentType(ArgumentKind kind)
      : kind(kind),
        address_space(0),
        vector_width(1),
        dereferenceable_bytes() {}

  /// @brief Pointer constructor.
  ///
  /// @param address_space The address space of the pointer argument.
  ArgumentType(uint32_t address_space)
      : kind(ArgumentKind::POINTER),
        address_space(address_space),
        vector_width(1),
        dereferenceable_bytes() {}

  ArgumentType(uint32_t address_space, uint64_t dereferenceable_bytes)
      : kind(ArgumentKind::POINTER),
        address_space(address_space),
        vector_width(1),
        dereferenceable_bytes(dereferenceable_bytes) {}

  /// @brief The type (possibly including integer bit width and vector
  /// width).
  ArgumentKind kind;
  /// @brief The address space of the argument.
  uint32_t address_space;
  /// @brief Vector width of type (only relevant to integer and fp types).
  size_t vector_width;
  /// @brief returns the amount of deferencable bytes of the argument. Note: The
  /// argument must be a pointer type
  cargo::optional<uint64_t> dereferenceable_bytes;
};

/// @brief Struct for storing kernel info.
struct KernelInfo {
  /// @brief Struct representing basic kernel argument information.
  struct ArgumentInfo {
    AddressSpace address_qual;
    KernelArgAccess access_qual;
    std::uint32_t type_qual;
    std::string type_name;
    std::string name;
  };

  std::string name;
  std::string attributes;
  cargo::dynamic_array<ArgumentType> argument_types;
  cargo::optional<cargo::small_vector<ArgumentInfo, 8>> argument_info;
  std::uint64_t private_mem_size;

  /// @brief Values of reqd_work_group_size attribute if it exists.
  cargo::optional<std::array<size_t, 3>> work_group;
};  // class KernelInfo

/// @brief Kernel info callback type.
using KernelInfoCallback = std::function<void(KernelInfo)>;

namespace spirv {
/// @brief Information about the target device to used during SPIR-V
/// translation.
struct DeviceInfo {
  /// @brief List of supported capabilities.
  cargo::small_vector<spv::Capability, 64> capabilities;
  /// @brief List of supported extensions.
  cargo::small_vector<std::string, 8> extensions;
  /// @brief List of supported extended instruction sets.
  cargo::small_vector<std::string, 2> ext_inst_imports;
  /// @brief Supported addressing model.
  spv::AddressingModel addressing_model;
  /// @brief Supported memory model.
  spv::MemoryModel memory_model;
  /// @brief Size of a device memory address in bits (Vulkan only).
  uint32_t address_bits;
};

/// @brief Information about SPIR-V constants to be specialized.
struct SpecializationInfo {
  /// @brief A specialization constant mapping to `data`.
  struct Entry {
    /// @brief Offset in bytes into `data`.
    uint32_t offset;
    /// @brief Size of the type pointed to at `offset` into `data`.
    size_t size;
  };

  /// @brief Map of ID to offset to `data`.
  std::unordered_map<spv::Id, Entry> entries;
  /// @brief Buffer containing constant values to specialize.
  const void *data;
};

/// @brief Struct describing a descriptor binding.
struct DescriptorBinding {
  /// @brief Descriptor set number.
  uint32_t set;
  /// @brief Binding number within `set`.
  uint32_t binding;

  /// @brief Less than comparison operator to enable sort by binding.
  bool operator<(const DescriptorBinding &other) const {
    return set < other.set || (set == other.set && binding < other.binding);
  }
};

/// @brief Information about a SPIR-V module after compilation.
struct ModuleInfo {
  /// @brief List of used descriptor bindings.
  std::vector<DescriptorBinding> used_descriptor_bindings;
  /// @brief Work group size.
  std::array<uint32_t, 3> workgroup_size;
};
}  // namespace spirv

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class Module {
 public:
  /// @brief Virtual destructor.
  virtual ~Module() {}

  /// @brief Clear out the stored data.
  virtual void clear() = 0;

  /// @brief Get a reference to the compiler options that will be used by this
  /// module.
  virtual Options &getOptions() = 0;

  /// @brief Get a reference to the compiler options that will be used by this
  /// module.
  virtual const Options &getOptions() const = 0;

  /// @brief Populate the Module's options from a given string.
  ///
  /// @param[in] input_options The string of options to parse.
  /// @param[in] mode Determines the OpenCL error code to return in case options
  /// are invalid
  ///
  /// @return Returns a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_BUILD_OPTIONS` when invalid options were set and
  /// `mode` is `compiler::Options::Mode::BUILD`.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set
  /// and `mode` is `compiler::Options::Mode::COMPILE`.
  /// @retval `Result::INVALID_LINK_OPTIONS` when invalid options were set and
  /// `mode` is `compiler::Options::Mode::LINK`.
  virtual Result parseOptions(cargo::string_view input_options,
                              compiler::Options::Mode mode) = 0;

  /// @brief Loads a SPIR program.
  ///
  /// @param[in] buffer Serialized SPIR binary to parse.
  ///
  /// @return True if loading the SPIR module was successful, false otherwise.
  virtual bool loadSPIR(cargo::array_view<const std::uint8_t> buffer) = 0;

  /// @brief Compiles a previously loaded SPIR program.
  ///
  /// @param[out] output_options The compilation options parsed from SPIR
  /// metadata will be output here.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` if `compileSPIR` was called
  /// before `loadSPIR`.
  virtual Result compileSPIR(std::string &output_options) = 0;

  /// @brief Compiles a SPIR-V program.
  ///
  /// @param[in] buffer View of the SPIR-V binary stream memory.
  /// @param[in] spirv_device_info Target device information.
  /// @param[in] spirv_spec_info Information about constants to be specialized.
  ///
  /// @return Returns either a SPIR-V module info object on success, or a status
  /// code otherwise.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::BUILD_PROGRAM_FAILURE` if compilation failed and `mode`
  /// is `compiler::Options::Mode::BUILD`.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` if compilation failed and `mode`
  /// is `compiler::Options::Mode::COMPILE`.
  virtual cargo::expected<spirv::ModuleInfo, Result> compileSPIRV(
      cargo::array_view<const std::uint32_t> buffer,
      const spirv::DeviceInfo &spirv_device_info,
      cargo::optional<const spirv::SpecializationInfo &> spirv_spec_info) = 0;

  /// @brief Compile an OpenCL C program.
  ///
  /// @param[in] device_profile Device profile string. Should be either
  /// FULL_PROFILE or EMBEDDED_PROFILE.
  /// @param[in] source OpenCL C source code string.
  /// @param[in] input_headers List of headers to be included.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` when compilation failed.
  virtual Result compileOpenCLC(
      cargo::string_view device_profile, cargo::string_view source,
      cargo::array_view<compiler::InputHeader> input_headers) = 0;

  /// @brief Link a set of program binaries together into the current program.
  ///
  /// @param[in] input_modules List of input modules to link.
  ///
  /// @return Returns a status code.
  /// @retval `Result::SUCCESS` when linking was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_LINKER_OPTIONS` when invalid options were set.
  /// @retval `Result::LINK_PROGRAM_FAILURE` when linking failed.
  virtual Result link(cargo::array_view<Module *> input_modules) = 0;

  /// @brief Generates a binary from the current program.
  ///
  /// @param[out] kernel_info_callback Kernel info callback.
  /// @param[out] printf_calls Output printf descriptor list.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when finalization was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FINALIZE_PROGRAM_FAILURE` when finalization failed. See
  /// the error log for more information.
  virtual Result finalize(
      KernelInfoCallback kernel_info_callback,
      std::vector<builtins::printf::descriptor> &printf_calls) = 0;

  /// @brief Creates a binary from the current module. This assumes that the
  /// module has been finalized.
  ///
  /// @param[out] buffer Output binary buffer.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when creating the binary was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FINALIZE_PROGRAM_FAILURE` if the module is not yet
  /// finalized.
  virtual Result createBinary(cargo::array_view<std::uint8_t> &buffer) = 0;

  /// @brief Returns an object that represents a kernel contained within this
  /// module.
  ///
  /// @param[in] name The name of the kernel.
  ///
  /// @return An object that represents a kernel contained within this module.
  /// The lifetime of the `Kernel` object will be managed by `Module`.
  virtual Kernel *getKernel(const std::string &name) = 0;

  /// @brief Compute the size of the serialized module.
  ///
  /// @return The size of the module (in bytes).
  virtual std::size_t size() = 0;

  /// @brief Serialize the module.
  ///
  /// @param[in] output_buffer The buffer to write the serialized LLVM module
  /// to. The buffer must be at least size() bytes.
  ///
  /// @return The number of bytes written to the output buffer (in bytes).
  virtual std::size_t serialize(std::uint8_t *output_buffer) = 0;

  /// @brief Deserialize a serialized module.
  ///
  /// @param[in] buffer Serialized module to parse.
  ///
  /// @return A boolean indicating whether deserialization was successful.
  virtual bool deserialize(cargo::array_view<const std::uint8_t> buffer) = 0;

  /// @brief Enables a snapshot callback to be triggered when a compilation
  /// stage is reached.
  ///
  /// @param[in] stage Snapshot stage to trigger.
  /// @param[in] callback Snapshot callback.
  /// @param[in] user_data Snapshot callback user data.
  /// @param[in] format Snapshot format to use. Defaults to
  /// `SnapshotFormat::DEFAULT`.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when snapshot successfully set.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if an invalid argument was passed.
  virtual Result setSnapshotCallback(
      const char *stage, compiler_snapshot_callback_t callback, void *user_data,
      SnapshotFormat format = SnapshotFormat::DEFAULT) = 0;

  /// @brief Returns the current state of the compiler module.
  virtual ModuleState getState() const = 0;
};  // class Module

/// @}
}  // namespace compiler
#endif  // COMPILER_MODULE_H_INCLUDED
