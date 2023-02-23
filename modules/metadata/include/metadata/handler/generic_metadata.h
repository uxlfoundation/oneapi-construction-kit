// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Handle Generic Metadata.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MD_HANDLER_GENERIC_METADATA_H_INCLUDED
#define MD_HANDLER_GENERIC_METADATA_H_INCLUDED
#include <cargo/optional.h>
#include <metadata/detail/fixed_or_scalable_quantity.h>
#include <metadata/metadata.h>

#include <string>
namespace handler {
/// @addtogroup md_handler
/// @{

/// @brief The block name used in the metadat API in which to store generic
/// metadata.
constexpr char GENERIC_MD_BLOCK_NAME[] = "GenericMetadata";

/// @brief Struct which holds generic metadata, i.e. which can be applied to any
/// kernel.
struct GenericMetadata {
  GenericMetadata() = default;
  GenericMetadata(std::string kernel_name, std::string source_name,
                  uint64_t local_memory_usage);
  GenericMetadata(std::string kernel_name, std::string source_name,
                  uint64_t local_memory_usage,
                  FixedOrScalableQuantity<uint32_t> sub_group_size);
  std::string kernel_name;
  std::string source_name;
  uint64_t local_memory_usage;
  FixedOrScalableQuantity<uint32_t> sub_group_size;
};

/// @brief GenericMetadataHandler handles interacting with the metadata API such
/// that kernel metadata can be correctly read from and written to the binary
/// representation.
class GenericMetadataHandler {
 public:
  GenericMetadataHandler() = default;
  virtual ~GenericMetadataHandler();

  /// @brief Initialize the metadata context.
  ///
  /// @param hooks Hooks forwarded to the context.
  /// @param userdata Userdata passed to the context.
  /// @return true if the context is initialized successfully, false otherwise.
  virtual bool init(md_hooks *hooks, void *userdata);

  /// @brief Finalize the metadata context.
  ///
  /// @return true if finalized successfully, false otherwise.
  virtual bool finalize();

  /// @brief Read kernel metadata.
  ///
  /// @param[in, out] md Metadata struct to be filled in.
  /// @return true if the data was read successfully, false otherwise.
  bool read(GenericMetadata &md);

  /// @brief Write kernel metadata.
  ///
  /// @param md The metadata to be written.
  /// @return true if the write completed successfully, false otherwise.
  bool write(const GenericMetadata &md);

 protected:
  md_ctx ctx = nullptr;
  md_hooks *hooks = nullptr;
  void *userdata = nullptr;

 private:
  char *data = nullptr;
  size_t data_len = 0;
  size_t offset = 0;
};

/// @}
}  // namespace handler
#endif  // MD_HANDLER_GENERIC_METADATA_H_INCLUDED
