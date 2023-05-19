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
/// @brief Definitions for the OpenCL kernel API.

#ifndef CL_KERNEL_H_INCLUDED
#define CL_KERNEL_H_INCLUDED

#include <CL/cl.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/optional.h>
#include <cl/base.h>
#include <cl/binary/kernel_info.h>
#include <cl/validate.h>
#include <compiler/kernel.h>
#include <mux/mux.hpp>

#include <unordered_map>

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Host callable function pointer definition.
///
/// @param args A pointer to the argument list.
using user_func_t = void(CL_CALLBACK *)(void *args);

/// @}
}  // namespace cl

/// @addtogroup cl
/// @{

/// @brief A wrapper over either a deferred compiled kernel or a pre-compiled
/// Mux kernel, depending on whether deferred compilation is supported.
class MuxKernelWrapper {
 public:
  /// @brief A Mux executable that contains a single kernel that has been
  /// optimized with specific runtime parameters.
  struct SpecializedKernel {
    SpecializedKernel()
        : mux_executable{nullptr, {nullptr, {nullptr, nullptr, nullptr}}},
          mux_kernel{nullptr, {nullptr, {nullptr, nullptr, nullptr}}} {};
    SpecializedKernel(mux::unique_ptr<mux_executable_t> &&mux_executable,
                      mux::unique_ptr<mux_kernel_t> &&mux_kernel)
        : mux_executable{std::move(mux_executable)},
          mux_kernel{std::move(mux_kernel)} {}
    SpecializedKernel(const SpecializedKernel &) = delete;
    SpecializedKernel(SpecializedKernel &&) = default;
    ~SpecializedKernel();

    SpecializedKernel &operator=(const SpecializedKernel &) = delete;
    SpecializedKernel &operator=(SpecializedKernel &&) = default;

    /// @brief The Mux executable that contains the specialized machine code.
    mux::unique_ptr<mux_executable_t> mux_executable;

    /// @brief A Mux kernel that points to the specialized kernel inside the Mux
    /// executable.
    mux::unique_ptr<mux_kernel_t> mux_kernel;
  };

  /// @brief Constructs an MuxKernelWrapper from a pre-compiled kernel.
  ///
  /// @param device OpenCL device.
  /// @param mux_kernel Pre-compiled Mux kernel to wrap.
  MuxKernelWrapper(cl_device_id device, mux_kernel_t mux_kernel);

  /// @brief Constructs an MuxKernelWrapper from a deferred kernel.
  ///
  /// @param device OpenCL device.
  /// @param deferred_kernel Deferred compiled kernel to wrap.
  MuxKernelWrapper(cl_device_id device, compiler::Kernel *deferred_kernel);

  /// @brief Queries whether this kernel's compilation is being deferred using a
  /// runtime compiler.
  bool supportsDeferredCompilation() const;

  /// @brief If this kernel supports specialization, this function causes the
  /// compiler to pre-cache a specific local size configuration.
  ///
  /// @param local_size_x Local size in the x dimension.
  /// @param local_size_y Local size in the y dimension.
  /// @param local_size_z Local size in the z dimension.
  ///
  /// @return Returns a compiler status code.
  /// @retval `Result::SUCCESS` when precaching the local size was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if the requested local size is invalid.
  compiler::Result precacheLocalSize(size_t local_size_x, size_t local_size_y,
                                     size_t local_size_z);

  /// @brief Returns the dynamic work width for a given local size. If this
  /// kernel does not support specialization, this simply returns 1.
  ///
  /// @param local_size_x Local size in the x dimension.
  /// @param local_size_y Local size in the y dimension.
  /// @param local_size_z Local size in the z dimension.
  ///
  /// @return The dynamic work width for the given local size.
  uint32_t getDynamicWorkWidth(size_t local_size_x, size_t local_size_y,
                               size_t local_size_z);

  /// @brief If this kernel supports specialization, this function performs
  /// deferred compilation and returns a Mux executable-kernel pair that
  /// contains a kernel optimized for the specific Mux execution parameters.
  ///
  /// @param specialization_options Mux execution options to specialize for.
  ///
  /// @return A valid SpecializedKernel object if specialization was successful,
  /// or a status code otherwise.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if any of the specialization options are
  /// invalid.
  /// @retval `Result::FAILURE` if this kernel is not specializable.
  cargo::expected<SpecializedKernel, compiler::Result> createSpecializedKernel(
      const mux_ndrange_options_t &specialization_options);

  /// @brief If this kernel does not support specialization, this returns the
  /// generic Mux kernel that is not specialized for any particular config.
  mux_kernel_t getPrecompiledKernel() const;

  /// @brief Return the sub-group size for this kernel.
  ///
  /// In general this is a function of the kernel, the device it will execute on
  /// and the local size.
  ///
  /// This is used to implement the CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE
  /// query of clGetKernelSubGroupInfo.
  ///
  /// @return The maximum sub-group size that would result from the given local
  /// size. May return CL_INVALID_OPERATION if sub-groups are not supported, or
  /// an error code if it was unsuccessful.
  cargo::expected<size_t, cl_int> getSubGroupSizeForLocalSize(
      size_t local_size_x, size_t local_size_y, size_t local_size_z) const;

  /// @brief Return the number of sub-groups that will be present when the
  /// kernel is enqueued with the given local size.
  ///
  /// This is used to implement the CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE
  /// query of clGetKernelSubGroupInfo.
  ///
  /// @param[in] local_size_x local size in x dimension to query the sub-group
  /// count for.
  /// @param[in] local_size_y local size in y dimension to query the sub-group
  /// count for.
  /// @param[in] local_size_z local size in z dimension to query the sub-group
  /// count for.
  ///
  /// @return The number of sub-groups that would be present if the kernel was
  /// enqueued with the given local size. May return CL_INVALID_OPERATION if
  /// sub-groups are not supported, or an error code if it was unsuccessful.
  cargo::expected<size_t, cl_int> getSubGroupCountForLocalSize(
      size_t local_size_x, size_t local_size_y, size_t local_size_z) const;

  /// @brief Return the local size that would result in the given number of
  /// sub-groups.
  ///
  /// This is used to implement the CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT
  /// query of clGetKernelSubGroupInfo.
  ///
  /// @param[in] sub_group_count The required number of sub-groups.
  //
  /// @return The local size that would result in this number of sub-groups. May
  /// return CL_INVALID_OPERATION if sub-groups are not supported, or an
  /// error code if it was unsuccessful.
  cargo::expected<std::array<size_t, 3>, cl_int> getLocalSizeForSubGroupCount(
      size_t sub_group_count) const;

  /// @brief Return the maximum number of sub-groups that can be supported by
  /// the kernel.
  ///
  /// This is used to implement the CL_KERNEL_MAX_NUM_SUB_GROUPS query of
  /// clGetKernelSubGroupInfo.
  ///
  /// @return The maximum number of sub-groups supported by the kernel. May
  /// return CL_INVALID_OPERATION if sub-groups are not supported, or an error
  /// code if it was unsuccessful.
  cargo::expected<size_t, cl_int> getMaxNumSubGroups() const;

  /// @brief The preferred local size in the x dimension for this kernel.
  const size_t preferred_local_size_x;

  /// @brief The preferred local size in the y dimension for this kernel.
  const size_t preferred_local_size_y;

  /// @brief The preferred local size in the z dimension for this kernel.
  const size_t preferred_local_size_z;

  /// @brief The amount of local memory used by this kernel.
  const size_t local_memory_size;

 private:
  mux_device_t mux_device;
  mux_allocator_info_t mux_allocator_info;
  mux_kernel_t precompiled_kernel;
  compiler::Kernel *deferred_kernel;
};

/// @brief Definition of the OpenCL kernel object.
struct _cl_kernel final : public cl::base<_cl_kernel> {
  /// @brief Struct that represents a kernel argument.
  struct argument final {
    /// @brief Default constructor used to initialize arrays of arguments.
    ///
    /// Arguments initialized with this constructor will have a `type.kind` of
    /// `compiler::ArgumentKind::UNKNOWN`, and a storage type of
    /// `storage_type::uninitialized`.
    argument();

    /// @brief Constructor to create a local memory buffer argument.
    ///
    /// @param arg_type Type of the kernel argument, must have `kind` of
    /// `compiler::ArgumentKind::POINTER` and `address_space` of
    /// `cl::binary::AddressSpace::LOCAL`.
    /// @param local_memory_size Size of the local memory buffer argument.
    argument(cl::binary::ArgumentType arg_type, size_t local_memory_size);

    /// @brief Constructor to create a sampler argument.
    ///
    /// @param arg_type Type of the kernel argument, must have `kind` of
    /// `compiler::ArgumentKind::SAMPLER`.
    /// @param sampler Sampler object.
    argument(cl::binary::ArgumentType arg_type, const cl_sampler sampler);

    /// @brief Constructor to create a memory buffer argument.
    ///
    /// This object does not internally retain the memory objects because the
    /// kernel isn't allowed to retain memory objects set as its arguments.
    ///
    /// @param type Type of the kernel argument, must be a valid type to use
    /// with a memory buffer.
    /// @param mem Memory buffer object.
    argument(cl::binary::ArgumentType type, cl_mem mem);

    /// @brief Constructor to create an argument passed by value.
    ///
    /// This constructor copies the value it's given internally.
    ///
    /// @param type Type of the argument, it should be a struct argument type,
    /// a numerical argument type, or a pointer to a custom buffer.
    ///
    /// @param value Value of the parameter.
    /// @param value_size Size of the data.
    argument(cl::binary::ArgumentType type, const void *value,
             size_t value_size);

#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
    /// @brief Constructor to create an USM allocation argument.
    ///
    /// @param type Type of the kernel argument, must be a valid type to use
    /// with a USM allocation, i.e a global or constant pointer.
    /// @param usm_alloc USM allocation associated with parameter.
    /// @param offset Byte offset from the start of the USM allocation.
    argument(cl::binary::ArgumentType type,
             extension::usm::allocation_info *usm_alloc, size_t offset);
#endif

    /// @brief Copy constructor.
    ///
    /// @param other Argument to copy from.
    argument(const argument &other);

    /// @brief Move constructor.
    ///
    /// @param other Argument to move from.
    argument(argument &&other);

    /// @brief Copy assignment operator.
    ///
    /// @param other Kernel argument to copy.
    ///
    /// @return Kernel argument assigned to.
    argument &operator=(const argument &other);

    /// @brief Move assignment operator.
    ///
    /// After a call to this operator, the right operand object is invalidated
    /// so its type and storage type are the same as if it has been constructed
    /// with the default constructor.
    ///
    /// @param other Kernel argument to move.
    ///
    /// @return Kernel argument assigned to.
    argument &operator=(argument &&other);

    /// @brief Destructor.
    ~argument();

    // @brief Type of the argument.
    cl::binary::ArgumentType type;

    union {
      size_t local_memory_size;
      cl_mem memory_buffer;
      cl_uint sampler_value;
      struct {
        void *data;
        size_t size;
      } value;
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
      struct {
        extension::usm::allocation_info *usm_ptr;
        size_t offset;
      } usm;
#endif
    };

    using info = cl::binary::KernelInfo::ArgumentInfo;

    /// @brief Enum representing the possible storage types that can be used by
    /// the kernel argument. Each of these storage type match a single field of
    /// the union used internally, except for the special storage type
    /// uninitialized which can come up either if the kernel argument object has
    /// been invalidated or if it has been constructed with the default no
    /// parameters constructor.
    enum struct storage_type {
      local_memory,
      memory_buffer,
      sampler,
      value,
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
      usm,
#endif
      uninitialized
    };

    storage_type stype;
  };

 private:
  /// @brief Private constructor, use _cl_kernel::create() instead.
  ///
  /// @param[in] program Program containing the kernel to create.
  /// @param[in] name Name of the kernel in the program to create.
  /// @param[in] info Information from the compiler about the kernel.
  _cl_kernel(cl_program program, std::string name,
             const cl::binary::KernelInfo *info);

 public:
  /// @brief Deleted copy constructor.
  ///
  /// The copy constructor is explicitly deleted to avoid accidentally using
  /// the compiler-generated one and the resulting dynamic memory issues.
  _cl_kernel(const _cl_kernel &) = delete;

  /// @brief Destructor.
  ~_cl_kernel();

  /// @brief Create a kernel object.
  ///
  /// @param[in] program Program containing the kernel to create.
  /// @param[in] name Name of the kernel in the program to create.
  /// @param[in] info Information from the compiler about the kernel.
  ///
  /// @return Returns a kernel object on success or an error code otherwise.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occured.
  static cargo::expected<cl_kernel, cl_int> create(
      cl_program program, std::string name, const cl::binary::KernelInfo *info);

  /// @brief Clone this kernel.
  ///
  /// Creates a shallow copy of this kernel object.
  ///
  /// @return A new kernel object.
  cargo::expected<cl_kernel, cl_int> clone() const;

  /// @brief Query the kernel meta data for argument information.
  ///
  /// In SPIR all argument information is always present, except for the
  /// argument name, which requires the kernel to be compiled with
  /// "-cl-kernel-arg-info". Use the presence of the argument name to track
  /// whether we can legally call clGetKernelArgInfo.
  ///
  /// @return Return true if the kernel was compiled to a program and the kernel
  /// argument info could be retrieved, false otherwise.
  bool GetArgInfo();

  /// @brief Query the kernel for the arguments type.
  ///
  /// @param[in] arg_index Index of the argument to get type of.
  ///
  /// @return Return the argument type on success, CL_INVALID_ARG_INDEX
  /// otherwise.
  cargo::expected<const cl::binary::ArgumentType &, cl_int> GetArgType(
      const cl_uint arg_index) const;

  /// @brief Set up the mux kernel execution options.
  ///
  /// @param[in] device OpenCL device to target.
  /// @param[in] device_index Index of the device in OpenCL context device list.
  /// @param[in] work_dim Number of dimensions of work to do.
  /// @param[in] local_size Local size of work to do.
  /// @param[in] global_offset Global index offset to begin work at.
  /// @param[in] global_size Global size of work to do.
  /// @param[in] printf_buffer Buffer to write printf output into.
  /// @param[out] descriptors A unique_ptr that can be used to store the
  /// allocated array of mux_descriptor_info_t used in the resulting
  /// mux_execution_options_t.
  ///
  /// @return Returns the relevant kernel execution options.
  mux_ndrange_options_t createKernelExecutionOptions(
      cl_device_id device, cl_uint device_index, size_t work_dim,
      const std::array<size_t, cl::max::WORK_ITEM_DIM> &local_size,
      const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_offset,
      const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_size,
      mux_buffer_t printf_buffer,
      std::unique_ptr<mux_descriptor_info_t[]> &descriptors);

  /// @brief Retain cl_mem objects that are the arguments to a kernel.
  ///
  /// Retain cl_mem objects via a callback through which the meaning of "retain"
  /// can be customized. For example; some callers may need to store the cl_mem
  /// reference in a specific buffer so it can be released later.
  ///
  /// @param[in] command_queue Queue on which kernel will be executed.
  /// @param[in] retain Callback which does the retaining.
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int retainMems(cl_command_queue command_queue,
                    std::function<bool(cl_mem)> retain);

  /// @brief Verify reqd_work_group_size attribute.
  ///
  /// Verifies that the reqd_work_group_size attribute matches the
  /// `local_work_size` if it is non-null. If `local_work_size` is null (this is
  /// the use case that the OpenCL user doesn't pass a local size) then set
  /// local_work_size to the value of reqd_work_group_size attribute.
  ///
  /// @param[in] work_dim The work dimensions.
  /// @param[in,out] local_work_size If null is set to value of
  /// reqd_work_group_size attribute (if it exists), otherwise functions checks
  /// that this parameter matches reqd_work_group_size.
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int checkReqdWorkGroupSize(cl_uint work_dim,
                                const size_t *&local_work_size);

  /// @brief Verify global and local sizes.
  ///
  /// @param[in] device Device on which the kernel will be enqueued.
  /// @param[in] work_dim Dimensions of the kernel enqueue.
  /// @param[in] global_work_offset (x,y,z) global offset.
  /// @param[in] global_work_size (x,y,z) global size.
  /// @param[in] local_work_size Local workgroup size (may be NULL).
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int checkWorkSizes(cl_device_id device, cl_uint work_dim,
                        const size_t *global_work_offset,
                        const size_t *global_work_size,
                        const size_t *local_work_size);

  /// @brief Validate kernel arguments.
  ///
  /// Checks that all kernel arguments are valid.
  ///
  /// @return CL_SUCCESS if all arguments valid, otherwise
  /// CL_INVALID_KERNEL_ARGS.
  cl_int checkKernelArgs();

  /// @brief Choose an appropriate local work group size.
  ///
  /// Should be called in the case the user doesn't request a local size and
  /// kernel does not have a reqd_work_group_size attribute.
  ///
  /// @param[in] device Device on which to execute this kernel.
  /// @param[in] global_size Global work size.
  /// @param[in] work_dim Work dimensions.
  ///
  /// @return local work group dimensions.
  std::array<size_t, cl::max::WORK_ITEM_DIM> getDefaultLocalSize(
      cl_device_id device, const size_t *global_size, cl_uint work_dim);

  /// @brief Program the kernel was constructed from.
  cl_program program;
  /// @brief Name of the kernel.
  std::string name;
  /// @brief Pointer to kernel information.
  const cl::binary::KernelInfo *info;
  /// @brief Array of arguments.
  cargo::dynamic_array<argument> saved_args;
  /// @brief Array of argument information.
  cargo::optional<cargo::dynamic_array<argument::info>> arg_info;
  /// @brief OpenCL device to kernels map.
  std::unordered_map<cl_device_id, std::unique_ptr<MuxKernelWrapper>>
      device_kernel_map;
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  /// @brief USM allocations set via clSetKernelExecInfo
  cargo::dynamic_array<extension::usm::allocation_info *> indirect_usm_allocs;
  /// @brief Bitfield representing USM flags set via clSetKernelExecInfo
  cl_ushort kernel_exec_info_usm_flags = 0;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL kernel object.
///
/// @param[in] program Successfully build program.
/// @param[in] kernel_name Name of the kernel in the program.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new kernel object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateKernel.html
CL_API_ENTRY cl_kernel CL_API_CALL CreateKernel(cl_program program,
                                                const char *kernel_name,
                                                cl_int *errcode_ret);

/// @brief Increment the kernel reference count.
///
/// @param[in] kernel Kernel to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainKernel.html
CL_API_ENTRY cl_int CL_API_CALL RetainKernel(cl_kernel kernel);

/// @brief Decrement the kernel reference count.
///
/// @param[in] kernel Kernel to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseKernel.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseKernel(cl_kernel kernel);

/// @brief Set an argument on the kernel for later use.
///
/// @param[in] kernel Kernel to set the argument on.
/// @param[in] arg_index Index of the argument to set.
/// @param[in] arg_size Size in bytes of the argument value.
/// @param[in] arg_value Pointer to the value to set.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clSetKernelArg.html
CL_API_ENTRY cl_int CL_API_CALL SetKernelArg(cl_kernel kernel,
                                             cl_uint arg_index, size_t arg_size,
                                             const void *arg_value);

/// @brief Create all kernels in the program.
///
/// @param[in] program Program to create the kernel from.
/// @param[in] num_kernels Number of kernel handles in the `kernels` list.
/// @param[out] kernels List of kernel objects to be created.
/// @param[out] num_kernels_ret Return the number of elements required for
/// `kernels` if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateKernelsInProgram.html
CL_API_ENTRY cl_int CL_API_CALL
CreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                       cl_kernel *kernels, cl_uint *num_kernels_ret);

/// @brief Query the kernel for information.
///
/// @param[in] kernel Kernel to query for information.
/// @param[in] param_name Type of information to query
/// @param[in] param_value_size Size in bytes of `param_value`.
/// @param[in] param_value Pointer to value to store information in.
/// @param[out] param_value_size_ret Return size in bytes required for
/// `param_value` to store information if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetKernelInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetKernelInfo(cl_kernel kernel,
                                              cl_kernel_info param_name,
                                              size_t param_value_size,
                                              void *param_value,
                                              size_t *param_value_size_ret);

/// @brief Query the kernel for argument information.
///
/// @param[in] kernel Kernel to query for argument information.
/// @param[in] arg_indx Index the argument to query information for.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of `param_value`.
/// @param[in] param_value Pointer to value to store information in.
/// @param[out] param_value_size_ret Return size in bytes required for
/// `param_value` to store information if not null.
///
/// @return Returns error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetKernelArgInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetKernelArgInfo(
    cl_kernel kernel, cl_uint arg_indx, cl_kernel_arg_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret);

/// @brief Query the kernel for for work group information.
///
/// @param[in] kernel Kernel to query for work group information.
/// @param[in] device Device to target when performing the query.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of `param_value`.
/// @param[in] param_value Pointer to value to store information int.
/// @param[out] param_value_size_ret Return size in bytes required for
/// `param_value` to store information.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetKernelWorkGroupInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetKernelWorkGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret);

/// @brief Enqueue an N-dimensional kernel invocation on the queue.
///
/// @param[in] command_queue Command queue to enqueue the invocation on.
/// @param[in] kernel Kernel to invoke on the queue.
/// @param[in] work_dim Number of dimensions for `global_work_offset`,
/// `global_work_size`, and `local_work_size`.
/// @param[in] global_work_offset Offset for global invocation ID's to begin
/// from, may be null.
/// @param[in] global_work_size Number of global work items, must not be null.
/// @param[in] local_work_size Local work group size, may be null.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueNDRangeKernel(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t *global_work_offset, const size_t *global_work_size,
    const size_t *local_work_size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a single kernel invocation.
///
/// @param[in] command_queue Command queue to queue the invocation on.
/// @param[in] kernel Kernel to invoke on the queue.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueTask.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueTask(cl_command_queue command_queue,
                                            cl_kernel kernel,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event *event_wait_list,
                                            cl_event *event);

/// @brief Enqueue a native kernel invocation on the queue.
///
/// @param[in] command_queue Command queue to enqueue to invocation on.
/// @param[in] user_func Pointer to host callable user callback.
/// @param[in] args Pointer to the arguments `user_func` is called with.
/// @param[in] cb_args Size in bytes of the callback arguments.
/// @param[in] num_mem_objects Number of memory objects passed in `args`.
/// @param[in] mem_list List of memory objects may be null is `num_mem_objects`
/// is `0`.
/// @param[in] args_mem_loc List of memory object locations.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueNativeKernel.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueNativeKernel(
    cl_command_queue command_queue, cl::user_func_t user_func, void *args,
    size_t cb_args, cl_uint num_mem_objects, const cl_mem *mem_list,
    const void *const *args_mem_loc, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @}
}  // namespace cl

#endif  // CL_KERNEL_H_INCLUDED
