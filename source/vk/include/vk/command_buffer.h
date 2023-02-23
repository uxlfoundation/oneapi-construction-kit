// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_COMMAND_BUFFER_H_INCLUDED
#define VK_COMMAND_BUFFER_H_INCLUDED

#include <compiler/kernel.h>
#include <compiler/module.h>
#include <mux/mux.hpp>
#include <vk/icd.h>
#include <vk/physical_device.h>
#include <vk/small_vector.h>

#include <array>
#include <mutex>

namespace vk {
/// @copydoc ::vk::command_pool_t
typedef struct command_pool_t *command_pool;

/// @copydoc ::vk::descriptor_set_t
typedef struct descriptor_set_t *descriptor_set;

/// @copydoc ::vk::buffer_t
typedef struct buffer_t *buffer;

/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::event_t
typedef struct event_t *event;

/// @copydoc ::vk::command_pool_t
typedef struct command_pool_t *command_pool;

/// @copydoc ::vk::image_t
typedef struct image_t *image;

/// @copydoc ::vk::pipeline_t
typedef struct pipeline_t *pipeline;

/// @copydoc ::vk::pipeline_layout_t
typedef struct pipeline_layout_t *pipeline_layout;

/// @copydoc ::vk::query_pool_t
typedef struct query_pool_t *query_pool;

/// @brief All supported commands
///
/// @see ::vk::command_info::type
enum command_type {
  command_type_bind_pipeline,
  command_type_bind_descriptorset,
  command_type_dispatch,
  command_type_dispatch_indirect,
  command_type_copy_buffer,
  command_type_update_buffer,
  command_type_fill_buffer,
  command_type_set_event,
  command_type_reset_event,
  command_type_wait_events,
  command_type_push_constants,
  command_type_pipeline_barrier
};

/// @brief argument information for vkCmdBindPipeline
struct command_info_bind_pipeline {
  /// @brief Pipeline to bind
  vk::pipeline pipeline;
};

/// @brief argument information for vkCmdBindDescriptorSets
struct command_info_bind_descriptorset {
  /// @brief Pipeline layout object used to program the bindings
  vk::pipeline_layout layout;
  /// @brief Set number of the first descriptor set to be bound
  uint32_t firstSet;
  /// @brief The number of descriptor_set objects in pDescriptorSets
  uint32_t descriptorSetCount;
  /// @brief List of handles to the descriptor sets to be bound
  VkDescriptorSet *pDescriptorSets;
  /// @brief The number of elements in pDynamicOffsets
  uint32_t dynamicOffsetCount;
  /// @brief Values specifying dynamic offsets
  const uint32_t *pDynamicOffsets;
};

/// @brief argument information for vkCmdDispatch
struct command_info_dispatch {
  /// @brief X dimension of the workgroup to dispatch
  uint32_t x;
  /// @brief Y dimension of the workgroup to dispatch
  uint32_t y;
  /// @brief Z dimension of the workgroup to dispatch
  uint32_t z;
};

/// @brief argument information for vkCmdDispatchIndirect
struct command_info_dispatch_indirect {
  /// @brief Buffer in which the dispatch info can be found
  vk::buffer buffer;
  /// @brief Offset into the buffer at which the dispatch info can be found
  VkDeviceSize offset;
};

/// @brief argument information for  vkCmdCopyBuffer
struct command_info_copy_buffer {
  /// @brief The buffer to copy from
  vk::buffer srcBuffer;
  /// @brief The buffer to copy to
  vk::buffer dstBuffer;
  /// @brief Length of pRegions
  uint32_t regionCount;
  /// @brief Array of VkBufferCopy structures that specify offsets and
  /// ranges for the copy operations
  const VkBufferCopy *pRegions;
};

/// @brief argument information for vkCmdUpdatebuffer
struct command_info_update_buffer {
  /// @brief The buffer to update
  vk::buffer dstBuffer;
  /// @brief The offset from the start of dstBuffer to update from
  VkDeviceSize dstOffset;
  /// @brief Size in bytes of region within the buffer to update
  VkDeviceSize dataSize;
  /// @brief Data to update the buffer with
  const void *pData;
};

/// @brief argument information for vkCmdFillBuffer
struct command_info_fill_buffer {
  /// @brief The buffer to fill
  vk::buffer dstBuffer;
  /// @brief Offset into the buffer to start filling from
  VkDeviceSize dstOffset;
  /// @brief Range of the buffer to fill
  VkDeviceSize size;
  /// @brief 4 byte word to be written repeatedly to the buffer
  uint32_t data;
};

/// @brief argument information for vkCmdSetEvent
struct command_info_set_event {
  ///@brief Event to be set
  vk::event event;
  /// @brief Pipeline stage at which the event will be set
  VkPipelineStageFlags stageMask;
};

/// @brief argument information for vkCmdResetEvent
struct command_info_reset_event {
  /// @brief The event to be reset
  vk::event event;
  /// @brief Pipeline stage at which the event will be reset
  VkPipelineStageFlags stageMask;
};

/// @brief argument information for vkCmdWaitEvents
struct command_info_wait_events {
  /// @brief Length of `pEvents`
  uint32_t eventCount;
  /// @brief Array of event objects to wait for
  VkEvent *pEvents;
  /// @brief Stage flags encoding which set event operations to wait for
  VkPipelineStageFlags srcStageMask;
  /// @brief Stage flags encoding which stages need to wait for the events
  VkPipelineStageFlags dstStageMask;
  /// @brief Length of `pMemoryBarriers`
  uint32_t memoryBarrierCount;
  /// @brief Array of structures specifying memory barriers
  const VkMemoryBarrier *pMemoryBarriers;
  /// @brief Length of pBufferMemoryBarriers
  uint32_t bufferMemoryBarrierCount;
  /// @brief Array of structures specifying buffer memory barriers
  const VkBufferMemoryBarrier *pBufferMemoryBarriers;
  /// @brief Length of pImageMemoryBarriers
  uint32_t imageMemoryBarrierCount;
  /// @brief Array of structures specifying image memory barriers
  const VkImageMemoryBarrier *pImageMemoryBarriers;
};

/// @brief argument information for vkCmdPushConstants
struct command_info_push_constants {
  /// @brief Pipeline layout used to program the push constant ranges
  vk::pipeline_layout pipelineLayout;
  /// @brief Offset into the push constant buffer these values are to be
  /// written to
  uint32_t offset;
  /// @brief Size in bytes of the values being written to the buffer
  uint32_t size;
  /// @brief Values to be written to the push constant buffer
  const void *pValues;
};

/// @brief argument information for vkCmdPipelineBarrier
struct command_info_pipeline_barrier {
  /// @brief Bitmask of stages in the first half of the dependency
  VkPipelineStageFlags srcStageMask;
  /// @brief Bitmask of stages in the second half of the dependency
  VkPipelineStageFlags dstStageMask;
  /// @brief Additional flags, currently irrelevant to compute
  VkDependencyFlags dependencyFlags;
  /// @brief Length of `pMemoryBarriers`
  uint32_t memoryBarrierCount;
  /// @brief Array of structs specifying memory barriers
  const VkMemoryBarrier *pMemoryBarriers;
  /// @brief Length of `pBufferMemoryBarriers`
  uint32_t bufferMemoryBarrierCount;
  /// @brief Array of structs specifying buffer memory barriers
  const VkBufferMemoryBarrier *pBufferMemoryBarriers;
  /// @brief Length of `pImageMemoryBarriers`
  uint32_t imageMemoryBarrierCount;
  /// @brief Array of structs specifying image memory barriers
  const VkImageMemoryBarrier *pImageMemoryBarriers;
};

/// @brief struct for storing information about commands submitted to a
/// secondary command buffer
struct command_info {
  command_info(command_info_bind_pipeline bind_pipeline_command)
      : type(command_type_bind_pipeline),
        stage_flag(VK_PIPELINE_STAGE_HOST_BIT),
        bind_pipeline_command(bind_pipeline_command) {}
  command_info(command_info_bind_descriptorset bind_descriptorset_command)
      : type(command_type_bind_descriptorset),
        stage_flag(VK_PIPELINE_STAGE_HOST_BIT),
        bind_descriptorset_command(bind_descriptorset_command) {}
  command_info(command_info_dispatch dispatch_command)
      : type(command_type_dispatch),
        stage_flag(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
        dispatch_command(dispatch_command) {}
  command_info(command_info_dispatch_indirect dispatch_indirect_command)
      : type(command_type_dispatch_indirect),
        stage_flag(0),
        dispatch_indirect_command(dispatch_indirect_command) {}
  command_info(command_info_copy_buffer copy_buffer_command)
      : type(command_type_copy_buffer),
        stage_flag(VK_PIPELINE_STAGE_TRANSFER_BIT),
        copy_buffer_command(copy_buffer_command) {}
  command_info(command_info_update_buffer update_buffer_command)
      : type(command_type_update_buffer),
        stage_flag(VK_PIPELINE_STAGE_TRANSFER_BIT),
        update_buffer_command(update_buffer_command) {}
  command_info(command_info_fill_buffer fill_buffer_command)
      : type(command_type_fill_buffer),
        stage_flag(VK_PIPELINE_STAGE_TRANSFER_BIT),
        fill_buffer_command(fill_buffer_command) {}
  command_info(command_info_set_event set_event_command)
      : type(command_type_set_event),
        stage_flag(set_event_command.stageMask),
        set_event_command(set_event_command) {}
  command_info(command_info_reset_event reset_event_command)
      : type(command_type_reset_event),
        stage_flag(reset_event_command.stageMask),
        reset_event_command(reset_event_command) {}
  command_info(command_info_wait_events wait_events_command)
      : type(command_type_wait_events),
        stage_flag(wait_events_command.srcStageMask),
        wait_events_command(wait_events_command) {}
  command_info(command_info_push_constants push_constants_command)
      : type(command_type_push_constants),
        stage_flag(0),
        push_constants_command(push_constants_command) {}
  command_info(command_info_pipeline_barrier pipeline_barrier_command)
      : type(command_type_pipeline_barrier),
        stage_flag(VK_PIPELINE_STAGE_HOST_BIT),
        pipeline_barrier_command(pipeline_barrier_command) {}

  /// @brief enum denoting which command the info stored concerns
  command_type type;

  /// @brief pipeline stage flag denoting which stage this command runs in
  VkPipelineStageFlags stage_flag;

  /// @brief union of the structs containing the actual command info
  union {
    command_info_bind_pipeline bind_pipeline_command;
    command_info_bind_descriptorset bind_descriptorset_command;
    command_info_dispatch dispatch_command;
    command_info_dispatch_indirect dispatch_indirect_command;
    command_info_copy_buffer copy_buffer_command;
    command_info_update_buffer update_buffer_command;
    command_info_fill_buffer fill_buffer_command;
    command_info_set_event set_event_command;
    command_info_reset_event reset_event_command;
    command_info_wait_events wait_events_command;
    command_info_push_constants push_constants_command;
    command_info_pipeline_barrier pipeline_barrier_command;
  };
};

/// @brief Possible types of a `command_buffer_info`
enum command_type_e { COMPUTE, INITIAL, TRANSFER };

/// @brief Struct containing a mux command buffer used to guarantee execution
/// dependencies of a pipeline barrier
typedef struct barrier_group_info_t {
  barrier_group_info_t(mux_command_buffer_t command_buffer, mux_fence_t fence,
                       mux_semaphore_t semaphore, VkPipelineStageFlags src_mask,
                       VkPipelineStageFlags dst_mask,
                       VkPipelineStageFlags stage_flags,
                       vk::allocator allocator)
      : command_buffer(command_buffer),
        fence(fence),
        semaphore(semaphore),
        src_mask(src_mask),
        dst_mask(dst_mask),
        stage_flags(stage_flags),
        user_wait_flags(0),
        dispatched(false),
        commands(
            {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

  /// @brief Mux command buffer
  mux_command_buffer_t command_buffer;

  /// @brief Mux fence
  mux_fence_t fence;

  /// @brief Semaphore that will be signaled
  mux_semaphore_t semaphore;

  /// @brief Source stage mask for this pipeline barrier
  VkPipelineStageFlags src_mask;

  /// @brief Destination stage mask for this pipeline barrier
  VkPipelineStageFlags dst_mask;

  /// @brief Pipeline stage flags representing what has actually been recorded
  /// into `command_buffer`
  VkPipelineStageFlags stage_flags;

  /// @brief Flags potentially set by user semaphore/event operations that
  /// mandate waiting on additional semaphores
  VkPipelineStageFlags user_wait_flags;

  /// @brief Whether `command_buffer` has ever been dispatched
  bool dispatched;

  /// @brief List of commands recorded to execute on `command_buffer`
  vk::small_vector<command_info, 4> commands;

  /// @brief Equality operator
  bool operator==(const barrier_group_info_t &other) const {
    return command_buffer == other.command_buffer && fence == other.fence &&
           semaphore == other.semaphore && src_mask == other.src_mask &&
           dst_mask == other.dst_mask && stage_flags == other.stage_flags &&
           user_wait_flags == other.user_wait_flags;
  }

  /// @brief Inequality operator
  bool operator!=(const barrier_group_info_t &other) const {
    return !(*this == other);
  }
} * barrier_group_info;

/// @brief Semaphore/pipeline stage flags pair struct
struct semaphore_flags_pair {
  mux_semaphore_t semaphore;
  VkPipelineStageFlags flags;
};

/// @brief Mux command buffer/semaphore/fence struct
struct command_buffer_semaphore_fence_tuple {
  mux_command_buffer_t command_buffer;
  mux_semaphore_t semaphore;
  mux_fence_t fence;
};

/// @brief Buffer and memory pair struct
struct buffer_memory_pair {
  mux_buffer_t buffer;
  mux_memory_t memory;
};

/// @brief internal command_buffer type
typedef struct command_buffer_t final : icd_t<command_buffer_t> {
  /// @brief constructor for primary command buffers
  /// @param command_pool_create_flags Flags passed at the creation of this
  /// command buffer's command pool
  /// @param mux_device Mux device that owns this command buffer
  /// @param command_buffer Mux command buffer commands not synchronized with a
  ///        pipeline barrier will be recorded into
  /// @param semaphore Semaphore `command_buffer` will signal when done
  /// @param allocator `vk::allocator` used to initialize `descriptor_sets`
  /// and `commands`
  command_buffer_t(VkCommandPoolCreateFlags command_pool_create_flags,
                   mux_device_t mux_device,
                   mux::unique_ptr<mux_command_buffer_t> command_buffer,
                   mux::unique_ptr<mux_fence_t> fence,
                   mux::unique_ptr<mux_semaphore_t> semaphore,
                   vk::allocator allocator);

  /// @brief constructor for secondary command buffers
  /// it is assumed that if no mux mux command buffer is provided this will be a
  /// secondary command buffer
  /// @param command_pool_create_flags create flags
  /// @param allocator `vk::allocator` used to initialize `descriptor_sets`
  /// and `commands`
  command_buffer_t(VkCommandPoolCreateFlags command_pool_create_flags,
                   vk::allocator allocator);

  /// @brief destructor
  ~command_buffer_t();

  /// @brief whether this command buffer is primary or secondary
  VkCommandBufferLevel command_buffer_level;

  /// @brief the flags provided when the command pool this command buffer was
  /// allocated from was created
  VkCommandPoolCreateFlags command_pool_create_flags;

  /// @brief Flags which indicate how this command buffer will be used
  VkCommandBufferUsageFlags usage_flags;

  /// @brief the list of argument descriptors to be passed when creating
  /// specialized kernel
  vk::small_vector<vk::descriptor_set, 4> descriptor_sets;

  /// @brief the state of the command buffer
  enum {
    initial,     ///< Initial state for newly created command buffers
    recording,   ///< Used when a command buffer has started recording
    executable,  ///< Used when a command buffer has finished recording
    resolving,   ///< Special state for dealing with pipeline barriers
    pending,     ///< Used when a command buffer has just been submitted
    invalid  ///< Invalid state for one time submit command buffers finishing
  } state;

  /// @brief since the command calls return a value, store error state here and
  /// check it at endCommandBuffer
  VkResult error;

  /// @brief Allocator stored for object internal allocations.
  vk::allocator allocator;

  /// @brief reference to the mux device that owns this command buffer, needed
  /// to create the various kernel objects
  mux_device_t mux_device;

  /// @brief compiler kernel provided when a pipeline is bound, to be used for
  /// specialized kernel creation later
  compiler::Kernel *compiler_kernel;

  /// @brief Mux binary kernel provided when a pipeline based on a cached shader
  /// is bound
  mux_kernel_t mux_binary_kernel;

  /// @brief Local work group size copied in from bound pipeline
  std::array<uint32_t, 3> wgs;

  /// @brief List of push constant buffer and memory objects created when a
  /// dispatch command is recorded
  vk::small_vector<buffer_memory_pair, 2> push_constant_objects;

  /// @brief A struct representing a recorded kernel.
  struct recorded_kernel {
    vk::small_vector<mux_descriptor_info_t, 4> descriptors;
    std::array<size_t, 3> local_size;
    std::array<size_t, 3> global_offset;
    std::array<size_t, 3> global_size;
    mux_kernel_t mux_binary_kernel;
    mux::unique_ptr<mux_executable_t> specialized_kernel_executable;
    mux::unique_ptr<mux_kernel_t> specialized_kernel;

    /// @brief Constructor.
    recorded_kernel(vk::allocator allocator);

    /// @brief Return kernel to be passed to muxCommandNDRange.
    mux_kernel_t getMuxKernel();

    /// @brief Generate Mux options to be passed to both the compiler and
    /// muxCommandNDRange.
    mux_ndrange_options_t getMuxNDRangeOptions();
  };

  /// @brief List of specialized kernel objects created when a dispatch command
  /// is recorded
  vk::small_vector<recorded_kernel, 2> specialized_kernels;

  /// @brief List of specialized kernels that have been dispatched
  ///
  /// We need to keep them around to properly dispose of them when they're done
  vk::small_vector<recorded_kernel, 2> dispatched_kernels;

  /// @brief List of set/binding pairs used in the bound pipeline's kernel
  vk::small_vector<compiler::spirv::DescriptorBinding, 2> shader_bindings;

  /// @brief list that commands pushed to this command buffer will be
  /// stored in
  vk::small_vector<command_info, 4> commands;

  /// @brief Mux command buffer commands without any barrier dependencies go in
  mux_command_buffer_t main_command_buffer;

  /// @brief Fence to signal host when device completes execution of
  /// `main_command_buffer`
  mux_fence_t main_fence;

  /// @brief Sempahore signalled when `main_command_buffer` has run to
  /// completion
  mux_semaphore_t main_semaphore;

  /// @brief Stage flags mask that represents what sort of commands will run
  /// as part of `main_command_buffer`
  VkPipelineStageFlags main_command_buffer_stage_flags;

  /// @brief Encodes whether main mux command buffer is obligated to wait for
  /// semaphores by any event in a given stage
  VkPipelineStageFlags main_command_buffer_event_wait_flags;

  /// @brief Whether `main_command_buffer` has been dispatched
  bool main_dispatched;

  /// @brief List of mux command buffer copies made to allow simultaneous use
  vk::small_vector<command_buffer_semaphore_fence_tuple, 2>
      simultaneous_use_list;

  /// @brief pointer to the mux command buffer compute commands should be put in
  mux_command_buffer_t compute_command_buffer;

  /// @brief Pointer to the stage flags compute submissions should effect
  VkPipelineStageFlags *compute_stage_flags;

  /// @brief Pointer to the list of commands that compute commands are added to
  vk::small_vector<command_info, 4> *compute_command_list;

  /// @brief pointer to the mux command buffer transfer commands should be put
  /// in
  mux_command_buffer_t transfer_command_buffer;

  /// @brief Pointer to the stage flags compute submissions should effect
  VkPipelineStageFlags *transfer_stage_flags;

  /// @brief Pointer to the list of commands that transfer commands are added to
  vk::small_vector<command_info, 4> *transfer_command_list;

  /// @brief List of structs that contain the pipeline barrier mux command
  /// buffers
  vk::small_vector<barrier_group_info, 2> barrier_group_infos;

  /// @brief Semaphores that correspond to mux command buffers that have wait
  /// events
  ///
  /// And the `dstStageMask` of the wait operation
  vk::small_vector<semaphore_flags_pair, 2> wait_events_semaphores;

  /// @brief mux descriptor info struct containing the push constant buffer
  mux_descriptor_info_t push_constant_descriptor_info;

  /// @brief push constant storage
  ///
  /// This is what we copy values into during a CmdPushConstants command
  std::array<std::uint8_t, CA_VK_MAX_PUSH_CONSTANTS_SIZE> push_constants;

  /// @brief total size in bytes of the buffer needed for push constants
  ///
  /// Note that this is only known once the pipeline is bound
  std::uint32_t total_push_constant_size;

  /// @brief References retained to memory allocs backing the descriptor size
  /// buffers created for dispatch commands.
  ///
  /// These must be retained while in use, and destroyed when the command buffer
  /// is destroyed or reset.
  vk::small_vector<mux_memory_t, 2> descriptor_size_memory_allocs;

  /// @brief Buffers containing the sizes of all the buffers passed to a kernel
  /// from a dispatch command.
  ///
  /// One of these will be created for each dispatch command recorded to the
  /// command buffer, they must be retained while the command buffer is
  /// executing and cleaned up when the command buffer is destroyed or reset.
  vk::small_vector<mux_buffer_t, 2> descriptor_size_buffers;

  /// @brief mux descriptor info for `descriptor_size_buffer`
  mux_descriptor_info_t descriptor_size_descriptor_info;

  /// @brief Mutex used to lock during access to certain members that can be
  /// accessed from multiple threads
  std::mutex mutex;
} * command_buffer;

/// @brief Internal implementation of allocateCommandBuffers
///
/// @param device device on which the command buffers are to be allocated
/// @param pAllocateInfo array of allocate info structures
/// @param pCommandBuffers return created command buffers
///
/// @return Vulkan result code
VkResult AllocateCommandBuffers(
    vk::device device, const VkCommandBufferAllocateInfo *pAllocateInfo,
    vk::command_buffer *pCommandBuffers);

/// @brief internal implementation of FreeCommandBuffers
///
/// @param device device the command buffers were created on
/// @param commandPool command pool from which the command buffer was allocated
/// @prarm commandBufferCount length of pCommandBuffers
/// @param pCommandBuffers array of command buffers to free
void FreeCommandBuffers(vk::device device, vk::command_pool commandPool,
                        uint32_t commandBufferCount,
                        const vk::command_buffer *pCommandBuffers);

/// @brief internal implementation of vkResetCommandBuffer
///
/// @param commandBuffer command buffer to reset
/// @prarm flags bitmask which controls the reset operation
///
/// @return Vulkan result code
VkResult ResetCommandBuffer(vk::command_buffer commandBuffer,
                            VkCommandBufferResetFlags flags);

/// @brief internal implementation of vkBeginCommandBuffer
///
/// @param commandBuffer the command buffer to begin recording
/// @param pBeginInfo begin info structure
///
/// @return Vulkan result code
VkResult BeginCommandBuffer(vk::command_buffer commandBuffer,
                            const VkCommandBufferBeginInfo *pBeginInfo);

/// @brief internal implementation of vkEndCommandBuffer
///
/// @param commandBuffer command buffer to finish recording
///
/// @return Vulkan result code
VkResult EndCommandBuffer(vk::command_buffer commandBuffer);

/// @brief function for executing command encoded by a `command_info` struct
///
/// @param commandBuffer the `command_buffer` to execute the command on
/// @param command_info `command_info` struct that encodes the command
void ExecuteCommand(vk::command_buffer commandBuffer,
                    const vk::command_info &command_info);

/// @brief internal implementation of vkCmdCopyBuffer
///
/// @param commandBuffer the command buffer the command will be recorded to
/// @param srcBuffer the buffer to copy from
/// @param dstBuffer the buffer to copy to
/// @param regionCount the length of pRegions
/// @param pRegions array of VkBufferCopy structures that specify offsets and
/// ranges for the copy operations
void CmdCopyBuffer(vk::command_buffer commandBuffer, vk::buffer srcBuffer,
                   vk::buffer dstBuffer, uint32_t regionCount,
                   const VkBufferCopy *pRegions);

/// @brief internal implementation of vkCmdUpdateBuffer
///
/// @param commandBuffer the command buffer the command will be recorded to
/// @param dstBuffer the buffer to update
/// @param dstOffset the offset from the start of dstBuffer to update from
/// @param dataSize the size in bytes to update
/// @param pData data to update the buffer with
void CmdUpdateBuffer(vk::command_buffer commandBuffer, vk::buffer dstBuffer,
                     VkDeviceSize dstOffset, VkDeviceSize dataSize,
                     const void *pData);

/// @brief internal implementation of vkCmdFillBuffer
///
/// @param commandBuffer the command buffer the command will be recorded to
/// @param dstBuffer the buffer to fill
/// @param dstOffset offset into the buffer to start filling from
/// @param size range of the buffer to fill
/// @param data the 4 byte word to be written repeatedly to the buffer
void CmdFillBuffer(vk::command_buffer commandBuffer, vk::buffer dstBuffer,
                   VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);

/// @brief internal implementation of vkCmdResetEvent
///
/// @param commandBuffer command buffer the command will be recorded to
/// @param event the event to be reset
/// @param stageMask the pipeline stage at which the event will be reset
void CmdResetEvent(vk::command_buffer commandBuffer, vk::event event,
                   VkPipelineStageFlags stageMask);

/// @brief internal implementation of vkCmdBindPipeline
///
/// @param commandBuffer the command buffer to bind the pipeline to
/// @param pipelineBindPoint specifies if this is a compute or graphics pipeline
/// @param pipeline pipeline to be bound
void CmdBindPipeline(vk::command_buffer commandBuffer,
                     VkPipelineBindPoint pipelineBindPoint,
                     vk::pipeline pipeline);

/// @brief internal implementation of vkCmdDispatch
///
/// @param commandBuffer the command buffer into which the command will be
/// recorded
/// @param x the x dimension of the global workgroup size
/// @param y the y dimension of the global workgroup size
/// @param z the z dimension of the global workgroup size
void CmdDispatch(vk::command_buffer commandBuffer, uint32_t x, uint32_t y,
                 uint32_t z);

/// @brief internal implementation of vkCmdBindDescriptorSets
///
/// @param commandBuffer the command buffer to bind to
/// @param pipelineBindPoint whether the descriptors will be used by graphics or
/// compute pipelines
/// @param layout pipeline_layout object used to program the bindings
/// @param firstSet set number of the first descriptor set to be bound
/// @param descriptorSetCount the number of descriptor_set objects in
/// pDescriptorSets
/// @oaram pDescriptorSets the descriptor sets to be bound
/// @param dynamicOffsetCount the number of elements in pDynamicOffsets
/// @param pDynamicOffsets values specifying dynamic offsets
void CmdBindDescriptorSets(vk::command_buffer commandBuffer,
                           VkPipelineBindPoint pipelineBindPoint,
                           vk::pipeline_layout layout, uint32_t firstSet,
                           uint32_t descriptorSetCount,
                           const VkDescriptorSet *pDescriptorSets,
                           uint32_t dynamicOffsetCount,
                           const uint32_t *pDynamicOffsets);

/// @brief internal implementation of vkCmdExecuteCommands
///
/// @param commandBuffer the command buffer being recorded
/// @param commandBufferCount the length of pCommandBuffers
/// @param pCommandBuffers the secondary command buffers to be executed
void CmdExecuteCommands(vk::command_buffer commandBuffer,
                        uint32_t commandBufferCount,
                        const vk::command_buffer *pCommandBuffers);

/// @brief internal implementation of cmdSetEvent
///
/// @param commandBuffer the command buffer the command will be recorded to
/// @param event the event to be set
/// @param stageMask the pipeline stage at which the event will be set
void CmdSetEvent(vk::command_buffer commandBuffer, vk::event event,
                 VkPipelineStageFlags stageMask);

/// @brief internal implementation of vkCmdPushConstants
///
/// @param commandBuffer command buffer the command will be recorded to
/// @param pipelineLayout pipeline layout used to program the push constant
/// ranges
/// @param stageFlags bitmask specifying the shader stage these push constants
/// will be available in (this implementation only supports one, so largely
/// meaningless)
/// @param offset offset into the push constant buffer these values are to be
/// written to
/// @param size size in bytes of the values being written to the buffer
/// @param pValues values to be written to the buffer
void CmdPushConstants(vk::command_buffer commandBuffer,
                      vk::pipeline_layout pipelineLayout,
                      VkShaderStageFlags stageFlags, uint32_t offset,
                      uint32_t size, const void *pValues);

/// @brief Internal implementation of vkCmdPipelineBarrier
///
/// @param commandBuffer Command buffer the command will be recorded into
/// @param srcStageMask Bitmask of stages in the first half of the dependency
/// @param dstStageMask Bitmask of stages in the second half of the dependency
void CmdPipelineBarrier(vk::command_buffer commandBuffer,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask, VkDependencyFlags,
                        uint32_t, const VkMemoryBarrier *, uint32_t,
                        const VkBufferMemoryBarrier *, uint32_t,
                        const VkImageMemoryBarrier *);

/// @brief Internal implementation of vkCmdWaitEvents
///
/// @param commandBuffer Command buffer the command will be recorded into
/// @param eventCount Length of `pEvents`
/// @param pEvents List of events the command buffer will wait on
/// @param srcStageMask Stage mask specifying the first synchronisation scope
/// @param dstStageMask Stage mask specifying the second synchronisation scope
void CmdWaitEvents(vk::command_buffer commandBuffer, uint32_t eventCount,
                   const VkEvent *pEvents, VkPipelineStageFlags srcStageMask,
                   VkPipelineStageFlags dstStageMask, uint32_t,
                   const VkMemoryBarrier *, uint32_t,
                   const VkBufferMemoryBarrier *, uint32_t,
                   const VkImageMemoryBarrier *);

/// @brief Stub of vkCmdDispatchIndirect
void CmdDispatchIndirect(vk::command_buffer commandBuffer, vk::buffer buffer,
                         VkDeviceSize offset);

/// @brief Stub of vkCmdCopyImage
void CmdCopyImage(vk::command_buffer commandBuffer, vk::image srcImage,
                  VkImageLayout srcImageLayout, vk::image dstImage,
                  VkImageLayout dstImageLayout, uint32_t regionCount,
                  const VkImageCopy *pRegions);

/// @brief Stub of vkCmdCopyBufferToImage
void CmdCopyBufferToImage(vk::command_buffer commandBuffer,
                          vk::buffer srcBuffer, vk::image dstImage,
                          VkImageLayout dstImageLayout, uint32_t regionCount,
                          const VkBufferImageCopy *pRegions);

/// @brief Stub of vkCmdCopyImageToBuffer
void CmdCopyImageToBuffer(vk::command_buffer commandBuffer, vk::image srcImage,
                          VkImageLayout srcImageLayout, vk::buffer dstBuffer,
                          uint32_t regionCount,
                          const VkBufferImageCopy *pRegions);

/// @brief Stub of vkCmdClearColorImage
void CmdClearColorImage(vk::command_buffer commandBuffer, vk::image image,
                        VkImageLayout imageLayout,
                        const VkClearColorValue *pColor, uint32_t rangeCount,
                        const VkImageSubresourceRange *pRanges);

/// @brief Stub of vkCmdBeginQuery
void CmdBeginQuery(vk::command_buffer commandBuffer, vk::query_pool queryPool,
                   uint32_t query, VkQueryControlFlags flags);

/// @brief Stub of vkCmdEndQuery
void CmdEndQuery(vk::command_buffer commandBuffer, vk::query_pool queryPool,
                 uint32_t query);

/// @brief Stub of vkCmdResetQueryPool
void CmdResetQueryPool(vk::command_buffer commandBuffer,
                       vk::query_pool queryPool, uint32_t firstQuery,
                       uint32_t queryCount);

/// @brief Stub of vkCmdWriteTimeStamp
void CmdWriteTimestamp(vk::command_buffer commandBuffer,
                       VkPipelineStageFlagBits pipelineStage,
                       vk::query_pool queryPool, uint32_t query);

/// @brief Stub of vkCmdCopyQueryPoolResults
void CmdCopyQueryPoolResults(vk::command_buffer commandBuffer,
                             vk::query_pool queryPool, uint32_t firstQuery,
                             uint32_t queryCount, VkBuffer dstBuffer,
                             VkDeviceSize dstOffset, VkDeviceSize stride,
                             VkQueryResultFlags flags);
}  // namespace vk

#endif  // VK_COMMAND_BUFFER_H_INCLUDED
