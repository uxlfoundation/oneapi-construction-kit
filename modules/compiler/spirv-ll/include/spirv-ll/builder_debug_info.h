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

#ifndef SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED

#include <spirv-ll/builder.h>
#include <spirv-ll/module.h>
#include <spirv-ll/opcodes.h>

namespace spirv_ll {

/// @brief Combined builder for the DebugInfo and OpenCLDebugInfo100 extended
/// instruction sets.
class DebugInfoBuilder : public ExtInstSetHandler {
 public:
  enum Workarounds {
    // Some versions of llvm-spirv mistakenly swap
    // DebugTypeTemplateTemplateParameter and DebugTypeTemplateParameterPack
    // opcodes, leading to incorrect binaries. When this workaround is enabled,
    // we assume binaries *may* have been created with this bug, and try to
    // infer which opcode is intended based on the operands.
    // See https://github.com/KhronosGroup/SPIRV-LLVM-Translator/pull/2248
    TemplateTemplateSwappedWithParameterPack = 1 << 0,
  };

  /// @brief Constructor.
  ///
  /// @param[in] builder spirv_ll::Builder object that will own this object.
  /// @param[in] module The module being translated.
  DebugInfoBuilder(Builder &builder, Module &module, uint64_t workarounds = 0)
      : ExtInstSetHandler(builder, module), workarounds(workarounds) {}

  /// @see ExtInstSetHandler::create
  virtual llvm::Error create(const OpExtInst &opc) override;

  virtual llvm::Error finishModuleProcessing() override;

 private:
  uint64_t workarounds = 0;
  /// @brief Map from DebugInfo instructions to the llvm::DIBuilder that builds
  /// them.
  std::unordered_map<spv::Id, std::unique_ptr<llvm::DIBuilder>>
      debug_builder_map;

  /// @brief Cache of translated DebugInfo instructions.
  std::unordered_map<spv::Id, llvm::MDNode *> debug_info_cache;

  /// @brief Create an DebugInfo extended instruction transformation to LLVM IR.
  ///
  /// @tparam T The OpenCL extended instruction class template to create.
  /// @param opc The OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <typename T>
  llvm::Error create(const OpExtInst &opc);

  /// @brief Returns the LLVM DIBuilder for the given instruction.
  ///
  /// We may only have one DICompileUnit per DIBuilder, so must support
  /// multiple builders. This function finds the DIBuilder for the instruction
  /// based on its chain of scopes, if applicable.
  llvm::DIBuilder &getDIBuilder(const OpExtInst *op) const;

  /// @brief Returns the first registered DIBuilder, for when it doesn't matter
  /// which is used.
  ///
  /// @see getDIBuilder
  llvm::DIBuilder &getDefaultDIBuilder() const;

  /// @brief Returns true if the given ID is DebugInfoNone.
  bool isDebugInfoNone(spv::Id id) const;

  /// @brief Returns true if the extended instruction set represented by the
  /// given ID is one covered by this builder.
  bool isDebugInfoSet(uint32_t set_id) const;

  /// @brief Returns the constant integer value of an ID, or std::nullopt for
  /// DebugInfoNone, or an error.
  llvm::Expected<std::optional<uint64_t>> getConstantIntValue(spv::Id id) const;

  /// @brief Translates a DebugInfo extension instruction to LLVM IR.
  ///
  /// This does it "on the fly", as opposed to 'create' which visits them in
  /// program order.
  template <typename T = llvm::MDNode>
  llvm::Expected<T *> translateDebugInst(spv::Id id) {
    auto it = debug_info_cache.find(id);
    if (it != debug_info_cache.end()) {
      return static_cast<T *>(it->second);
    }
    auto *const op = module.get_or_null(id);
    // If this isn't a recognized ID, it's probably a forward reference. We
    // count this as an error in this case, as forward references are generally
    // not allowed in DebugInfo instruction sets.
    if (!op) {
      return makeStringError("Unknown id " + getIDAsStr(id, &module) +
                             " - unexpected forward reference?");
    }
    auto *const op_ext_inst = dyn_cast<OpExtInst>(op);

    // If this isn't an OpExtInst, we're trying to translate the wrong thing.
    if (!op_ext_inst || !isDebugInfoSet(op_ext_inst->Set())) {
      return makeStringError("id " + getIDAsStr(id, &module) +
                             " is not a DebugInfo OpExtInst");
    }

    llvm::Expected<llvm::MDNode *> res_or_error =
        translateDebugInstImpl(op_ext_inst);
    if (auto err = res_or_error.takeError()) {
      return std::move(err);
    }
    // Cache this result
    debug_info_cache[id] = res_or_error.get();
    return static_cast<T *>(res_or_error.get());
  }

  llvm::Expected<llvm::MDNode *> translateDebugInstImpl(const OpExtInst *op);

  /// @brief Given an operation that is either a
  /// DebugTypeTemplateTemplateParameter or DebugTypeTemplateParameterPack, try
  /// and infer which is which (in the presence of several known bugs in
  /// ecosystem tooling) and translate it as such.
  llvm::Expected<llvm::MDNode *>
  translateTemplateTemplateParameterOrTemplateParameterPack(
      const OpExtInst *op);

  template <typename T>
  llvm::Expected<llvm::MDNode *> translate(const T *op);

  /// @brief Process the DebugTypeComposite instructions once all other nodes
  /// have been visited.
  llvm::Error finalizeCompositeTypes();

  /// @brief A collection of DebugTypeTemplate instructions
  ///
  /// These instructions are processed at the end of the module, (seemingly)
  /// due to bugs in producers allowing forward references to these nodes.
  llvm::SmallVector<spv::Id, 4> template_types;
  /// @brief A collection of DebugTypeComposite instructions
  ///
  /// These instructions are processed at the end of the module because they
  /// make contain forward references to other nodes, as per the specification.
  llvm::SmallVector<spv::Id, 4> composite_types;
};

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_DEBUG_INFO_H_INCLUDED
