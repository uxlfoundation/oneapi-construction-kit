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

/// @file md_ctx.h
///
/// @brief Metadata API Context.

#ifndef MD_DETAIL_MD_CTX_H_INCLUDED
#define MD_DETAIL_MD_CTX_H_INCLUDED
#include <cargo/expected.h>
#include <metadata/detail/md_stack.h>
#include <metadata/detail/md_value.h>
#include <metadata/detail/metadata_impl.h>
#include <metadata/detail/utils.h>

namespace md {
/// @addtogroup md
/// @{

/// @brief Represents a basic metadata context.
///
/// @tparam AllocatorType Defaulted to callback_allocator.
template <template <class U> class AllocatorType = md::callback_allocator>
class basic_context {
 public:
  using string_t =
      std::basic_string<char, std::char_traits<char>, AllocatorType<char>>;
  using allocator_helper_t = AllocatorHelper<AllocatorType>;
  using stack_t = basic_stack<AllocatorType>;

  // Using shared_ptr due to limitation with allocator aware make_unique()
  // (see P0211)
  using map_t = basic_map<string_t, std::shared_ptr<stack_t>, AllocatorType>;

  /// @brief Construct a new basic context object.
  ///
  /// @param hooks A pointer to user-provided hooks.
  /// @param userdata A pointer to user-supplied data which is passed as a
  /// parameter into supported callback hooks.
  basic_context(md_hooks *hooks, void *userdata)
      : hooks(hooks),
        userdata(userdata),
        alloc(hooks, userdata),
        stack_map(alloc),
        endianness(utils::get_mach_endianness()) {}

  /// @brief Default virtual destructor for inheritance.
  virtual ~basic_context() = default;

  /// @brief Get a pointer to a stack registered with the provided name.
  ///
  /// @param name The name of the stack to find.
  /// @return A pointer to the stack if exists, otherwise
  /// MD_E_STACK_NOT_REGISTERED.
  cargo::expected<stack_t *, md_err> get_block(const char *name) {
    const string_t stack_name(name, alloc.template get_allocator<char>());
    const auto it = stack_map.find(stack_name);
    if (it == stack_map.end()) {
      return cargo::make_unexpected(md_err::MD_E_STACK_NOT_REGISTERED);
    }
    return it->second.get();
  }

  /// @brief Create a new stack object with the specified name.
  ///
  /// @param name The name of the stack to create.
  /// @return A pointer to the newly created stack, or
  /// MD_E_STACK_ALREADY_REGISTERED if a stack with the name already exists in
  /// this context.
  cargo::expected<stack_t *, md_err> create_block(const char *name) {
    const string_t stack_name(name, alloc.template get_allocator<char>());
    const stack_t stack(alloc);
    const std::pair<typename map_t::iterator, bool> in = stack_map.insert(
        std::make_pair(stack_name, alloc.template allocate_shared<stack_t>(
                                       std::move(stack))));
    if (!in.second) {
      return cargo::make_unexpected(md_err::MD_E_STACK_ALREADY_REGISTERED);
    }
    return in.first->second.get();
  }

  /// @brief Get or create a stack in the current context.
  ///
  /// If a stack already exists with the provided name it will be returned.
  /// Otherwise a new stack will be created and returned.
  ///
  /// @param name The name of the stack.
  /// @return A pointer to the stack.
  stack_t *get_or_create_block(const char *name) {
    auto stack = get_block(name);
    if (stack.has_value()) {
      return stack.value();
    }
    return create_block(name).value_or(nullptr);
  }

  /// @brief Get a reference to the custom allocator.
  ///
  /// @return Reference to an allocator helper used to allocated memory.
  const allocator_helper_t &get_alloc_helper() const { return alloc; }

  /// @brief Finalize the context.
  ///
  /// All registered stacks will be serialized and an output binary will be
  /// generated. The binary will be written out with the `write` hook. After all
  /// bytes have been written, the `finalize` hook will be called. If neither
  /// hook is present MD_E_NO_HOOKS will be returned. This method does a single
  /// pass through the registered stacks and uses a reversing algorithm to avoid
  /// knowing the binary size ahead of time.
  ///
  /// @return MD_SUCCESS if completed successfully, otherwise an appropriate
  /// error code is returned.
  md_err finalize() const {
    if (!hooks->write || !hooks->finalize) {
      return md_err::MD_E_NO_HOOKS;
    }

    // Since the string table is of variable length, we instead compute offsets
    // from the end of the binary (which is fixed), only after we know the
    // length of the string table we can update the offsets to point from the
    // beginning of the binary
    std::vector<uint8_t> binary;
    std::vector<uint8_t> string_table;
    std::vector<CAMD_BlockInfo> block_infos;
    for (auto &item : stack_map) {
      // Serialize a stack into bytes
      std::vector<uint8_t> stack_bytes;
      item.second->finalize(stack_bytes, endianness);
      const size_t size_before_padding = stack_bytes.size();
      utils::pad_to_alignment(stack_bytes);

      // push to string_table
      const uint32_t string_table_index = string_table.size();
      const char *stack_name = item.first.c_str();
      // Size must include null terminator
      const size_t stack_name_len = item.first.size() + 1;
      string_table.insert(string_table.end(), stack_name,
                          stack_name + stack_name_len);
      const size_t inverse_block_offset = binary.size() + stack_bytes.size();

      const CAMD_BlockInfo info{
          inverse_block_offset, size_before_padding,
          MD_HEADER_SIZE + string_table_index,
          utils::get_flags(item.second->get_out_fmt(), md_enc::MD_NO_ENC)};
      block_infos.push_back(info);

      // insert bytes in reverse order
      binary.insert(binary.end(), stack_bytes.rbegin(), stack_bytes.rend());
    }

    // pad string table to 8-byte alignment
    utils::pad_to_alignment(string_table);
    const size_t string_table_size = string_table.size();

    const size_t length_of_binary = MD_HEADER_SIZE + string_table_size +
                                    (MD_BLOCK_INFO_SIZE * block_infos.size()) +
                                    binary.size();

    // Now that we know the length of the binary we can update the offset
    // positions positions and serialize the block infos.
    for (auto &info : block_infos) {
      info.offset = (length_of_binary - info.offset);
      std::vector<uint8_t> block_info_bin;
      utils::serialize_block_info(info, endianness, block_info_bin);
      binary.insert(binary.end(), block_info_bin.rbegin(),
                    block_info_bin.rend());
    }

    // push string table
    binary.insert(binary.end(), string_table.rbegin(), string_table.rend());

    // Generate a header for this binary
    const CAMD_Header header{
        {MD_MAGIC_0, MD_MAGIC_1, MD_MAGIC_2, MD_MAGIC_3},
        endianness,
        0x01 /* version */,
        {0x00, 0x00} /* padding */,
        static_cast<uint32_t>(MD_HEADER_SIZE +
                              string_table_size) /*block-list offset*/,
        static_cast<uint32_t>(block_infos.size()) /* n blocks*/};

    std::vector<uint8_t> header_bin;
    utils::serialize_md_header(header, header_bin);
    binary.insert(binary.end(), header_bin.rbegin(), header_bin.rend());

    // Now we can reverse the binary
    std::reverse(binary.begin(), binary.end());

    // Write to target
    hooks->write(userdata, binary.data(), binary.size());
    hooks->finalize(userdata);

    return md_err::MD_SUCCESS;
  }

  /// @brief Decode from a correctly formatted metadata binary.
  ///
  /// @return MD_SUCCESS is returned if the decoding of the binary completed
  /// successfully. MD_NO_HOOKS is returned if there is no user-supplied `map`
  /// hook. MD_INVALID_BINARY is returned when the provided binary is not in a
  /// valid metadata binary format.
  md_err decode_binary() {
    if (!hooks->map) {
      return md_err::MD_E_NO_HOOKS;
    }

    size_t bin_size;
    uint8_t *bin_start =
        static_cast<uint8_t *>(hooks->map(userdata, &bin_size));

    // Decode the header.
    CAMD_Header header;
    const auto header_err =
        utils::decode_md_header(bin_start, header, bin_size);
    if (!header_err.has_value()) {
      return md_err::MD_E_INVALID_BINARY;
    }
    endianness = static_cast<MD_ENDIAN>(header.endianness);

    // Decode the block infos.
    std::vector<CAMD_BlockInfo> infos;
    const auto infos_err = utils::decode_md_block_info_list(
        utils::get_block_list_start(bin_start, header), header, infos,
        bin_size);
    if (!infos_err.has_value()) {
      return md_err::MD_E_INVALID_BINARY;
    }

    // Register each block into the context
    for (auto info : infos) {
      const md_err block_info_err =
          add_block_from_block_info(info, bin_start, endianness);
      if (MD_CHECK_ERR(block_info_err)) {
        return md_err::MD_E_INVALID_BINARY;
      }
    }
    return md_err::MD_SUCCESS;
  }

  /// @brief Get the endian encoding used by this context.
  ///
  /// @return MD_ENDIAN
  MD_ENDIAN get_endianness() { return endianness; }

 private:
  /// @brief Add a block to the metadata context using a BlockInfo.
  ///
  /// @param info The Block Info from which to add the block.
  /// @param bin_start The start of the serialized binary.
  /// @param endianness The endianness of the binary.
  /// @return MD_SUCCESS if the block was added successfully, otherwise
  /// MD_INVALID_BINARY is returned.
  md_err add_block_from_block_info(const CAMD_BlockInfo &info,
                                   uint8_t *bin_start, MD_ENDIAN endianness) {
    const auto fmt = utils::get_fmt(info.flags);
    if (!fmt.has_value()) {
      return md_err::MD_E_INVALID_BINARY;
    }
    stack_t stack(alloc, /* reserve */ 0, fmt.value());
    const string_t stack_name(utils::get_block_info_name(bin_start, info),
                              alloc.template get_allocator<char>());
    switch (stack.get_out_fmt()) {
      case md_fmt::MD_FMT_RAW_BYTES: {
        RawStackSerializer<stack_t>::deserialize(
            stack, utils::get_block_start(bin_start, info), info.size,
            endianness);
        const auto inserted = stack_map.insert(std::make_pair(
            stack_name,
            alloc.template allocate_shared<stack_t>(std::move(stack))));
        if (!inserted.second) {
          return md_err::MD_E_INVALID_BINARY;
        }
      } break;
      case md_fmt::MD_FMT_MSGPACK: {
        BasicMsgPackStackSerializer<stack_t>::deserialize(
            stack, utils::get_block_start(bin_start, info), info.size,
            endianness);
        const auto inserted = stack_map.insert(std::make_pair(
            stack_name,
            alloc.template allocate_shared<stack_t>(std::move(stack))));
        if (!inserted.second) {
          return md_err::MD_E_INVALID_BINARY;
        }
        break;
      }
      case md_fmt::MD_FMT_JSON:
      case md_fmt::MD_FMT_LLVM_BC_MD:
      case md_fmt::MD_FMT_LLVM_TEXT_MD:
      default:
        assert(false && "Unsupported Format type.");
        return md_err::MD_E_INVALID_BINARY;
    }
    return md_err::MD_SUCCESS;
  }

  md_hooks *hooks;
  void *userdata;
  allocator_helper_t alloc;
  map_t stack_map;
  MD_ENDIAN endianness;
};

/// @}
}  // namespace md

/// @brief Implement the API definition of md_ctx_ as a specific
/// instantiation of basic_ctx
struct md_ctx_ final : public md::basic_context<> {
  using basic_context::basic_context;
};

#endif  // MD_DETAIL_MD_CTX_H_INCLUDED
