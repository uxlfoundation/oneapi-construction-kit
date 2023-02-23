// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Definition for the OpenCL device API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_DEVICE_H_INCLUDED
#define CL_DEVICE_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <cargo/fixed_vector.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/base.h>
#include <cl/config.h>
#include <cl/limits.h>
#include <compiler/info.h>
#include <mux/mux.h>

#include <string>

/// @addtogroup cl
/// @{

/// @brief Definition of the OpenCL device object.
struct _cl_device_id final : public cl::base<_cl_device_id> {
  /// @brief Device constructor.
  ///
  /// @param platform Platform the device belongs to.
  /// @param mux_allocator Allocators be used for mux.
  /// @param mux_device Associated mux device.
  _cl_device_id(cl_platform_id platform, mux_allocator_info_t mux_allocator,
                mux_device_t mux_device);

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  _cl_device_id(_cl_device_id &&) = delete;

  /// @brief Destructor.
  ~_cl_device_id();

  /// @brief Platform the device belongs to.
  cl_platform_id platform;
  /// @brief Mux allocator info.
  mux_allocator_info_t mux_allocator;
  /// @brief Associated mux device.
  mux_device_t mux_device;
  /// @brief Associated compiler.
  const compiler::Info *compiler_info;
  /// @brief Device version string.
  std::string version;

  // Device properties:
  // https://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceInfo.html
  //
  /// @brief Size of the default device's address space, 32 or 64.
  cl_uint address_bits;
  /// @brief CL_TRUE if the device is available and CL_FALSE otherwise.
  cl_bool available;
  /// @brief CL_TRUE if the implementation has a compiler available. CL_FALSE
  /// otherwise, can be false only for the embedded platform profile.
  cl_bool compiler_available;
  /// @brief Describes the double precision floating point capabilities in a
  /// bit-field with the possibles values: CL_FP_{DENORM, INF_NAN,
  /// ROUND_TO_NEAREST, ROUND_TO_ZERO, ROUND_TO_INF, FMA, SOFT_FLOAT}. 0 if
  /// double precision is not supported, if it is, the minimum is: CL_FP_FMA |
  /// CL_FP_ROUND_TO_NEAREST | CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF |
  /// CL_FP_INF_NAN | CL_FP_DENORM.
  cl_device_fp_config double_fp_config;
  /// @brief CL_TRUE for a little endian device, CL_FALSE otherwise.
  cl_bool endian_little;
  /// @brief CL_TRUE if the device implements error correction for accesses to
  /// host memory. CL_FALSE otherwise.
  cl_bool error_correction_support;
  /// @brief CL_EXEC_KERNEL and optionally CL_EXEC_NATIVE_KERNEL if the device
  /// can execute native kernels.
  cl_device_exec_capabilities execution_capabilities;
  /// @brief Size of global memory cache in bytes.
  cl_ulong global_mem_cache_size;
  /// @brief Type of global memory (CL_NONE, CL_READ_ONLY_CACHE,
  /// CL_READ_WRITE_CACHE).
  cl_device_mem_cache_type global_mem_cache_type;
  /// @brief Size of global memory cache line in bytes.
  cl_uint global_mem_cacheline_size;
  /// @brief Size of global device memory in bytes.
  cl_ulong global_mem_size;
  /// @brief Describes the optional half precision floating-point capabilities
  /// in a bit-field with the possible values: CL_FP_{DENORM, INF_NAN,
  /// ROUND_TO_NEAREST, ROUND_TO_ZERO, ROUND_TO_INF, FMA, SOFT_FLOAT}. The
  /// minimum is CL_FP_ROUND_TO_ZERO or CL_FP_ROUND_TO_INF | CL_FP_INF_NAN.
  cl_device_fp_config half_fp_config;
  /// @brief CL_TRUE if the device and the host have a unified memory subsystem,
  /// CL_FALSE otherwise.
  cl_bool host_unified_memory;

  // Image properties:
  /// @brief CL_TRUE if the device supports images, CL_FALSE otherwise.
  cl_bool image_support;
  /// @brief CL_TRUE if the decice supports 3d image writes, CL_FALSE otherwise.
  cl_bool image3d_writes;
  /// @brief Max height of 2D image in pixels, minimum 8192.
  size_t image2d_max_height;
  /// @brief Max width of 2D image or 1D not created from a buffer in pixels,
  /// minimum 8192.
  size_t image2d_max_width;
  /// @brief Max depth of 3D image in pixels, minimum 2048.
  size_t image3d_max_depth;
  /// @brief Max height of 3D image in pixels, minimum 2048.
  size_t image3d_max_height;
  /// @brief Max width of 3D image in pixels, minimum 2048.
  size_t image3d_max_width;
  /// @brief Max number of pixels for a 1D image created from a buffer object,
  /// minimum 65536.
  size_t image_max_buffer_size;
  /// @brief Max number of images in a 1D or 2D image array, minimum 2048.
  size_t image_max_array_size;

  /// @brief CL_TRUE if the implementation has a linker available, CL_FALSE
  /// otherwise, can only be false for the embedded platform profile.
  cl_bool linker_available;
  /// @brief Size of the local memory in bytes, minimum 32KB for non
  /// CL_DEVICE_TYPE_CUSTOM devices.
  cl_ulong local_mem_size;
  /// @brief Type of local memory CL_LOCAL, CL_GLOBAL, and can be CL_NONE for
  /// custom devices without local memory support.
  cl_device_local_mem_type local_mem_type;
  /// @brief Maximum clock frequency in MHz.
  cl_uint max_clock_frequency;
  /// @brief Maximum number of parallel compute units, minimum 1.
  cl_uint max_compute_units;
  /// @brief Maximum number of __constant arguments in a kernel, minimum 8 for
  /// non CL_DEVICE_TYPE_CUSTOM devices.
  cl_uint max_constant_args;
  /// @brief Maximum size of a constant buffer allocation in bytes, minimum 64KB
  /// for non CL_DEVICE_TYPE_CUSTOM devices.
  cl_ulong max_constant_buffer_size;
  /// @brief Maximum size of memory object allocation in bytes, minimum is
  /// max(1/4 * CL_DEVICE_GLOBAL_MEM_SIZE, 128*1024*1024), for non
  /// CL_DEVICE_TYPE_CUSTOM devices.
  cl_ulong max_mem_alloc_size;
  /// @brief Maximum size of the arguments that can be passed to a kernel in
  /// bytes, minimum 1024 for non CL_DEVICE_TYPE_CUSTOM devices. With the
  /// minimum value, only 128 arguments can be passed to a kernel.
  size_t max_parameter_size;
  /// @brief Maximum number of simultaneous image objects that can be read by a
  /// kernel, minimum 128.
  cl_uint max_read_image_args;
  /// @brief Maximum number of samplers that can be used in a kernel, minimum 16
  /// (only if image support enabled).
  cl_uint max_samplers;
  /// @brief Maximum number of work-items in a work-group executing a kernel on
  /// a single compute unit using the data parallel execution model, minimum 1.
  size_t max_work_group_size;
  /// @brief Maximum dimensions that specify the global and local work-item IDs
  /// used by the data parallel execution model. Minimum 3 for non
  /// CL_DEVICE_TYPE_CUSTOM devices.
  cl_uint max_work_item_dimensions;
  /// @brief Maximum number of work-items that can be specified in each
  /// dimension of the work-group. Returns n size_t entries where n is the
  /// maximum number of work-items dimensions. The minimum is [1, 1, 1] for non
  /// CL_DEVICE_TYPE_CUSTOM devices.
  size_t max_work_item_sizes[cl::max::WORK_ITEM_DIM];
  /// @brief Max number of simultaneous image objects that can be written to by
  /// a kernel, minimum 8.
  cl_uint max_write_image_args;
  /// @brief Minimum value in bits of the largest OpenCL built-in data type
  /// supported by the device (long16 in FULL profile, long16 or int16 in
  /// embedded profile), for non CL_DEVICE_TYPE_CUSTOM devices.
  cl_uint mem_base_addr_align;
  /// @brief Smallest alignment in bytes which can be used for any data type.
  cl_uint min_data_type_align_size;

  /// @brief Native ISA vector width for char.
  cl_uint native_vector_width_char;
  /// @brief Native ISA vector width for short.
  cl_uint native_vector_width_short;
  /// @brief Native ISA vector width for int.
  cl_uint native_vector_width_int;
  /// @brief Native ISA vector width for long.
  cl_uint native_vector_width_long;
  /// @brief Native ISA vector width for float.
  cl_uint native_vector_width_float;
  /// @brief Native ISA vector width for double, 0 if double support is
  /// disabled.
  cl_uint native_vector_width_double;
  /// @brief Native ISA vector width for double, 0 if half support is disabled.
  cl_uint native_vector_width_half;
  // Sub-device specific info
  /// @brief cl_device_id of the parent device of this sub-device. A null
  /// pointer if the device is a root-level device.
  _cl_device_id *parent_device;
  /// @brief Maximum number of sub-devices that can be created, maximum
  /// CL_DEVICE_MAX_COMPUTE_UNITS.
  cl_uint partition_max_sub_devices;
  /// @brief List of partition types supported, possible values:
  /// CL_DEVICE_PARTITION_{EQUALLY, BY_COUNTS, BY_AFFINITY_DOMAIN}, or 0 if
  /// none of these are supported.
  cl_device_partition_property partition_properties;
  /// @brief List of supported affinity domains for partitioning the device. Bit
  /// field with possible values: CL_DEVICE_AFFINITY_DOMAIN_{NUMA, L4_CACHE,
  /// L3_CACHE, L2_CACHE, L1_CACHE, NEXT_PATITIONABLE}, or 0 if the device
  /// doesn't support affinity domains.
  cl_device_affinity_domain partition_affinity_domain;
  /// @brief Returns the properties argument specified in clCreateSubDevices if
  /// device is a subdevice. Otherwise the implementation may either return a
  /// param_value_size_ret of 0 i.e. there is no partition type associated with
  /// device or can return a property value of 0 (where 0 is used to terminate
  /// the partition property list) in the memory that param_value points to.
  cl_device_partition_property partition_type;

  // Preferred vector sizes:
  /// @brief Preferred vector width size for char.
  cl_uint preferred_vector_width_char;
  /// @brief Preferred vector width size for short.
  cl_uint preferred_vector_width_short;
  /// @brief Preferred vector width size for int.
  cl_uint preferred_vector_width_int;
  /// @brief Preferred vector width size for long.
  cl_uint preferred_vector_width_long;
  /// @brief Preferred vector width size for float.
  cl_uint preferred_vector_width_float;
  /// @brief Preferred vector width size for doubles, 0 if double support is
  /// disabled.
  cl_uint preferred_vector_width_double;
  /// @brief Preferred vector width size for half, 0 if half support is
  /// disabled.
  cl_uint preferred_vector_width_half;

  /// @brief Maximum size of the internal buffer that holds the output of
  /// printf calls from a kernel, minimum 1MB for the FULL profile.
  size_t printf_buffer_size;
  /// @brief CL_TRUE if the device's preference is for the user to be
  /// responsible for synchronisation.
  cl_bool preferred_interop_user_sync;
  /// @brief OpenCL profile string, the profile name supported by the devices.
  cargo::string_view profile;
  /// @brief Resolution of the device's timer in nanoseconds.
  size_t profiling_timer_resolution;
  /// @brief Command-queue properties supported by the device. Bit-field with
  /// possible values: CL_QUEUE_{OUT_OF_ORDER_EXEC_MODE_ENABLE,
  /// PROFILING_ENABLE}, minimum capability: CL_QUEUE_PROFILING_ENABLE.
  cl_command_queue_properties queue_properties;
  /// @brief Device reference count, 1 if the device is a root-level device.
  cl_uint reference_count;
  /// @brief Describes the single precision floating-point capabilities of the
  /// device in a bit-field with the possible values: CL_FP_{DENORM, INF_NAN,
  /// ROUND_TO_NEAREST, ROUND_TO_ZERO, ROUND_TO_INF, FMA,
  /// CORRECTLY_ROUNDED_DIVIDE_SQRT, SOFT_FLOAT}. The minimum is
  /// CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN for non CL_DEVICE_TYPE_CUSTOM
  /// devices.
  cl_device_fp_config single_fp_config;
  /// @brief OpenCL device type, possible values are a combination of:
  /// CL_DEVICE_TYPE_{CPU, GPU, ACCELERATOR, DEFAULT}, or CL_DEVICE_TYPE_CUSTOM.
  cl_device_type type;
  /// @brief Unique device vendor identifier.
  cl_uint vendor_id;
  /// @brief Semi-colon seperated list of builtin kernels.
  std::string builtin_kernel_names;
#if defined(CL_VERSION_3_0)
  /// @brief Bit field of the device's SVM capabilities.
  const cl_device_svm_capabilities svm_capabilities;
  /// @brief Bit field of the devices's atomic memory capabilities.
  const cl_device_atomic_capabilities atomic_memory_capabilities;
  /// @brief Bit field of the devices's atomic fence capabilities.
  const cl_device_atomic_capabilities atomic_fence_capabilities;
  /// @brief Bit field of the devices's device enqueue capabilities.
  const cl_device_device_enqueue_capabilities device_enqueue_capabilities;
  /// @brief Bit field of the device command-queue properties supported by the
  /// device.
  const cl_command_queue_properties queue_on_device_properties;
  /// @brief Prefered size of the device queue in bytes.
  const cl_uint queue_on_device_prefered_size;
  /// @brief Maximum size of the device queue in bytes.
  const cl_uint queue_on_device_max_size;
  /// @brief Maximum number of device queues that can be created for this device
  /// in a single context.
  const cl_uint max_on_device_queues;
  /// @brief Maximum number of events in use by a device queue.
  const cl_uint max_on_device_events;
  /// @brief Whether device supports pipes.
  const cl_bool pipe_support;
  /// @brief Maximum number of pipe objects that can be passed as arguments to a
  /// kernel.
  const cl_uint max_pipe_args;
  /// @brief Maximum number of reservations that can be active for a pipe per
  /// work-item in a kernel.
  const cl_uint pipe_max_active_reservations;
  /// @brief Maximum size of pipe packet in bytes.
  const cl_uint pipe_max_packet_size;
  /// @brief Maximum number of bytes of storage that may be allocated for any
  /// single variable in program scope or inside a function in an OpenCL kernel
  /// language declared in the global address space.
  size_t max_global_variable_size;
  /// @brief Maximum preferred total size, in bytes, of all program variables in
  /// the global address space.
  size_t global_variable_prefered_total_size;
  /// @brief Whether the device supports non-uniform work groups.
  const cl_bool non_uniform_work_group_support;
  /// @brief Max number of image objects arguments of a kernel declared with the
  /// write_only or read_write qualifier.
  const cl_uint max_read_write_image_args;
  /// @brief Row pitch alignment size in pixels for 2D images create from a
  /// buffer.
  const cl_uint image_pitch_alignment;
  /// @brief Minimum alignment in pixels of the host_ptr specified to
  /// clCreateBuffer or clCreateBufferWithProperties when a 2D image is created
  /// from a buffer which was created using CL_MEM_USE_HOST_PTR.
  const cl_uint image_base_address_alignment;
  /// @brief Intermediate languages that can be supported by
  /// clCreateProgramWithIL.
  std::string il_version;
  /// @brief Maximum number of sub-groups in a work-group that a device is
  /// capable of executing on a single compute unit, for a given
  /// kernel-instance running on the device.
  const cl_uint max_num_sub_groups;
  /// @brief Whether device supports independent forward progress of sub-groups.
  const cl_bool sub_group_independent_forward_progress;
  /// @brief Whether device supports work group collective functions.
  const cl_bool work_group_collective_functions_support;
  /// @brief Whether device supports the generic address space and its
  /// associated built-in functions.
  const cl_bool generic_address_space_support;
  /// @brief Array of name,version descriptions listing all the versions of
  /// OpenCL C supported by the compiler for the device.
  cargo::fixed_vector<cl_name_version_khr, 4> opencl_c_all_versions;
  /// @brief Value representing the preferred alignment in bytes for OpenCL 2.0
  /// fine-grained SVM atomic types.
  cl_uint preferred_platform_atomic_alignment;
  /// @brief Value representing the preferred alignment in bytes for OpenCL 2.0
  /// atomic types to global memory.
  cl_uint preferred_global_atomic_alignment;
  /// @brief Value representing the preferred alignment in bytes for OpenCL 2.0
  /// atomic types to local memory.
  cl_uint preferred_local_atomic_alignment;
  /// @brief Preferred multiple of work-group size for the given device.
  /// TODO: Should probably be a core property, see CA-2717.
  size_t preferred_work_group_size_multiple;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Query the platform for its devices.
///
/// @param platform Platform to query for devices.
/// @param device_type Type of device to return.
/// @param num_entries Number of elements in the @p devices list.
/// @param devices List of device handles to store results in.
/// @param num_devices Return number of devices the platform contains.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceIDs.html
CL_API_ENTRY cl_int CL_API_CALL GetDeviceIDs(cl_platform_id platform,
                                             cl_device_type device_type,
                                             cl_uint num_entries,
                                             cl_device_id *devices,
                                             cl_uint *num_devices);

/// @brief Increment the device reference count.
///
/// @param device Device to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainDevice.html
CL_API_ENTRY cl_int CL_API_CALL RetainDevice(cl_device_id device);

/// @brief Decrement the device reference count.
///
/// @param device Device to decrement the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseDevice.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseDevice(cl_device_id device);

/// @brief Query the device for information.
///
/// @param device Device to query for information.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of @p param_value storage.
/// @param param_value Pointer to value to return information in.
/// @param param_value_size_ret Return size in bytes of the storage required
/// for @p param_value.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetDeviceInfo(cl_device_id device,
                                              cl_device_info param_name,
                                              size_t param_value_size,
                                              void *param_value,
                                              size_t *param_value_size_ret);

/// @brief Create an OpenCL sub device object by partitioning a device.
///
/// @param in_device Device to partition to create the sub device.
/// @param properties Properties to enable on the sub device.
/// @param num_devices Number of devices in the @p out_devices list.
/// @param out_devices Return list of sub devices.
/// @param num_devices_ret Return number of devices required for @p
/// out_devices.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateSubDevices.html
CL_API_ENTRY cl_int CL_API_CALL CreateSubDevices(
    cl_device_id in_device, const cl_device_partition_property *properties,
    cl_uint num_devices, cl_device_id *out_devices, cl_uint *num_devices_ret);

/// @}
}  // namespace cl

#endif  // CL_DEVICE_H_INCLUDED
