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

/// @brief Command Buffer.
///
/// This extension adds support for buffers of commands to be recorded and
/// replayed, reducing the driver overhead required to rebuild command queues.

#ifndef CL_EXTENSION_KHR_COMMAND_BUFFER_H_INCLUDED
#define CL_EXTENSION_KHR_COMMAND_BUFFER_H_INCLUDED

#include <CL/cl_ext.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <cl/base.h>
#include <cl/limits.h>
#include <cl/mux.h>
#include <extension/extension.h>

#include <array>
#include <atomic>
#include <mutex>

struct printf_info_t;

namespace extension {
/// @addtogroup cl_extension
/// @{

class khr_command_buffer : public extension {
 public:
  khr_command_buffer();
  /// @brief Queries for the extension function associated with
  /// `func_name`.
  ///
  /// @see
  /// https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_command_buffer
  ///
  /// @param[in] platform OpenCL platform `func_name` belongs to.
  /// @param[in] func_name Name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function with
  /// `func_name` or `nullptr` if it does not exist.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#ifdef OCL_EXTENSION_cl_khr_command_buffer
/// @addtogroup cl
/// @{

/// @brief Definition of the OpenCL cl_mutable_command_khr object.
struct _cl_mutable_command_khr final {
 private:
  /// @brief Private constructor, use the `create()` function instead.
  ///
  /// By making the constructor private we can restrict creation of
  /// `_cl_mutable_command_khr` objects to the factory function `create()`
  /// which allows us to return error codes in the case that construction
  /// fails, e.g. due to a failed allocation and avoid stack allocation.
  ///
  /// @param[in] id a non-negative index that uniquely identifies this command
  /// within its containing command-buffer.
  /// @param[in] kernel the kernel object that is executed in the mutable
  /// command.
  _cl_mutable_command_khr(cl_uint id, cl_kernel kernel);

 public:
  /// @brief Destructor.
  ~_cl_mutable_command_khr();

  /// @brief Create _cl_mutable_command_khr.
  ///
  /// @param[in] id a non-negative index that uniquely identifies this command.
  /// @param[in] kernel the kernel object that is executed in the mutable
  /// command.
  ///
  /// @return valid _cl_mutable_command_khr object or an error.
  static cargo::expected<std::unique_ptr<_cl_mutable_command_khr>, cl_int>
  create(cl_uint id, cl_kernel kernel);

  /// @brief Index that uniquely identifies this command within its containing
  /// command-buffer.
  cl_uint id;
  /// @brief The kernel object the mutable object is a handle on.
  ///
  /// We can use this get the types of the kernel arguments then construct the
  /// appropriate descriptors to update the arguments.
  cl_kernel kernel;

  // Below members are used for clGetMutableCommandInfo querying, if we ever
  // support mutating the ND-range configs they will need updated too.

  /// @brief Command-buffer used to create mutable command
  cl_command_buffer_khr command_buffer;
  /// @brief List of properties passed on creation
  cargo::small_vector<cl_ndrange_kernel_command_properties_khr, 3>
      properties_list;
  /// @brief Fields of mutable kernel command that can be modified
  cl_mutable_dispatch_fields_khr updatable_fields = 0;
  /// @brief Work dimensions used on mutable-dispatch creation
  cl_uint work_dim;
  /// @brief Global work offset used on mutable-dispatch creation
  std::array<size_t, 3> work_offset = {0, 0, 0};
  /// @brief Global work size used on mutable-dispatch creation
  std::array<size_t, 3> global_size = {0, 0, 0};
  /// @brief Local work size used on mutable-dispatch creation
  std::array<size_t, 3> local_size = {0, 0, 0};
};

/// @brief Definition of the OpenCL cl_command_buffer_khr object.
///
/// This class inherits from cl::base to make use of the reference counting
/// mechanisms in that base class. Because references to a command-buffer may
/// be added through calls to clRetainCommandBufferKHR, or through internal
/// calls, the destructor of this class must only be called once the internal
/// and external reference counts are zero. This is all handled by cl::base.
/// Because of the above, the destructor of _cl_command_buffer_khr must not be
/// called anywhere except from reference counting functions in cl::base. To
/// enforce this we make the constructor of _cl_command_buffer_khr private to
/// avoid stack allocations and add a static factory method called `create()`
/// which heap allocates _cl_command_buffer_khr instances.
struct _cl_command_buffer_khr final : public cl::base<_cl_command_buffer_khr> {
 private:
  /// @brief Kernels associated to the command-buffer via a call to
  /// clEnqueueCommandNDRangeKHR
  cargo::small_vector<cl_kernel, 8> kernels;
  /// @brief Specialized executables and kernels that were created from the CL
  /// kernels. The command-buffer is responsible for destroying these when it is
  /// released.
  cargo::small_vector<std::pair<mux_executable_t, mux_kernel_t>, 8> mux_kernels;
  /// @brief OpenCL buffers and images associated with the command-buffer
  cargo::small_vector<cl_mem, 8> mems;
  /// @brief Index of next command enqueued to the command-buffer.
  cl_uint next_command_index;
  /// @brief List of Mux buffers used to implement printf which are associated
  /// with kernels in the command-buffer
  cargo::small_vector<std::unique_ptr<printf_info_t>, 1> printf_buffers;
  /// @brief Bitfield of the flags set in properties
  cl_command_buffer_flags_khr flags;
  /// @brief List of mux sync-points indexed by cl_sync_point_khr
  cargo::small_vector<mux_sync_point_t, 4> mux_sync_points;

  /// @brief Private constructor, use the `create()` function instead.
  ///
  /// By making the constructor private we can restrict creation of
  /// `_cl_command_buffer_khr` objects to the factory function `create()`
  /// which allows us to return error codes in the case that construction
  /// fails, e.g. due to a failed allocation and avoid stack allocation.
  ///
  /// @param[in] queue cl_command_queue the command-buffer is associated
  /// to. In the future we may support associating multiple queues to
  /// a single command-buffer.
  _cl_command_buffer_khr(cl_command_queue queue);

  /// @brief Associate a kernel to the command-buffer.
  ///
  /// @param kernel cl_kernel to associate with the command-buffer. The command
  /// buffer will retain the kernel and release the reference on the kernel when
  /// it is destroyed.
  cl_int retain(cl_kernel kernel);

  /// @brief Associate a specialized kernel to the command-buffer.
  ///
  /// @param[in] executable mux_executable_t to associate with the command
  /// buffer. The command-buffer will destroy the specialized executable when it
  /// is destroyed.
  /// @param[in] kernel mux_kernel_t to associate with the command-buffer. The
  /// command-buffer will destroy the specialized kernel when it is destroyed.
  cl_int storeKernel(mux_executable_t executable, mux_kernel_t kernel);

  /// @brief Associate a buffer or image with the command-buffer.
  ///
  /// @param[in] mem_obj cl_mem to associate with the command-buffer. The
  /// command-buffer will retain the memory object and release the reference on
  /// the memory when the command-buffer is destroyed.
  cl_int retain(cl_mem mem_obj);

  /// @brief Returns a list of mux sync-points for a mux command to wait on
  /// based on a list of OpenCL sync-points.
  ///
  /// The OpenCL sync-points are used as an index into an internal list of Mux
  /// sync-points held by the command-buffer.
  ///
  /// @param[in] List of OpenCL sync-points
  ///
  /// @return List of Mux sync-points
  cargo::expected<cargo::small_vector<mux_sync_point_t, 4>, cl_int>
  convertWaitList(
      const cargo::array_view<const cl_sync_point_khr> &cl_wait_list);

 public:
  /// @brief Boolean flag indicating whether the clFinalizeCommandBufferKHR API
  /// has been called on this command-buffer.
  cl_bool is_finalized;
  /// @brief mux command-buffer underlying the command-buffer.
  mux_command_buffer_t mux_command_buffer;
  /// @brief command queue associated to the command-buffer. We currently
  /// only support a single queue here.
  cl_command_queue command_queue;
  /// @brief Reference count of active command-buffer submissions, used to
  /// determine state.
  std::atomic<cl_uint> execution_refcount;
  /// @brief List of properties passed on creation
  cargo::small_vector<cl_command_buffer_properties_khr, 3> properties_list;
  /// @brief List of handles returned from command recording entry-points.
  ///
  /// The lifetime of returned handles is tied to the lifetime of the
  /// command-buffer, and it is the responsibility of this command-buffer
  /// object to destroy these handles rather than the application user.
  cargo::small_vector<std::unique_ptr<_cl_mutable_command_khr>, 2>
      command_handles;

  struct UpdateInfo final {
    // Storage for descriptors of each argument that is getting updated.
    cargo::dynamic_array<mux_descriptor_info_s> descriptors;
    // Storage for the indices of each arguments that is getting updated.
    cargo::dynamic_array<uint64_t> indices;
    // Storage for any pointers referenced by descriptors.
    cargo::dynamic_array<const void *> pointers;
    // ID of mutable command
    cl_uint id;
  };
  cargo::small_vector<UpdateInfo, 1> updates;

  /// @brief Mutex to protect the state of the command-buffer.
  std::mutex mutex;

  /// @brief Identify the command-buffer's state.
  ///
  /// @return State of the command-buffer. One of: Recording, Executable,
  /// Pending, or Invalid.
  cl_command_buffer_state_khr getState() const;

  /// @brief Finalize the command-buffer.
  ///
  /// Finalize the underlying mux command-buffer and updated the finalized flag.
  ///
  /// @return CL_SUCCESS or appropriate error if finalization failed.
  cl_int finalize();

  /// @brief Destructor.
  ///
  /// Because there may be multiple references to the given command-buffer
  /// this constructor should only be called through the reference counting
  /// machinery in cl::base once the reference count reaches zero.
  ~_cl_command_buffer_khr();

  /// @brief Create command queue.
  ///
  /// @param[in] queue cl_command_queue associated with the command-buffer.
  /// @param[in] properties List of properties values denoting property
  /// information about the command-buffer to be created.
  static cargo::expected<std::unique_ptr<_cl_command_buffer_khr>, cl_int>
  create(cl_command_queue queue,
         const cl_command_buffer_properties_khr *properties);

  /// @brief Add a barrier command to the command-buffer
  ///
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS if successful, otherwise appropriate OpenCL error code.
  cl_int commandBarrierWithWaitList(
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a copy buffer command to the command-buffer.
  ///
  /// @param[in] src_buffer Source buffer.
  /// @param[in] dst_buffer Destination buffer.
  /// @param[in] src_offset Offset into source buffer at which to start reading.
  /// @param[in] dst_offset Offset into destination buffer at which to start
  /// writing.
  /// @param[in] size Size in bytes of the copy.
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS if successful, otherwise appropriate OpenCL error code.
  cl_int commandCopyBuffer(
      cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset,
      size_t dst_offset, size_t size,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a copy image command to the command-buffer.
  ///
  /// @param[in] src_image Source image.
  /// @param[in] dst_image Destination image.
  /// @param[in] src_origin Pixel offset into source image at which to start
  /// reading.
  /// @param[in] dst_origin Pixel offset into destination image at which to
  /// start writing.
  /// @param[in] region (widths, height, depth) in pixels to copy
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS if successful, otherwise appropriate OpenCL error code.
  cl_int commandCopyImage(
      cl_mem src_image, cl_mem dst_image, const size_t *src_origin,
      const size_t *dst_origin, const size_t *region,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a copy buffer rect command to the command-buffer.
  ///
  /// @param[in] src_buffer Source buffer.
  /// @param[in] dst_buffer Destination buffer.
  /// @param[in] src_origin (x, y, z) offset in `src_buffer`.
  /// @param[in] dst_origin (x, y, z) offset in `dst_buffer`.
  /// @param[in] region (width in bytes, height in rows, depth in slices)
  /// rectangle/cube to be copied.
  /// @param src_row_pitch length of each row slice in bytes for `src_buffer`.
  /// @param src_slice_pitch length of each 2D slice in bytes for `src_buffer`.
  /// @param dst_row_pitch length of each row slice in bytes for `dst_buffer`.
  /// @param dst_row_pitch length of each 2D slice in bytes for `dst_buffer`.
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// See the OpenCL clEnqueueCopyBufferRect spec for a more in depth
  /// explanation of these arguments:
  /// https://www.khronos.org/registry/OpenCL/specs/3.0-unified/pdf/OpenCL_API.pdf
  ///
  /// @return CL_SUCCESS of appropriate OpenCL error code.
  cl_int commandCopyBufferRect(
      cl_mem src_buffer, cl_mem dst_buffer, const size_t *src_origin,
      const size_t *dst_origin, const size_t *region, size_t src_row_pitch,
      size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a fill buffer command to the command-buffer.
  ///
  /// @param[in] buffer Buffer to fill.
  /// @param[in] pattern Pattern to fill `buffer` with.
  /// @param[in] pattern_size size of `pattern` in bytes.
  /// @param[in] offset Offset in bytes into `buffer` at which to start filling.
  /// @param[in] size Size in bytes of region in `buffer` to fill with pattern.
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int commandFillBuffer(
      cl_mem buffer, const void *pattern, size_t pattern_size, size_t offset,
      size_t size, cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a fill image command to the command-buffer.
  ///
  /// @param[in] image Image to fill.
  /// @param[in] fill_color Color used to fill the image with
  /// @param[in] origin Offset in pixels of the image
  /// @param[in] region (Height, Width, Depth) of the image
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int commandFillImage(
      cl_mem image, const void *fill_color, const size_t *origin,
      const size_t *region,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a copy buffer to image command to the command-buffer.
  ///
  /// @param[in] src_buffer Source buffer.
  /// @param[in] dst_image Destination image.
  /// @param[in] src_offset Byte offset into source buffer at which to start
  /// reading.
  /// @param[in] dst_origin Pixel offset into destination image at which to
  /// start writing.
  /// @param[in] region (widths, height, depth) in pixels to copy
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS if successful, otherwise appropriate OpenCL error code.
  cl_int commandCopyBufferToImage(
      cl_mem src_buffer, cl_mem dst_image, size_t src_offset,
      const size_t *dst_origin, const size_t *region,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add a copy image to buffer command to the command-buffer.
  //
  /// @param[in] src_image Source image.
  /// @param[in] dst_buffer Destination buffer.
  /// @param[in] src_origin Pixel offset into source image at which to start
  /// reading.
  /// @param[in] region (widths, height, depth) in pixels to copy
  /// @param[in] dst_offset Byte offset into destination buffer at which to
  /// start writing.
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  ///
  /// @return CL_SUCCESS if successful, otherwise appropriate OpenCL error code.
  cl_int commandCopyImageToBuffer(
      cl_mem src_image, cl_mem dst_buffer, const size_t *src_origin,
      const size_t *region, size_t dst_offset,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point);

  /// @brief Add an ND Range command to the command-buffer.
  ///
  /// @param[in] kernel Kernel to enqueue to the command-buffer.
  /// @param[in] work_dim Dimension of the ND Range i.e. 1, 2 or 3.
  /// @param[in] global_work_offset Offset of global ids in each dimension.
  /// @param[in] global_work_size Global work dimensions.
  /// @param[in] local_work_size Local workgroup size.
  /// @param[in] cl_wait_list List of sync-point command dependencies to wait
  /// on.
  /// @param[out] cl_sync_point Sync-point representing this command, which
  /// other commands can wait on.
  /// @param[out] mutable_handle If non-null will return a handle to this
  /// command for later reference.
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int commandNDRangeKernel(
      cl_kernel kernel, cl_uint work_dim, const size_t *global_work_offset,
      const size_t *global_work_size, const size_t *local_work_size,
      cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
      cl_sync_point_khr *cl_sync_point, cl_mutable_command_khr *mutable_handle);

  /// @brief Modify commands in command-buffer
  ///
  /// @param[in] mutable_config New configuration for one or more commands
  ///
  /// @return CL_SUCCESS or appropriate OpenCL error code.
  cl_int updateCommandBuffer(const cl_mutable_base_config_khr &mutable_config);

  /// @brief Verifies whether a queue is compatible with the command-buffer.
  ///
  /// A command queue is considered compatible if it has identical properties,
  /// underlying device and context.
  ///
  /// @param queue Command queue being checked for compatibility.
  ///
  /// @return Boolean value indicating whether the queue is compatible.
  /// @retval `true` if queue is compatible.
  /// @retval `false` if queue is not compatible.
  bool isQueueCompatible(const cl_command_queue queue) const;

  /// @brief Checks whether simultaneous use flag is set on command-buffer
  ///
  /// @return True if flag was set, false otherwise
  bool supportsSimultaneousUse() const {
    return flags & CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR;
  }

#ifdef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  bool isMutable() const { return flags & CL_COMMAND_BUFFER_MUTABLE_KHR; }
#endif
};
/// @}
#endif  // OCL_EXTENSION_cl_khr_command_buffer
#endif  // CL_EXTENSION_KHR_COMMAND_BUFFER_H_INCLUDED
