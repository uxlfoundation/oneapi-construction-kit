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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#include "vector_add.h"  // Contains shader SPIR-V

#define IS_VK_SUCCESS(X)                                                       \
  {                                                                            \
    const VkResult ret_val = X;                                                \
    if (VK_SUCCESS != ret_val) {                                               \
      fprintf(stderr, "Vulkan error occurred: %s returned %d\n", #X, ret_val); \
      exit(1);                                                                 \
    }                                                                          \
  }

#define NUM_WORK_ITEMS 64

// There is no global state in Vulkan. Create and return a VkInstance object
// which initializes the Vulkan library and encapsulates per-application state.
VkInstance createVkInstance() {
  const VkApplicationInfo app_info = {
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      NULL,
      "VectorAddition",          // Application name
      VK_MAKE_VERSION(1, 0, 0),  // Application version (Major.Minor.Patch)
      "Codeplay",                // Engine Name
      0,
      VK_MAKE_VERSION(1, 0, 0)  // Vulkan API version to target
  };

  // The first member of all create info structs is a `sType` member
  // representing the structure type. This is to aide backwards compatibility,
  // so that the stuct can change in future versions without having to add a
  // new entry point.
  const VkInstanceCreateInfo inst_create_info = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      NULL,
      0,
      &app_info,
      0,  // Number of layers to enable
      NULL,
      0,  // Number of instance extensions to enable
      NULL};

  VkInstance instance;

  // `VkCreate*` APIs allocate memory for creating objects, and take
  // `Vk*CreateInfo` structures with the object parameters. As a result this
  // memory must also be freed by an associated VkDestroy*. Here we call
  // vkDestoryInstance() at the end of main().
  IS_VK_SUCCESS(vkCreateInstance(&inst_create_info, NULL, &instance));

  printf(" * VkInstance successfully created\n");
  return instance;
}

// Queries for suitable device memory which is large enough for our requirements
uint32_t getMemoryTypeIndex(VkPhysicalDevice device, VkDeviceSize memory_size) {
  // The VkPhysicalDeviceMemoryProperties struct describes memory heaps as well
  // as memory types that can be used to access those heaps.
  VkPhysicalDeviceMemoryProperties properties;
  vkGetPhysicalDeviceMemoryProperties(device, &properties);

  // Iterate over all memory types in physical device looking for a memory of
  // at least required size which is cache coherent and can be mapped to host.
  uint32_t mem_type_index = UINT32_MAX;
  for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
    const VkMemoryType mem_type = properties.memoryTypes[i];
    if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & mem_type.propertyFlags) &&
        (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & mem_type.propertyFlags) &&
        (memory_size < properties.memoryHeaps[mem_type.heapIndex].size)) {
      mem_type_index = i;
      break;
    }
  }

  if (mem_type_index == UINT32_MAX) {
    fprintf(stderr,
            "Couldn't find suitable memory of a least %" PRIu64 " bytes\n",
            memory_size);
    exit(1);
  }

  return mem_type_index;
}

// A physical device should group all queues of matching capabilities together
// in a single family. We want to find the index of the first queue family with
// compute support.
uint32_t getComputeQueueFamilyIndex(VkPhysicalDevice device) {
  // Query number of compute families
  uint32_t queue_properties_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_properties_count, 0);

  // Request all queue families
  VkQueueFamilyProperties* queue_properties = (VkQueueFamilyProperties*)malloc(
      sizeof(VkQueueFamilyProperties) * queue_properties_count);

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_properties_count,
                                           queue_properties);

  uint32_t compute_queue_index = UINT32_MAX;
  for (uint32_t i = 0; i < queue_properties_count; ++i) {
    const VkQueueFlags queue_flags = queue_properties[i].queueFlags;
    if (queue_flags & VK_QUEUE_COMPUTE_BIT) {
      compute_queue_index = i;
      break;
    }
  }

  if (compute_queue_index == UINT32_MAX) {
    fputs("Couldn't find a compute queue on device, exiting\n", stderr);
    exit(1);
  }

  free(queue_properties);
  return compute_queue_index;
}

// Given a VkInstance this function finds a vkPhysicalDevice and VkDevice
// corresponding to Codeplay's CPU target.
void createVkDevice(VkInstance instance, VkDevice* device,
                    VkPhysicalDevice* physical_device) {
  // Retrieve list of physical devices
  uint32_t num_devices = 0;
  IS_VK_SUCCESS(vkEnumeratePhysicalDevices(instance, &num_devices, 0));

  if (0 == num_devices) {
    fputs("No Vulkan devices found, exiting\n", stderr);
    exit(1);
  }

  VkPhysicalDevice* phys_devices =
      (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * num_devices);
  IS_VK_SUCCESS(
      vkEnumeratePhysicalDevices(instance, &num_devices, phys_devices));

  // Find Codeplay CPU device
  VkPhysicalDevice* codeplay_cpu_device = NULL;
  for (uint32_t i = 0; i < num_devices; ++i) {
    VkPhysicalDevice phys_device = phys_devices[i];

    VkPhysicalDeviceProperties phys_dev_properties;
    vkGetPhysicalDeviceProperties(phys_device, &phys_dev_properties);

    // 0x10004 is Codeplay vendor ID VK_VENDOR_ID_CODEPLAY, find our CPU target
    if (VK_VENDOR_ID_CODEPLAY == phys_dev_properties.vendorID &&
        VK_PHYSICAL_DEVICE_TYPE_CPU == phys_dev_properties.deviceType) {
      printf(" * Selected device: %s\n", phys_dev_properties.deviceName);
      codeplay_cpu_device = &phys_devices[i];
      break;
    }
  }

  if (NULL == codeplay_cpu_device) {
    fputs("Couldn't find Codeplay Vulkan CPU device, exiting\n", stderr);
    exit(1);
  }

  // Set output parameter
  *physical_device = *codeplay_cpu_device;

  // Find index of compute queue family
  const uint32_t queue_family = getComputeQueueFamilyIndex(*physical_device);
  // 1.0 is highest priority queue
  const float prioritory = 1.0f;

  // Creating a logical device also requires us to create the queues associated
  // with that device.
  const VkDeviceQueueCreateInfo queue_create_info = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      NULL,
      0,
      queue_family,
      1,  // Number of queues to create
      &prioritory};

  // Vulkan differentiates between physical devices, which represent a single
  // piece of hardware, and logical devices that act as an abstraction to that
  // physical device for the application to interface with.
  const VkDeviceCreateInfo device_create_info = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      NULL,
      0,
      1,  // Size of queue create info array
      &queue_create_info,
      0,
      NULL,
      0,  // Number of device extensions to enable
      NULL,
      NULL};

  // Create our logical device.
  IS_VK_SUCCESS(
      vkCreateDevice(*physical_device, &device_create_info, 0, device));

  free(phys_devices);
}

// Loads our shader, sets up compute pipeline and command buffer, before
// finally executing vector add shader.
void buildAndRunShader(VkDevice device, uint32_t compute_queue_family,
                       VkBuffer src1_buffer, VkBuffer src2_buffer,
                       VkBuffer dst_buffer) {
  // Create our shader module, containing SPIR-V defined in header include
  const VkShaderModuleCreateInfo shader_module_info = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, 0, 0,
      vector_add_shader_size,       // Code size
      (uint32_t*)vector_add_shader  // Pointer to SPIR-V code
  };

  VkShaderModule shader_module;
  IS_VK_SUCCESS(
      vkCreateShaderModule(device, &shader_module_info, 0, &shader_module));

  // A descriptor represents a binding to a resource such as image, sampler, or
  // buffer for a shader to access. A descriptor layout defines an array of
  // descriptor bindings which may be accessed by the pipeline.
  const VkDescriptorSetLayoutBinding descriptor_set_bindings[3] = {
      {0,  // Binding number
       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
      {1,  // Binding number
       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
      {2,  // Binding number
       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0}};

  const VkDescriptorSetLayoutCreateInfo descriptor_set_create = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, 0, 0, 3,
      descriptor_set_bindings};

  VkDescriptorSetLayout descriptor_set_layout;
  IS_VK_SUCCESS(vkCreateDescriptorSetLayout(device, &descriptor_set_create, 0,
                                            &descriptor_set_layout));

  // A pipeline layout is used to access descriptor sets, describing the
  // complete set of resources than are available to a pipeline.
  const VkPipelineLayoutCreateInfo pipeline_layout_info = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      NULL,
      0,
      1,
      &descriptor_set_layout,
      0,
      NULL};

  VkPipelineLayout pipeline_layout;
  IS_VK_SUCCESS(vkCreatePipelineLayout(device, &pipeline_layout_info, 0,
                                       &pipeline_layout));

  // Create a compute pipeline running our vector add shader
  const VkPipelineShaderStageCreateInfo shader_stage_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      NULL,
      0,
      VK_SHADER_STAGE_COMPUTE_BIT,
      shader_module,
      "main",  // Shader module entry point, `main` is vector add
      NULL};

  const VkComputePipelineCreateInfo pipeline_create_info = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      NULL,
      0,
      shader_stage_create_info,
      pipeline_layout,
      VK_NULL_HANDLE,
      0};

  VkPipeline pipeline;
  IS_VK_SUCCESS(vkCreateComputePipelines(device, 0, 1, &pipeline_create_info, 0,
                                         &pipeline));

  // A descriptor pool maintains a pool of descriptors, from which descriptor
  // sets are allocated.
  const VkDescriptorPoolSize descriptor_pool_size = {
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};

  const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      NULL,
      0,
      1,  // Max number of descriptor sets that can be allocated
      1,  // Number of elements in pool size
      &descriptor_pool_size};

  VkDescriptorPool descriptor_pool;
  IS_VK_SUCCESS(vkCreateDescriptorPool(device, &descriptor_pool_create_info, 0,
                                       &descriptor_pool));

  // Allocate our descriptor set
  const VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, descriptor_pool, 1,
      &descriptor_set_layout};

  VkDescriptorSet descriptor_set;
  IS_VK_SUCCESS(vkAllocateDescriptorSets(device, &descriptor_set_alloc_info,
                                         &descriptor_set));

  // First storage input buffer has binding zero
  const VkDescriptorBufferInfo descriptor_buffer_in1 = {src1_buffer, 0,
                                                        VK_WHOLE_SIZE};
  const VkWriteDescriptorSet src1_write_descriptor = {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      NULL,
      descriptor_set,
      0,  // Binding
      0,  // Array element
      1,  // Descriptor count
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      NULL,
      &descriptor_buffer_in1,
      NULL};

  // Second storage input buffer has binding one
  const VkDescriptorBufferInfo descriptor_buffer_in2 = {src2_buffer, 0,
                                                        VK_WHOLE_SIZE};
  const VkWriteDescriptorSet src2_write_descriptor = {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      NULL,
      descriptor_set,
      1,  // Binding
      0,  // Array element
      1,  // Descriptor count
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      NULL,
      &descriptor_buffer_in2,
      NULL};

  // Output storage buffer has binding two
  const VkDescriptorBufferInfo descriptor_buffer_out = {dst_buffer, 0,
                                                        VK_WHOLE_SIZE};
  const VkWriteDescriptorSet dst_write_descriptor = {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      NULL,
      descriptor_set,
      2,  // Binding
      0,  // Array element
      1,  // Descriptor count
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      NULL,
      &descriptor_buffer_out,
      NULL};

  // Update descriptor set with buffer bindings
  VkWriteDescriptorSet write_descriptor_set[3] = {
      src1_write_descriptor, src2_write_descriptor, dst_write_descriptor};
  vkUpdateDescriptorSets(device, 3, write_descriptor_set, 0, NULL);

  // Command pools allow Vulkan to amortize the cost of resource allocation
  // when creating multiple command buffers.
  const VkCommandPoolCreateInfo command_pool_create_info = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL, 0,
      compute_queue_family  // Specifies than commands from this buffer
                            // can only be submitted to compute queues.
  };

  VkCommandPool command_pool;
  IS_VK_SUCCESS(vkCreateCommandPool(device, &command_pool_create_info, NULL,
                                    &command_pool));

  // Primary command buffers are submitted to queues, and can execute secondary
  // command buffers. Whereas secondary command buffers are executed by
  // primary command buffers rather than submitted to queues.
  const VkCommandBufferAllocateInfo command_buffer_alloc_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, command_pool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      1  // Number of command buffers to allocate
  };

  // Command buffers are used to record commands which will be submitted to the
  // device queue for execution.
  VkCommandBuffer command_buffer;
  IS_VK_SUCCESS(vkAllocateCommandBuffers(device, &command_buffer_alloc_info,
                                         &command_buffer));

  const VkCommandBufferBeginInfo command_buffer_begin_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // We're only submitting the
                                                    // command buffer once
      NULL};

  // Start recording our command buffer
  IS_VK_SUCCESS(
      vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

  // Invoke shader in a single dimension with NUM_WORK_ITEMS work groups.
  // Our shader has a local size of 1,1,1 and therefore a single work item in
  // each group.
  const uint32_t group_count_x = NUM_WORK_ITEMS;
  const uint32_t group_count_y = 1;
  const uint32_t group_count_z = 1;
  vkCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z);

  // Complete recording of command buffer, and check for errors
  IS_VK_SUCCESS(vkEndCommandBuffer(command_buffer));

  // Get our compute queue from logical device
  VkQueue queue;
  vkGetDeviceQueue(device, compute_queue_family, 0, &queue);

  const VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                    NULL,
                                    0,  // No semaphores to wait on
                                    NULL,
                                    NULL,
                                    1,  // Command buffer count
                                    &command_buffer,
                                    0,  // No semaphores to signal
                                    NULL};

  // Submit command buffer to queue
  IS_VK_SUCCESS(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

  // Waits indefinitely for all submitted commands to complete
  IS_VK_SUCCESS(vkQueueWaitIdle(queue));

  // Cleanup
  vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL); 
  vkDestroyShaderModule(device, shader_module, NULL);
  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyPipelineLayout(device, pipeline_layout, NULL);
  vkDestroyDescriptorPool(device, descriptor_pool, NULL);
  vkDestroyCommandPool(device, command_pool, NULL);
}

// Sample Vulkan compute application performing a vector add
int main() {
  puts("Vector add Vulkan compute example:");
  // Initialize the Vulkan library
  VkInstance instance = createVkInstance();

  // Find Codeplay CPU devices
  VkDevice device;
  VkPhysicalDevice physical_device;
  createVkDevice(instance, &device, &physical_device);

  // We will have 3 buffers, each containing a single int32_t per work item
  const uint32_t buffer_size = sizeof(int32_t) * NUM_WORK_ITEMS;
  const VkDeviceSize memory_size = buffer_size * 3;

  // Type and amount of memory we want to allocate
  const uint32_t memory_type_index =
      getMemoryTypeIndex(physical_device, memory_size);
  const VkMemoryAllocateInfo memory_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL,
      memory_size,  // bytes to allocate
      memory_type_index};

  // Allocate memory, to be shared among all the buffers
  VkDeviceMemory memory;
  IS_VK_SUCCESS(vkAllocateMemory(device, &memory_info, NULL, &memory));
  printf(" * Allocated %" PRIu64 " bytes of device memory\n", memory_size);

  // Map input buffers to host so that we can initialize them
  int32_t* mapped_ptr;

  // Input buffer 1 will reside in the first `buffer_size` bytes of memory, and
  // each buffer element is initialized to it's index.
  IS_VK_SUCCESS(
      vkMapMemory(device, memory, 0, buffer_size, 0, (void*)&mapped_ptr));
  for (int32_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    mapped_ptr[i] = i;
  }
  vkUnmapMemory(device, memory);

  // Input buffer 2 will be located at offset `buffer_size` in memory, and
  // each buffer element is initialized to it's index plus one.
  IS_VK_SUCCESS(vkMapMemory(device, memory, buffer_size, buffer_size, 0,
                            (void*)&mapped_ptr));
  for (int32_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    mapped_ptr[i] = i + 1;
  }
  vkUnmapMemory(device, memory);

  // Find compute queue family index
  const uint32_t queue_family = getComputeQueueFamilyIndex(physical_device);
  // All our buffers will be of the same size
  const VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      NULL,
      0,
      buffer_size,                         // Size in bytes of buffer
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,  // These are storage buffers
      VK_SHARING_MODE_EXCLUSIVE,           // Buffer won't overlap in memory
      1,
      &queue_family};

  // Bind the first input buffer to the first `buffer_size` bytes of memory
  VkBuffer src1_buffer;
  IS_VK_SUCCESS(
      vkCreateBuffer(device, &buffer_create_info, NULL, &src1_buffer));
  IS_VK_SUCCESS(vkBindBufferMemory(device, src1_buffer, memory, 0));

  // Bind the second input buffer to memory at offset `buffer_size`
  VkBuffer src2_buffer;
  IS_VK_SUCCESS(
      vkCreateBuffer(device, &buffer_create_info, NULL, &src2_buffer));
  IS_VK_SUCCESS(vkBindBufferMemory(device, src2_buffer, memory, buffer_size));

  // Bind the output buffer to memory at offset 2 x `buffer_size`
  VkBuffer dst_buffer;
  IS_VK_SUCCESS(vkCreateBuffer(device, &buffer_create_info, NULL, &dst_buffer));
  IS_VK_SUCCESS(
      vkBindBufferMemory(device, dst_buffer, memory, 2 * buffer_size));

  printf(" * Created input & output buffers\n");

  // Build our vector add shader and run with our buffers on target device
  buildAndRunShader(device, queue_family, src1_buffer, src2_buffer, dst_buffer);

  // Map our output buffer back to host memory
  IS_VK_SUCCESS(vkMapMemory(device, memory, 2 * buffer_size, buffer_size, 0,
                            (void*)&mapped_ptr));

  // Verify our results
  for (int32_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    const int32_t reference = i + (i + 1);
    if (mapped_ptr[i] != reference) {
      printf("Result mismatch for index %d\n", i);
      printf("Got %d, but expected %d\n", mapped_ptr[i], reference);
      exit(1);
    }
  }
  vkUnmapMemory(device, memory);

  printf(" * Successfully validated result\n");

  // Cleanup
  vkFreeMemory(device, memory, NULL);
  vkDestroyBuffer(device, src1_buffer, NULL);
  vkDestroyBuffer(device, src2_buffer, NULL);
  vkDestroyBuffer(device, dst_buffer, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroyInstance(instance, NULL);
  printf(" * Released all created Vulkan objects\n");

  printf("\nExample ran successfully, exiting\n");

  return 0;
}
