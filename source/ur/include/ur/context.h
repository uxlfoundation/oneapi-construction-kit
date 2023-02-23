// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UR_CONTEXT_H_INCLUDED
#define UR_CONTEXT_H_INCLUDED

#include <cassert>
#include <mutex>

#include "cargo/array_view.h"
#include "cargo/dynamic_array.h"
#include "cargo/expected.h"
#include "cargo/small_vector.h"
#include "mux.h"
#include "ur/base.h"

namespace ur {
/// @brief Abstract class which different USM allocations types can inherit
/// from.
struct allocation_info {
  allocation_info(const allocation_info &) = delete;

  /// @brief constructor
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] pUSMFlag Flags to guide allocation behavior.
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  allocation_info(ur_context_handle_t context,
                  const ur_usm_mem_flags_t *pUSMFlag, size_t size,
                  uint32_t alignment)
      : context(context),
        flags(*pUSMFlag),
        size(size),
        align(alignment),
        base_ptr(nullptr) {
    if (context) {
      (void)ur::retain(context);
    }
  }

  /// @brief Pure virtual function which classes will implement to allocates
  /// memory for the USM allocation and binds it to Mux objects for supported
  /// devices.
  ///
  /// @return UR_RESULT_SUCCESS or an UR error code on failure
  virtual ur_result_t allocate() = 0;

  /// @brief Pure virtual function which returns buffer allocated for particular
  /// device.
  ///
  /// @param[in] query_device The device to retrieve the unique index for.
  ///
  /// @return mux_buffer_t on success or nullptr on failure.
  virtual mux_buffer_t getMuxBufferForDevice(
      ur_device_handle_t query_device) const = 0;

  /// @brief Context associated with allocation
  ur_context_handle_t context{nullptr};
  /// @brief Flags guiding allocation
  ur_usm_mem_flags_t flags;
  /// @brief Size in bytes of the requested device allocation.
  size_t size;
  /// @brief Alignment requirements for the allocation
  uint32_t align;
  /// @brief Pointer returned by USM allocation entry points
  void *base_ptr{nullptr};

  /// @brief Destructor.
  virtual ~allocation_info() {
    if (context) {
      (void)ur::release(context);
    }
  }
};

/// @brief Derived class for host USM allocations
struct host_allocation_info final : public allocation_info {
  host_allocation_info(const host_allocation_info &) = delete;

  /// @brief constructor
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] pUSMFlag Flags to guide allocation behavior
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  host_allocation_info(ur_context_handle_t context,
                       const ur_usm_mem_flags_t *pUSMFlag, size_t size,
                       uint32_t alignment);

  /// @brief Allocates host memory for the USM allocation
  /// and binds it to Mux objects for supported devices.
  ///
  /// @return UR_RESULT_SUCCESS or an UR error code on failure
  ur_result_t allocate() override;

  /// @brief Function which returns buffer allocated for particular
  /// device.
  ///
  /// @param[in] query_device The device to retrieve the unique index for.
  ///
  /// @return mux_buffer_t on success or nullptr on failure.
  mux_buffer_t getMuxBufferForDevice(
      ur_device_handle_t query_device) const override;

  /// @brief Destructor.
  ~host_allocation_info();

  /// @brief Mux memory object bound to every device in the OpenCL context.
  cargo::dynamic_array<mux_memory_t> mux_memories;
  /// @brief Mux buffer object bound to every device in the OpenCL context.
  cargo::dynamic_array<mux_buffer_t> mux_buffers;
};

/// @brief Derived class for device USM allocations
struct device_allocation_info final : public allocation_info {
  device_allocation_info(const device_allocation_info &) = delete;

  /// @brief constructor
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] device Device for the allocation
  /// @param[in] pUSMFlag Flags to guide allocation behavior
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  device_allocation_info(ur_context_handle_t context, ur_device_handle_t device,
                         ur_usm_mem_flags_t *pUSMFlag, size_t size,
                         uint32_t alignment);

  /// @brief Allocates device memory for the USM allocation
  /// and binds it to Mux objects for supported devices.
  ///
  /// @return UR_RESULT_SUCCESS or an UR error code on failure
  ur_result_t allocate() override;

  /// @brief Function which returns buffer allocated for particular
  /// device
  ///
  /// @param[in] query_device The device to retrieve the unique index for.
  ///
  /// @return mux_buffer_t on success or nullptr on failure
  mux_buffer_t getMuxBufferForDevice(
      ur_device_handle_t query_device) const override {
    return query_device == device ? mux_buffer : nullptr;
  };

  /// @brief Destructor.
  ~device_allocation_info();

  /// @brief UR device associated with memory allocation
  ur_device_handle_t device;
  /// @brief Mux memory allocated on device
  mux_memory_t mux_memory;
  /// @brief Mux buffer tied to mux_memory
  mux_buffer_t mux_buffer;
};
}  // namespace ur

/// @brief Compute Mux specific implementation of the opaque
/// ur_context_handle_t_ API object.
struct ur_context_handle_t_ : ur::base {
  /// @brief constructor for creating context.
  ///
  /// @param[in] platform Platform to which this context belongs.
  ur_context_handle_t_(ur_platform_handle_t platform) : platform{platform} {}
  ur_context_handle_t_(const ur_context_handle_t_ &) = delete;
  ur_context_handle_t_ &operator=(const ur_context_handle_t_ &) = delete;

  /// @brief Factory method for creating contexts.
  ///
  /// @param[in] platform The platform to which the context will belong.
  /// @param[in] devices The devices that make up this context
  ///
  /// @return A context object or an error code if something went wrong.
  static cargo::expected<ur_context_handle_t, ur_result_t> create(
      ur_platform_handle_t platform,
      cargo::array_view<ur_device_handle_t> devices);

  /// @brief Retrieve unique index associated to a device in the context.
  ///
  /// @param[in] device The device to retrieve the unique index for.
  ///
  /// @return Unique index associated to the device in this context.
  uint32_t getDeviceIdx(ur_device_handle_t device) const {
    const auto it = std::find(std::begin(devices), std::end(devices), device);
    assert((it != std::end(devices)) && "Device does not exist in context!");
    return std::distance(std::begin(devices), it);
  }

  ur::allocation_info *findUSMAllocation(const void *base_ptr);

  /// @brief The platform to which this context belongs.
  ur_platform_handle_t platform = nullptr;
  /// @brief The Devices in this context, the order of these is important and
  /// must remain invariant since it is used to lookup device specific buffers.
  cargo::small_vector<ur_device_handle_t, 4> devices;
  /// @brief List of allocations made through the USM extension entry points.
  cargo::small_vector<std::unique_ptr<ur::allocation_info>, 1> usm_allocations;
  /// @brief Mutex to lock when pushing to queued_commands
  std::mutex mutex;
};

#endif  // UR_CONTEXT_H_INCLUDED
