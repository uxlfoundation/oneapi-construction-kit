// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler kernel API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_COMPILER_KERNEL_H_INCLUDED
#define HOST_COMPILER_KERNEL_H_INCLUDED

#include <base/kernel.h>
#include <compiler/module.h>
#include <host/common/jit_kernel.h>

#include <map>

#include "base/module.h"

namespace llvm {
class Module;
}

namespace host {
class HostTarget;

/// @brief An object that represents a kernel who's compilation has been
/// deferred.
struct OptimizedKernel {
  /// @brief A weak reference to a module whose lifetime is managed by
  /// `HostTarget` (as part of `llvm::ExecutionEngine`). In `~HostKernel`, we
  /// tell the execution engine to free these modules.
  llvm::Module *optimized_module;

  /// @brief The JIT kernel metadata, stored in a unique_ptr to guarantee
  /// pointer stability.
  std::unique_ptr<::host::jit_kernel_s> binary_kernel;
};

class HostKernel : public compiler::BaseKernel {
 public:
  HostKernel(HostTarget &target, compiler::Options &build_options,
             cargo::array_view<compiler::BaseModule::SnapshotDetails> snapshots,
             llvm::Module *module, std::string name,
             std::array<size_t, 3> preferred_local_sizes,
             size_t local_memory_used);

  ~HostKernel();

  /// @see Kernel::precacheLocalSize
  compiler::Result precacheLocalSize(size_t local_size_x, size_t local_size_y,
                                     size_t local_size_z) override;

  /// @see Kernel::getDynamicWorkWidth
  cargo::expected<uint32_t, compiler::Result> getDynamicWorkWidth(
      size_t local_size_x, size_t local_size_y, size_t local_size_z) override;

  /// @see Kernel::createSpecializedKernel
  cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result>
  createSpecializedKernel(
      const mux_ndrange_options_t &specialization_options) override;

  /// @brief No-op implementation indicating sub-groups are not supported.
  cargo::expected<uint32_t, compiler::Result> querySubGroupSizeForLocalSize(
      size_t local_size_x, size_t local_size_y, size_t local_size_z) override;

  /// @brief No-op implementation indicating sub-groups are not supported.
  cargo::expected<std::array<size_t, 3>, compiler::Result>
  queryLocalSizeForSubGroupCount(size_t sub_group_size) override;

  /// @brief No-op implementation indicating sub-groups are not supported.
  cargo::expected<size_t, compiler::Result> queryMaxSubGroupCount() override;

 private:
  /// @brief Gets an `OptimizedKernel` object for the given local size.
  ///
  /// @param local_size Local size to optimize the kernel for.
  cargo::expected<const OptimizedKernel &, compiler::Result>
  lookupOrCreateOptimizedKernel(std::array<size_t, 3> local_size);

  /// @brief LLVM module containing only the kernel function and functions it
  /// calls, not yet optimized for a local size.
  llvm::Module *module;

  /// @brief Map of optimized modules to their local sizes.
  ///
  /// By an "optimized module" we mean a copy of this kernel's LLVM module which
  /// has had passes that optimize for a specific local size run on it.
  std::map<std::array<size_t, 3>, OptimizedKernel> optimized_kernel_map;

  /// @brief Target object that created the module this kernel is derived from.
  HostTarget &target;

  /// @brief Build options passed to the module this kernel was created from.
  compiler::Options &build_options;

  /// @brief A reference to the snapshot state in the module.
  cargo::array_view<compiler::BaseModule::SnapshotDetails> snapshots;
};
}  // namespace host

#endif  // HOST_KERNEL_H_INCLUDED
