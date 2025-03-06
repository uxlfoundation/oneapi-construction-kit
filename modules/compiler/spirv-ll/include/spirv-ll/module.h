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

#ifndef SPIRV_LL_SPV_MODULE_H_INCLUDED
#define SPIRV_LL_SPV_MODULE_H_INCLUDED

#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/llvm_version.h>
#include <spirv-ll/context.h>
#include <spirv-ll/opcodes.h>
#include <spirv/unified1/spirv.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace spirv_ll {
/// @brief Enum class used to represent an Extended Instruction Set.
enum class ExtendedInstrSet {
  GLSL450,             ///< The "GLSL.std.450" instruction set.
  OpenCL,              ///< The "OpenCL.std" instruction set.
  GroupAsyncCopies,    ///< The "Codeplay.GroupAsyncCopies" instruction set.
  DebugInfo,           ///< The "DebugInfo" instruction set.
  OpenCLDebugInfo100,  ///< The "OpenCL.DebugInfo.100" instruction set.
};

/// @brief Interface to a binary SPIR-V module's header.
class ModuleHeader {
 public:
  /// @brief SPIR-V magic number.
  static const uint32_t MAGIC = 0x07230203;

  /// @brief Construct from a SPIR-V binary stream.
  ///
  /// @param code View of the SPIR-V binary stream.
  ModuleHeader(llvm::ArrayRef<uint32_t> code);

  /// @brief Returns the endian swapped SPIR-V magic number.
  uint32_t magic() const;

  /// @brief Returns the endian swapped SPIR-V version number.
  uint32_t version() const;

  /// @brief Returns the endian swapped SPIR-V generator ID.
  uint32_t generator() const;

  /// @brief Returns the endian swapped ID bound.
  uint32_t bound() const;

  /// @brief Returns the endian swapped schema (currently reserved).
  uint32_t schema() const;

  /// @brief Check if this SPIR-V module has a valid SPIR-V magic number.
  ///
  /// @return Returns `true` if a valid magic number is found, false otherwise.
  bool isValid() const { return magic() == MAGIC; }

 protected:
  /// View of the SPIR-V binary stream.
  llvm::ArrayRef<uint32_t> code;
  /// @brief Flag indicating the module's endianness needs swapped.
  const bool endianSwap;
};

/// @brief OpCode iterator for a SPIR-V module.
class iterator {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = spirv_ll::OpCode;
  using pointer = const value_type *;
  using reference = const value_type &;
  using iterator_category = std::input_iterator_tag;

  /// @brief Constructor.
  ///
  /// @param endianSwap Flag indicating the module's endianness needs swapped.
  /// @param word Pointer to the first word of the SPIR-V module.
  iterator(bool endianSwap, const uint32_t *word)
      : endianSwap(endianSwap), word(word) {}

  /// @brief Copy constructor.
  iterator(const iterator &rhs) = default;

  /// @brief Move constructor.
  iterator(iterator &&rhs) = default;

  /// @brief Copy assignment operator.
  iterator &operator=(const iterator &rhs) {
    new (this) iterator(rhs);
    return *this;
  }

  /// @brief Move assignment operator.
  iterator &operator=(iterator &&rhs) {
    new (this) iterator(std::move(rhs));
    return *this;
  }

  /// @brief Equality operator, compares the iterators' current words.
  bool operator==(const iterator &rhs) const { return word == rhs.word; }

  /// @brief Not equal operator, compares the iterators' current words.
  bool operator!=(const iterator &rhs) const { return word != rhs.word; }

  /// @brief Dereference operator, returns the current `spirv_ll::OpCode`.
  value_type operator*() const { return value_type(*this); }

  /// @brief Dereference operator, returns the current `spirv_ll::OpCode`.
  value_type operator->() const { return value_type(*this); }

  /// @brief Increment operator, advances the iterator one instruction.
  iterator &operator++() {
    word += *word >> spv::WordCountShift;
    return *this;
  }

  /// @brief Increment operator, advances the iterator one instruction.
  iterator operator++(int) {
    iterator current = *this;
    ++(*this);
    return current;
  }

  /// @brief Flag indicating the module's endianness needs swapped.
  const bool endianSwap;
  /// @brief The word the iterator is currently pointing to.
  const uint32_t *word;
};

/// @brief Swap `spirv_ll::iterator`'s.
///
/// @param lhs Left hand iterator to swap.
/// @param rhs Right hand iterator to swap.
inline void swap(iterator &lhs, iterator &rhs) {
  iterator tmp = std::move(lhs);
  lhs = std::move(rhs);
  rhs = std::move(tmp);
}

/// @brief Struct describing a descriptor binding.
struct DescriptorBinding {
  /// @brief Descriptor set number.
  uint32_t set;
  /// @brief Binding number within `set`.
  uint32_t binding;

  /// @brief Less than comparison operator to enable sort by binding.
  bool operator<(const DescriptorBinding &other) const {
    return set < other.set || (set == other.set && binding < other.binding);
  }
};

/// @brief Container class for translating a binary SPIR-V module.
class Module : public ModuleHeader {
 public:
  /// @brief Construct a SPIR-V module for translation.
  ///
  /// @param[in] context The SPIR-V context the module reside within.
  /// @param[in] code View of the SPIR-V binary stream.
  /// @param[in] specInfo Information about specialization constants.
  Module(spirv_ll::Context &context, llvm::ArrayRef<uint32_t> code,
         cargo::optional<const spirv_ll::SpecializationInfo &> specInfo);

  /// @brief Construct a SPIR-V module to extract specializable constants.
  ///
  /// @param[in] context The SPIR-V context the module resides within.
  /// @param[in] code View of the SPIR-V binary stream.
  Module(spirv_ll::Context &context, llvm::ArrayRef<uint32_t> code);

  /// @brief Returns an iterator that points to the first instruction in the
  /// module.
  iterator begin() const { return {endianSwap, code.begin() + 5}; }

  /// @brief Returns an iterator that points to one past the last word in the
  /// module.
  iterator end() const {
    return {endianSwap,
            code.begin() + std::max(code.size(), static_cast<size_t>(5))};
  }

  /// @brief Enable a capability.
  ///
  /// @param capability The capability to enable.
  void enableCapability(spv::Capability capability) {
    capabilities.insert(capability);
  }

  /// @brief Check if a capability has been enabled.
  ///
  /// @param capability The capability to check.
  ///
  /// @return Returns true if the capability has been enabled, false otherwise.
  bool hasCapability(spv::Capability capability) const {
    return 0 != capabilities.count(capability);
  }

  /// @brief Check if any of the capabilities have been enabled.
  ///
  /// @param caps List of capabilties to check.
  ///
  /// @return Returns true if any capability in the list has been enabled,
  /// false otherwise.
  bool hasCapabilityAnyOf(llvm::ArrayRef<spv::Capability> caps) const {
    return std::any_of(caps.begin(), caps.end(),
                       [this](spv::Capability capability) {
                         return hasCapability(capability);
                       });
  }

  /// @brief Add an extension declared with `OpExtension` to the module.
  ///
  /// @param extension String containing the name of the extension to declare.
  void declareExtension(llvm::StringRef extension) {
    extensions.insert(extension);
  }

  /// @brief Check if an extension has been declared by the module.
  ///
  /// @param extension Name of the extension to check for.
  ///
  /// @return Returns true if the extension has been declared, false otherwise.
  bool isExtensionEnabled(llvm::StringRef extension) const {
    return extensions.count(extension);
  }

  /// @brief Associates an SPV ID with an extended instruction set.
  ///
  /// @param ID the SpirV ID associated with the instruction set.
  /// @param set the ExtendedInstrSet indicating the instruction set.
  void associateExtendedInstrSet(spv::Id ID, ExtendedInstrSet set);

  /// @brief Returns the extended instruction set associated with an SPV ID.
  ///
  /// @param id SPIRV ID associated with the instruction set.
  ///
  /// @return The extended instruction set associated with the ID.
  ExtendedInstrSet getExtendedInstrSet(spv::Id id) const;

  /// @brief Set the addressing model.
  ///
  /// @param addrModel Addressing model that is defined.
  void setAddressingModel(const uint32_t addrModel);

  /// @brief Returns the value of the addressing model.
  uint32_t getAddressingModel() const;

  /// @brief Add a new entry point to the module.
  ///
  /// @param[in] op The OpEntryPoint object to add.
  void addEntryPoint(const OpEntryPoint *op);

  /// @brief Check if a given ID was declared as an entry point, and return a
  /// pointer to the declaring OpEntryPoint if it was.
  ///
  /// @param[in] id The ID to search for.
  ///
  /// @return Pointer to the declaring OpEntryPoint or nullptr if the ID isn't
  /// found.
  const OpEntryPoint *getEntryPoint(spv::Id id) const;

  /// @brief Wrapper for `addID` to deal with cases where an ID needs to have
  /// its value replaced.
  ///
  /// This wrapper violates the golden rule of SSA by searching the ID/Value map
  /// for the given ID (obtained from the provided `OpResult`) and removing
  /// any entry it finds so the ID can be reassigned to a new value. This is
  /// needed for cases where a value that should be global in scope is
  /// translated into a local value in each function.
  ///
  /// @param Op `OpResult` object with the ID whose value will be replaced.
  /// @param V `Value` object to replace the old value with.
  void replaceID(const OpResult *Op, llvm::Value *V);

  /// @brief Add a specified execution mode to the module.
  ///
  /// @param executionMode The execution mode to add to the module.
  void addExecutionMode(const OpExecutionMode *executionMode);

  /// @brief Get the list of execution modes for the given entry point.
  ///
  /// @param entryPoint ID of the entry point to get the execution modes for.
  ///
  /// @return Returns a list of execution modes for the given entry point.
  llvm::ArrayRef<const OpExecutionMode *> getExecutionModes(
      spv::Id entryPoint) const;

  /// @brief Get the requested execution mode for the given entry point.
  ///
  /// @param entryPoint ID of the entry point to get the execution mode for.
  /// @param mode The requested execution mode type.
  ///
  /// @return Returns a pointer to the execution mode if found, `nullptr`
  /// otherwise.
  const OpExecutionMode *getExecutionMode(spv::Id entryPoint,
                                          spv::ExecutionMode mode) const;

  /// @brief Sets the internally stored source language enum.
  void setSourceLanguage(const spv::SourceLanguage sourceLang);

  /// @brief Gets the source language enum reported by OpSource.
  spv::SourceLanguage getSourceLanguage() const;

  /// @brief Sets the string used to hold the source language source code
  /// included with the OpSource and OpSourceContinued functions.
  ///
  /// @param str The new source metadata string.
  void setSourceMetadataString(const std::string &str);

  /// @brief Appends the string used to hold the source language source code
  /// included with the OpSource and OpSourceContinued instructions.
  ///
  /// @param str The text to append to the source metadata string.
  void appendSourceMetadataString(const std::string &str);

  /// @brief Gets the string used to hold the source language source code
  /// included with the OpSource and OpSourceContinued functions.
  ///
  const std::string &getSourceMetadataString() const;

  /// @brief Sets the string used to hold the process/processor
  void setModuleProcess(const std::string &str);

  /// @brief Gets the string used to hold the process/processor
  const std::string &getModuleProcess() const;

  /// @brief Check if this ID is an OpExtInst with the given opcode.
  ///
  /// @return Returns true if this opcode is an OpExtInst with the given
  /// opcode, false otherwise.
  bool isOpExtInst(spv::Id id, uint32_t opcode,
                   const std::unordered_set<ExtendedInstrSet> &sets) const;

  /// @brief Check if this ID is an OpExtInst with any of the given opcodes.
  ///
  /// @return Returns true if this opcode is an OpExtInst with any of the given
  /// opcodes, false otherwise.
  bool isOpExtInst(spv::Id id, const std::unordered_set<uint32_t> &opcodes,
                   const std::unordered_set<ExtendedInstrSet> &sets) const;

  /// @brief Set the DICompileUnit for this module.
  void setCompileUnit(llvm::DICompileUnit *compile_unit);

  /// @brief Get this module's compile unit
  ///
  /// @return Pointer to the module's `DICompileUnit`
  llvm::DICompileUnit *getCompileUnit() const;

  /// @brief Set the current `DIFIle`.
  ///
  /// @param[in] file The new DIFile.
  void setDIFile(llvm::DIFile *file);

  /// @brief Get the current DIFile used for debug instructions.
  ///
  /// @return The DIFile or nullptr if no `DIFile` has been set.
  llvm::DIFile *getDIFile() const;

  /// @brief Add a new ID/string pair to the module for debug instructions.
  ///
  /// @param[in] id The new ID.
  /// @param[in] string The string to associate with the given ID.
  ///
  /// @return True on success, false if the ID already exists in the list
  bool addDebugString(spv::Id id, const std::string &string);

  /// @brief Get the debug string associated with an ID.
  ///
  /// @param[in] id The ID the string is associated with.
  ///
  /// @return The string or std::nullopt if the ID isn't found.
  std::optional<std::string> getDebugString(spv::Id id) const;

  /// @brief Add a basic block and associated lexical block to the module.
  ///
  /// @param b_block Basic block to associate the lexical block with.
  /// @param lex_block Lexical block to add to the module.
  void addLexicalBlock(llvm::BasicBlock *b_block,
                       llvm::DILexicalBlock *lex_block);

  /// @brief Get the `DILexicalBlock` associated with a basic block
  ///
  /// @param block Pointer to the basic block you wish to look up.
  ///
  /// @return Pointer to `DILexical` block or nullptr if `block` isn't found
  llvm::DILexicalBlock *getLexicalBlock(llvm::BasicBlock *block) const;

  /// @brief Add a DISubprogram to the module, and associate it with an ID.
  ///
  /// @param function_id Function to associate the `DISubprogram` with.
  /// @param function_scope `DISubprogram` to add to the module.
  void addDebugFunctionScope(spv::Id function_id,
                             llvm::DISubprogram *function_scope);

  /// @brief Get the `DISubProgram` associated with a given function
  ///
  /// @param function_id ID of the function to look up.
  ///
  /// @return Pointer to the `DISubprogram` associated with the function, or
  /// nullptr if there isn't one
  llvm::DISubprogram *getDebugFunctionScope(spv::Id function_id) const;

  /// @brief Store control mask metadata created by OpLoopMerge
  ///
  /// As the metadata is to be added to the latch of the loop, it will be stored
  /// until `resolveLoopControl` is called.
  ///
  /// @param latch The "continue" aka loop latch block id
  /// @param md_node Pointer to `llvm::MDNode` object created by the
  /// OpLoopMerge.
  void setLoopControl(spv::Id latch, llvm::MDNode *md_node);

  /// @brief Add !llvm.loop metadata to loops
  ///
  /// Attaches all loop metadata previously added via `setLoopControl` to their
  /// respective loop latches.
  void resolveLoopControl();

  /// @brief Add a new ID/name pair to the module.
  ///
  /// @param[in] id The new ID.
  /// @param[in] name The name to associate with the given ID.
  ///
  /// @return True on success, false if the ID already exists in the list.
  bool addName(spv::Id id, const std::string &name);

  /// @brief Get the name associated with an ID.
  ///
  /// @param[in] id The ID the name is associated with.
  ///
  /// @return The name or an empty string if the ID isn't found.
  std::string getName(spv::Id id) const;

  /// @brief Get the name associated with an llvm::Value.
  ///
  /// @param[in] Value The llvm::Value the name is associated with.
  ///
  /// @return The name or an empty string if the llvm::Value isn't found.
  std::string getName(llvm::Value *Value) const;

  /// @brief Add an id and a decoration to associate with it to the module.
  ///
  /// @param[in] id The ID to add.
  /// @param[in] decoration Pointer to the OpDecorate that decorates the ID.
  void addDecoration(spv::Id id, const OpDecorateBase *decoration);

  /// @brief Get the list of decorations for the given ID.
  ///
  /// @param id Opcode ID to get decorations for.
  ///
  /// @return Returns the list of decorations for the given ID.
  llvm::ArrayRef<const OpDecorateBase *> getDecorations(spv::Id id) const;

  /// @brief Get the list of matching decorations for the given ID.
  ///
  /// @param id Opcode ID to get decorations for.
  /// @param decoration The decoration to match.
  ///
  /// @return Returns the list of decorations for the given ID.
  llvm::SmallVector<const OpDecorateBase *, 2> getDecorations(
      spv::Id id, spv::Decoration decoration) const;

  /// @brief Get the first matching decoration for the given ID.
  ///
  /// @param[in] id Opcode id to get the first decoration for.
  /// @param[in] decoration The decoration to match.
  ///
  /// @return Returns a pointer to the first matching decoration if found,
  /// `nullptr` otherwise.
  const OpDecorateBase *getFirstDecoration(spv::Id id,
                                           spv::Decoration decoration) const;

  /// @brief Add a decoration to a member of a struct type.
  ///
  /// @param structType ID of the struct type to add a decoration to.
  /// @param member Member of the struct type to decorate.
  /// @param op `OpDecorateBase` object representing the original decorate
  /// instruction.
  void addMemberDecoration(spv::Id structType, uint32_t member,
                           const OpDecorateBase *op);

  /// @brief Get list of decoration instructions applied to a member of a struct
  /// type.
  ///
  /// @param structType ID of the struct type to query for decorations.
  /// @param member Member index to query.
  ///
  /// @return List of `OpDecorateBase` objects representing member decorations
  /// or empty list if there are none.
  llvm::SmallVector<const OpDecorateBase *, 2> getMemberDecorations(
      spv::Id structType, uint32_t member);

  /// @brief Apply the effects of any decorations associated with an ID.
  ///
  /// @param id An ID with decorations to resolve.
  void resolveDecorations(spv::Id id);

  /// @brief Struct containing information about an interface block.
  struct InterfaceBlock {
    /// @brief `DescriptorBinding` struct that has the binding info.
    DescriptorBinding binding;

    /// @brief Global variable that stores a reference to the interface block.
    llvm::GlobalVariable *variable;

    /// @brief Underlying interface block type. The global variable's value
    /// type will be a pointer to this type.
    llvm::Type *block_type;

    /// @brief `OpVariable` that declared the interface block.
    const spirv_ll::OpVariable *op;
  };

  /// @brief Add an interface block ID and its descriptor set to the module.
  ///
  /// @param[in] id The ID of an interface block.
  /// @param[in] set The descriptor set number of the interface block.
  void addSet(spv::Id id, uint32_t set);

  /// @brief Add an interface block ID and its descriptor binding to the module.
  ///
  /// @param[in] id ID of an interface block already added to the module with.
  /// @param[in] binding The binding number of the interface block.
  void addBinding(spv::Id id, uint32_t binding);

  /// @brief Return a list of interface block IDs, sorted by their descriptor
  /// bindings.
  ///
  /// @return List of IDs.
  llvm::SmallVector<spv::Id, 4> getDescriptorBindingList() const;

  /// @brief Fill a list with descriptor set/binding slots used in the module.
  ///
  /// @return Returns the list of descriptor set/binding slots used.
  std::vector<DescriptorBinding> getUsedDescriptorBindings() const;

  /// @brief Whether or not the module uses any descriptor bindings.
  ///
  /// @return True if bindings were used, false otherwise.
  bool hasDescriptorBindings() const;

  /// @brief Look up the `OpCode` object associated with an interface block ID.
  ///
  /// @param id ID of the interface block to look up.
  ///
  /// @return `OpCode` object that originally created the variable.
  const OpCode *getBindingOp(spv::Id id) const;

  /// @brief Add an interface block variable to the module.
  ///
  /// @param id SPIR-V ID of the block.
  /// @param op `OpVariable` that declared the block.
  /// @param variable LLVM global variable object created for the block.
  void addInterfaceBlockVariable(const spv::Id id,
                                 const spirv_ll::OpVariable *op, llvm::Type *ty,
                                 llvm::GlobalVariable *variable);

  /// @brief Return the type of an interface block referred to by an ID.
  ///
  /// @param[in] id ID of an interface block.
  ///
  /// @return Type of the interface block.
  llvm::Type *getBlockType(const spv::Id id) const;

  /// @brief Create a new `OpCode` derivative object.
  ///
  /// @tparam Op Type of the derived `OpCode`.
  /// @param opCode Reference to the base `OpCode` object.
  ///
  /// @return Returns a pointer to the new `OpCode` derivative object.
  template <class Op>
  const Op *create(const OpCode &opCode) {
    static_assert(std::is_base_of_v<OpCode, Op>,
                  "Op must be derived from OpCode");
    OpCodes.emplace_back(new Op(opCode));
    SPIRV_LL_ASSERT(OpCodes.back()->code == Op::ClassCode,
                    "mismatch between Op::ClassCode and OpCode::code");
    const Op *op = static_cast<const Op *>(OpCodes.back().get());
    if (std::is_base_of_v<OpResult, Op>) {
      resolveDecorations(reinterpret_cast<const OpResult *>(op)->IdResult());
    }
    return op;
  }

  /// @brief Add a new ID, matching Op and LLVM Type to the module
  ///
  /// If the ID doesn't exist, a new one will be created and inserted into the
  /// Types map. If the ID already exists, the operation will fail, since SSA
  /// form does not allow for IDs to be reassigned.
  ///
  /// @param[in] id The new ID.
  /// @param[in] Op The Op associated with (i.e. creating) the ID.
  /// @param[in] T The LLVM value for the given Op.
  ///
  /// @return true on success, false if the ID already exists
  bool addID(spv::Id id, const OpCode *Op, llvm::Type *T);

  /// @brief track the original SPIR-V type ids for the OpFunctionType `func`
  ///
  /// This is needed to work around type lookup for formal parameters of pointer
  /// type who become opaque. This allows us to track the original types once
  /// the function is in LLVM IR
  ///
  /// @param[in] func The OpFunctionType function
  /// @param[in] ids array of type IDs for the formal parameters of the function
  /// given by `func`
  void setParamTypeIDs(spv::Id func, llvm::ArrayRef<spv::Id> ids);

  /// @brief get the original SPIR-V types ids for the OpFunctionType `func`
  ///
  /// @param[in] func The function type of interest
  /// @param[in] argno the zero-indexed argument number
  /// @return SPIR-V SSA ID referring to the parameter type or nullopt on
  /// failure
  std::optional<spv::Id> getParamTypeID(spv::Id func, unsigned argno) const;

  /// @brief Get the LLVM Type for the given SPIR-V ID.
  ///
  /// @param[in] id The SPIR-V ID to fetch the value for.
  ///
  /// @return A pointer to the Type or nullptr if not found.
  llvm::Type *getLLVMType(spv::Id id) const;

  /// @brief Get the `OpType` from the result type of an `OpCode`.
  ///
  /// @param[in] opCode The `OpCode` to get the result type `OpType` from.
  ///
  /// @return Returns an `OpType` pointer representing the result type ID.
  const OpType *getResultType(const OpCode *opCode) const {
    SPIRV_LL_ASSERT(opCode->hasResult(), "id does not have a result");
    auto opType = get<OpType>(cast<OpResult>(opCode)->IdResultType());
    SPIRV_LL_ASSERT_PTR(opType);
    return opType;
  }

  /// @brief Get the `OpType` from the result type ID.
  ///
  /// @param[in] id An opcode ID to get the result type `OpType` from.
  ///
  /// @return Returns an `OpType` pointer representing the result type ID.
  const OpType *getResultType(spv::Id id) const {
    auto opCode = get<OpCode>(id);
    SPIRV_LL_ASSERT_PTR(opCode);
    return getResultType(opCode);
  }

  /// @brief Add forward pointer to the module.
  ///
  /// @param id ID of the forward pointer.
  void addForwardPointer(spv::Id id);

  /// @brief Look up an ID to see if it was forward declared.
  ///
  /// @param id ID of the forward pointer.
  ///
  /// @return True if the ID was found, false otherwise.
  bool isForwardPointer(spv::Id id) const;

  /// @brief Remove forward pointer from the module.
  ///
  /// @param id ID of the forward pointer.
  void removeForwardPointer(spv::Id id);

  /// @brief Log a forward reference to a function, to be resolved later.
  ///
  /// @param id ID of the forward function reference.
  /// @param fn the forward-declared function.
  void addForwardFnRef(spv::Id id, llvm::Function *fn);

  /// @brief Retrieve a forward reference to a function.
  ///
  /// @param id ID of the forward function reference.
  /// @return The function, or nullptr if no forward reference was made to this.
  llvm::Function *getForwardFnRef(spv::Id id) const;

  /// @brief Resolve a forward function reference.
  ///
  /// @param id ID of the forward function reference.
  void resolveForwardFnRef(spv::Id id);

  /// @brief Add an incomplete struct and its missing type IDs to the module.
  ///
  /// @param struct_type OpCode object that created the incomplete struct.
  /// @param missing_types List of IDs representing which types from the struct
  /// have not yet been defined.
  void addIncompleteStruct(const OpTypeStruct *struct_type,
                           const llvm::SmallVector<spv::Id, 2> &missing_types);

  /// @brief Update an incomplete struct type with a newly defined member.
  ///
  /// @param member_id ID of the member type that was defined.
  void updateIncompleteStruct(spv::Id member_id);

  /// @brief Return the LLVM address space for the given storage class, or an
  /// error if the storage class is unknown/unsupported.
  llvm::Expected<unsigned> translateStorageClassToAddrSpace(
      uint32_t storage_class) const;

  /// @brief Add a complete pointer.
  ///
  /// @param pointer_type OpCode object that describes the pointer type.
  llvm::Error addCompletePointer(const OpTypePointer *pointer_type);

  /// @brief Add an incomplete pointer and its missing type IDs to the module.
  ///
  /// @param pointer_type OpCode object that created the incomplete pointer.
  /// @param missing_type ID representing which type has not yet been defined.
  void addIncompletePointer(const OpTypePointer *pointer_type,
                            spv::Id missing_type);

  /// @brief Update an incomplete pointer type with a newly defined type.
  ///
  /// @param type_id ID of the type that was defined.
  llvm::Error updateIncompletePointer(spv::Id type_id);

  /// @brief Add id, image and sampler to the module.
  ///
  /// @param[in] id The ID to add.
  /// @param[in] image The image value to be added.
  /// @param[in] sampler The sampler value to be added.
  void addSampledImage(spv::Id id, llvm::Value *image, llvm::Value *sampler);

  /// @brief Struct holding the information needed for a sampled image: image
  /// and sampler.
  struct SampledImage {
    /// @brief Default constructor.
    SampledImage() : image(nullptr), sampler(nullptr) {}
    /// @brief Image and sampler sampler constrtuctor.
    SampledImage(llvm::Value *Image, llvm::Value *Sampler)
        : image(Image), sampler(Sampler) {}
    /// @brief Image value.
    llvm::Value *image;
    /// @brief Sampler value.
    llvm::Value *sampler;
  };

  /// @brief Returns a SampledImage struct from llvm::DenseMap<spv::Id,
  /// SampledImage> SampledImagesMap based on the id.
  ///
  /// @param[in] id The ID used for the lookup.
  SampledImage getSampledImage(spv::Id id) const;

  /// @brief Add a new ID, matching Op and LLVM Value to the module.
  ///
  /// If the ID doesn't exist, a new one will be created and inserted into the
  /// Values map. If the ID already exists, the operation will fail, since SSA
  /// form does not allow for IDs to be reassigned.
  ///
  /// @param[in] id The new ID.
  /// @param[in] Op The Op associated with (i.e. creating) the ID.
  /// @param[in] V The LLVM value for the given Op.
  ///
  /// @return true on success, false if the ID already exists
  bool addID(spv::Id id, const OpCode *Op, llvm::Value *V);

  /// @brief Get the LLVM Value for the given SPIR-V ID.
  ///
  /// @param[in] id The SPIR-V ID to fetch the value for.
  ///
  /// @return A pointer to the Value or nullptr if not found.
  llvm::Value *getValue(spv::Id id) const;

  /// @brief Get the SPIR-V Op for the given ID.
  ///
  /// The function will search both the Types and the Values to try and find
  /// the given ID.
  ///
  /// @param[in] id The ID for the Op to get.
  ///
  /// @return A pointer to the Op or nullptr if not found.
  template <class Op = OpCode>
  const Op *get_or_null(spv::Id id) const {
    if (!id) {
      return nullptr;
    }
    if (auto ty = Types.find(id); ty != Types.end()) {
      return cast<Op>(ty->second.Op);
    }
    if (auto val = Values.find(id); val != Values.end()) {
      return cast<Op>(val->second.Op);
    }
    auto found = std::find_if(
        OpCodes.begin(), OpCodes.end(),
        [&](const std::unique_ptr<const spirv_ll::OpCode> &opcode) {
          if (opcode.get()->hasResult()) {
            auto op = cast<OpResult>(opcode.get());
            if (op->IdResult() == id) {
              return true;
            }
          }
          return false;
        });
    return found == OpCodes.end() ? nullptr : cast<Op>(found->get());
  }

  /// @brief Get the SPIR-V Op for the given ID.
  ///
  /// The function will search both the Types and the Values to try and find
  /// the given ID. Asserts that the op was found.
  ///
  /// @param[in] id The ID for the Op to get.
  ///
  /// @return A pointer to the Op.
  template <class Op = OpCode>
  const Op *get(spv::Id id) const {
    auto *const op = get_or_null<Op>(id);
    SPIRV_LL_ASSERT(op, "OpCode for ID not found");
    return op;
  }

  /// @brief Get the SpirV Op for the given LLVM Value.
  ///
  /// @param[in] v The LLVM value to find the Op for.
  ///
  /// @return A pointer to the Op or nullptr if not found.
  template <class Op = OpCode>
  const Op *get(llvm::Value *v) const {
    auto found = std::find_if(
        Values.begin(), Values.end(),
        [v](decltype(*Values.begin()) &e) { return e.second.Value == v; });
    SPIRV_LL_ASSERT(found != Values.end(), "OpCode for llvm::Value not found");
    return cast<Op>(found->second.Op);
  }

  /// @brief Get the SpirV Op for the given LLVM Type.
  ///
  /// @param[in] ty The LLVM type to find the Op for.
  ///
  /// @return A pointer to the Op or nullptr if not found.
  template <class Op = OpCode>
  const Op *getFromLLVMTy(llvm::Type *ty) const {
    assert(!ty->isPointerTy() && "can't get the type of a pointer");
    auto found = std::find_if(
        Types.begin(), Types.end(),
        [&](decltype(*Types.begin()) &e) { return e.second.Type == ty; });
    SPIRV_LL_ASSERT(found != Types.end(), "OpCode for llvm::Type not found");
    return cast<Op>(found->second.Op);
  }

  /// @brief Add an ID to the list of builtin variable IDs.
  ///
  /// @param[in] id ID of the decorated variable.
  void addBuiltInID(spv::Id id);

  /// @brief Get a reference to the list of decorated builtin variable IDs.
  ///
  /// @return Const reference to the list.
  const llvm::SmallVector<spv::Id, 4> &getBuiltInVarIDs() const;

  /// @brief Get user specified specialization information.
  ///
  /// @return Returns an optional containing the specialization information.
  const cargo::optional<const spirv_ll::SpecializationInfo &> &getSpecInfo();

  /// @brief Add a spec constant's specialization ID to the module.
  ///
  /// @param id ID of the spec constant.
  /// @param spec_id Specialization ID to associate with the ID.
  void addSpecId(spv::Id id, uint32_t spec_id);

  /// @brief Get the specialization ID for a spec constant.
  ///
  /// @param id ID of the spec constant.
  ///
  /// @return Return the specialization ID of the spec constant if present,
  /// `std::nullopt` otherwise.
  std::optional<uint32_t> getSpecId(spv::Id id) const;

  /// @brief Get a pointer to the push constant struct type defined in the
  /// module.
  ///
  /// @return Pointer to the push constant struct type defined in the module or
  /// `nullptr` if one was not defined.
  llvm::Type *getPushConstantStructType() const;

  /// @brief Get the ID that will be used to access the push constant struct.
  ///
  /// @return Push constant struct ID.
  spv::Id getPushConstantStructID() const;

  /// @brief Get the previously stored buffer size array `Value`
  ///
  /// @return Buffer size array `Value`
  llvm::Value *getBufferSizeArray() const;

  /// @brief Set a global variable for the push constant struct.
  ///
  /// The push constant struct is passed into the entry point of the module as
  /// an argument, so to make it available globally like it should be the
  /// argument is stored in this global variable at the start of the entry point
  /// and loaded from it at the start of every other function.
  ///
  /// @param id SPIR-V ID the push constant struct will be accessed with.
  /// @param variable LLVM global variable object.
  void setPushConstantStructVariable(spv::Id id,
                                     llvm::GlobalVariable *variable);

  /// @brief Store local workgroup size specified by module in the module.
  ///
  /// @param x The x dimension of the workgroup size.
  /// @param y The y dimension of the workgroup size.
  /// @param z The z dimension of the workgroup size.
  void setWGS(uint32_t x, uint32_t y, uint32_t z);

  /// @brief Retrieve local workgroup size set by the module.
  ///
  /// @return array containing the local workgroup size dimensions in x, y, z
  /// order.
  const std::array<uint32_t, 3> &getWGS() const;

  /// @brief Save the buffer size array `Value`.
  ///
  /// @param buffer_size_array Buffer size array `Value`.
  void setBufferSizeArray(llvm::Value *buffer_size_array);

  /// @brief Store an OpSpecConstantOp that can't be translated immediately.
  ///
  /// @param op `OpSpecConstantOp` object representing the instruction to defer.
  void deferSpecConstantOp(const spirv_ll::OpSpecConstantOp *op);

  /// @brief Accessor for `deferredSpecConstantOps`.
  const llvm::SmallVector<const OpSpecConstantOp *, 2> &
  getDeferredSpecConstants();

  /// @brief Get a list of entry point arguments that need to have global scope.
  llvm::SmallVector<std::pair<spv::Id, llvm::GlobalVariable *>, 4>
  getGlobalArgs() const;

  /// @brief The context that this module is using.
  spirv_ll::Context &context;
  /// @brief The `llvm::Module` to write translated SPIR-V into.
  std::unique_ptr<llvm::Module> llvmModule;
  /// @brief The fence wrapper function.
  llvm::Function *fenceWrapperFcn;
  /// @brief The barrier wrapper function.
  llvm::Function *barrierWrapperFcn;
  /// @brief Map of OpGropup(Any|All) to the wrapper functions required
  /// to implement them in IR. This will get populated as and when operations
  /// need to get exapanded.
  std::unordered_map<std::string, llvm::Function *> predicateWrapperMap;
  /// @brief Map of OpGroupBroadcast to the wrapper functions required
  /// to implement them in IR. This will get populated as and when operations
  /// need to get exapanded.
  std::unordered_map<const OpType *,
                     std::unordered_map<unsigned, llvm::Function *>>
      broadcastWrapperMap;
  /// @brief Map of OpGroup(IAdd|FAdd|FMin|UMin|SMin|FMax|UMax|SMax) to the
  /// wrapper functions required to implement them in IR. This will get
  /// populated as and when operations need to get exapanded.
  std::unordered_map<
      spv::GroupOperation,
      std::unordered_map<std::string,
                         std::unordered_map<const OpType *, llvm::Function *>>>
      reductionWrapperMap;

  /// @brief Turn off the use of implicit debug scopes across the module.
  void disableImplicitDebugScopes();

  /// @brief Returns true if implicit debug scopes should be created to handle
  /// debug information.
  bool useImplicitDebugScopes() const;

 private:
  /// @brief The set of enabled capabilities.
  llvm::SmallSet<spv::Capability, 16> capabilities;

  /// @brief The set of extensions declared by the module.
  llvm::SmallSet<llvm::StringRef, 2> extensions;

  // TODO: Store OpExtension's?
  /// @brief Bindings between SpirV ID and extended instruction set.
  llvm::DenseMap<spv::Id, ExtendedInstrSet> ExtendedInstrSetBindings;

  // TODO: Store OpMemoryModel?
  /// @brief The addressing model that is defined.
  uint32_t addressingModel;

  /// @brief A map of IDs forward declared as entry points and their
  /// corresponding OpEntryPoint objects.
  llvm::DenseMap<spv::Id, const OpEntryPoint *> EntryPoints;

  /// @brief A map of ID's to execution modes.
  llvm::DenseMap<spv::Id, llvm::SmallVector<const OpExecutionMode *, 2>>
      ExecutionModes;

  /// @brief Source language enum reported by OpSource.
  spv::SourceLanguage sourceLanguage;
  /// @brief The string which contains the source language source code metadata
  /// included with OpSource and OpSourceContinued instructions.
  std::string sourceMetadataString;

  /// @brief The DICompileUnit for this module.
  llvm::DICompileUnit *CompileUnit;
  /// @brief A map of string used for debug instructions.
  llvm::DenseMap<spv::Id, std::string> DebugStrings;
  /// @brief `DIFile` object specified by the module currently being translated.
  llvm::DIFile *File;
  /// @brief Map of BasicBlock to associated `DILexicalBlock`.
  llvm::DenseMap<llvm::BasicBlock *, llvm::DILexicalBlock *> LexicalBlocks;
  /// @brief Map of function IDs to their associated `DISubprogram`s.
  llvm::DenseMap<spv::Id, llvm::DISubprogram *> FunctionScopes;
  /// @brief A mapping between spirv block id's and LLVM loop control mask.
  ///
  /// For each entry, the LLVM block generated by the spirv block `Id` will
  /// have the respective `MDNode` loop data attached to it.
  llvm::DenseMap<spv::Id, llvm::MDNode *> LoopControl;

  /// @brief A list of names from the OpName instructions in the module.
  llvm::DenseMap<spv::Id, std::string> Names;

  /// @brief Type for storing and looking up struct type member decorations.
  using DecoratedStruct =
      llvm::DenseMap<uint32_t, llvm::SmallVector<const OpDecorateBase *, 2>>;
  /// @brief A map of IDs and their decorations.
  llvm::DenseMap<spv::Id, llvm::SmallVector<const OpDecorateBase *, 2>>
      DecorationMap;
  /// @brief Map to keep track of decorations applied by `OpMemberDecorate`.
  llvm::DenseMap<spv::Id, DecoratedStruct> MemberDecorations;
  /// @brief Map of IDs to the interface blocks they reference.
  llvm::DenseMap<spv::Id, InterfaceBlock> InterfaceBlocks;

  /// @brief Owning container for all the OpCodes in this module.
  // TODO: Could use a slab allocator for `OpCode` derived object storage and
  // use a rough `module.build() * 2` heurisitic to reserve memory for the
  // `OpCodes` pointer storage. This may reduce the number of small allocations
  // made during translation.
  llvm::SmallVector<std::unique_ptr<const OpCode>, 64> OpCodes;

  /// @brief Pair holding a SPIR-V Op and the matching LLVM Type.
  struct TypePair {
    /// @brief Empty constructor, initializes everything to nullptr.
    TypePair() : Op(nullptr), Type(nullptr) {}
    /// @brief Constructor with initializers
    TypePair(const OpCode *Op, llvm::Type *Type) : Op(Op), Type(Type) {}
    /// @brief Pointer to the SPIR-V Op.
    const OpCode *Op;
    /// @brief Pointer to the LLVM Type defined by the SPIR-V Op.
    llvm::Type *Type;
  };
  /// @brief Map of IDs to LLVM Types.
  llvm::MapVector<spv::Id, TypePair> Types;
  /// @brief Map of function IDs to SPIR-V Types IDs
  llvm::DenseMap<spv::Id, llvm::SmallVector<spv::Id, 3>> ParamTypeIDs;
  /// @brief List of IDs that correspond to forward declared pointer types.
  llvm::SmallSet<spv::Id, 4> ForwardPointers;
  /// @brief Map of function IDs to forward-declared functions.
  llvm::DenseMap<spv::Id, llvm::Function *> ForwardFnRefs;
  /// @brief Map incomplete (contains forward pointer) struct and missing types.
  llvm::DenseMap<const OpTypeStruct *, llvm::SmallVector<spv::Id, 2>>
      IncompleteStructs;
  /// @brief Map incomplete pointers and pointed to types.
  llvm::DenseMap<const OpTypePointer *, spv::Id> IncompletePointers;
  /// @brief Map of IDs that correspond to sampled image structs.
  llvm::DenseMap<spv::Id, SampledImage> SampledImagesMap;

  /// @brief Pair holding a SPIR-V Op and the matching LLVM Value.
  struct ValuePair {
    /// @brief Empty constructor, initializes everything to nullptr.
    ValuePair() : Op(nullptr), Value(nullptr) {}
    /// @brief Constructor with initializers
    ValuePair(const OpCode *Op, llvm::Value *Value) : Op(Op), Value(Value) {}
    /// @brief Pointer to the SPIR-V Op.
    const OpCode *Op;
    /// @brief Pointer to the LLVM Value defined by the SPIR-V Op.
    llvm::Value *Value;
  };
  /// @brief Map of IDs to LLVM Values.
  llvm::MapVector<spv::Id, ValuePair> Values;
  /// @brief Set containing IDs that have been decorated as builtin variables.
  llvm::SmallVector<spv::Id, 4> BuiltInVarIDs;
  /// @brief Map of spec constant IDs and their specialization IDs.
  llvm::DenseMap<spv::Id, spv::Id> SpecIDs;
  /// @brief Information about specialization constants.
  cargo::optional<const spirv_ll::SpecializationInfo &> specInfo;
  /// @brief Global variable that stores a reference to the push constant
  /// struct.
  llvm::GlobalVariable *PushConstantStructVariable;
  /// @brief ID used throughout the module to access the push constant struct.
  spv::Id PushConstantStructID;
  /// @brief Array of values that represent local workgroup size of the module.
  std::array<uint32_t, 3> WorkgroupSize;
  /// @brief Handle to an array of sizes of descriptor bindings in the module.
  ///
  /// More specifically, it contains the sizes of the buffers the descriptors
  /// are backed by. This exists as a function argument and will be passed in by
  /// the api if any descriptor bindings are used.
  llvm::Value *BufferSizeArray;
  /// @brief List of `OpSpecConstantOp` instructions whose translation had to be
  /// deferred.
  llvm::SmallVector<const spirv_ll::OpSpecConstantOp *, 2>
      deferredSpecConstantOps;
  std::string ModuleProcess;
  /// @brief True if debug scopes should be inferred and generated when
  /// processing debug information.
  ///
  /// False if a DebugInfo-like extension is enabled, and only explicit scope
  /// instructions are to be obeyed.
  bool ImplicitDebugScopes = true;
};

/// @brief Less than operator that compares the descriptor binding in each `ID`
/// `InterfaceBlock` pair to allow for a list of IDs sorted by their associated
/// descriptor set bindings.
inline bool operator<(const std::pair<spv::Id, Module::InterfaceBlock> &lhs,
                      const std::pair<spv::Id, Module::InterfaceBlock> &rhs) {
  return lhs.second.binding < rhs.second.binding;
}

}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_MODULE_H_INCLUDED
