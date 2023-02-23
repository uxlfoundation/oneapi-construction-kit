// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Definition of VK's instance API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_INSTANCE_H_INCLUDED
#define VK_INSTANCE_H_INCLUDED

#include <compiler/library.h>
#include <mux/mux.h>
#include <vk/allocator.h>
#include <vk/error.h>
#include <vk/icd.h>
#include <vk/small_vector.h>

#include <array>

namespace vk {
/// @copydoc ::vk::physical_device_t
typedef struct physical_device_t *physical_device;

/// @brief Internal instance type.
typedef struct instance_t final : icd_t<instance_t> {
  /// @brief Constructor.
  ///
  /// @param pCreateInfo Instance create info.
  /// @param allocator `vk::allocator` passed to `muxCreateDevices` via the
  /// @p mux_allocator_info_t struct, a reference is maintained here because it
  /// will be used for all mux allocations following instance creation, and we
  /// also use it to initialize `devices`
  instance_t(const VkInstanceCreateInfo *pCreateInfo, vk::allocator allocator);

  /// @brief Destructor.
  ~instance_t();

  /// @brief Instance create info.
  const VkInstanceCreateInfo pCreateInfo;

  /// @brief copy of allocator used to create the instance
  vk::allocator allocator;

  /// @brief List of mux device pointers obtained at initialization
  vk::small_vector<vk::physical_device, 2> devices;
} * instance;

/// @brief The master list of instance extensions this implementation implements
static constexpr std::array<VkExtensionProperties, 1> instance_extensions = {{
    {"VK_KHR_get_physical_device_properties2", 1},
}};

/// @brief Internal implementation of vkCreateInstance.
///
/// @param pCreateInfo Instance create info.
/// @param allocator Allocator wrapper.
/// @param pInstance Return created instance.
///
/// @return Return result code.
VkResult CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                        vk::allocator allocator, vk::instance *pInstance);

/// @brief Internal implementation of vkDestroyInstance.
///
/// @param instance Instance to destroy.
/// @param allocator Allocator wrapper.
void DestroyInstance(vk::instance instance, const vk::allocator allocator);

/// @brief Internal implementation of vkEnumerateInstanceExtensionProperties
///
/// @param pPropertyCount The length of `pProperties` if `pProperties` isn't
/// null, otherwise return the number of available extensions
/// @param pProperties Return `pPropertyCount` extension properties
///
/// @return Return vulkan result code
VkResult EnumerateInstanceExtensionProperties(
    const char *, uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
}  // namespace vk

#endif  // VK_INSTANCE_H_INCLUDED
