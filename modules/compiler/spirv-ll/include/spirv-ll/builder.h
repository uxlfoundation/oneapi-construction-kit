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

/// @file

#ifndef SPIRV_LL_SPV_BUILDER_H_INCLUDED
#define SPIRV_LL_SPV_BUILDER_H_INCLUDED

#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Metadata.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/vector_type_helper.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv-ll/opcodes.h>
#include <spirv/1.0/OpenCL.std.h>

#include <optional>
#include <utility>

/// @brief Forward declarations
namespace llvm {
class Value;
class Type;
class Module;
}  // namespace llvm

namespace spirv_ll {
/// @brief Type used to pass around the list of builtin IDs used by a function
using BuiltinIDList = llvm::SmallVector<spv::Id, 2>;

static constexpr const char SAMPLER_INIT_FN[] =
    "__translate_sampler_initializer";

class Builder;

/// Wrap a string into an llvm::StringError.
static inline llvm::Error makeStringError(const llvm::Twine &message) {
  return llvm::make_error<llvm::StringError>(message.str(),
                                             llvm::inconvertibleErrorCode());
}

static inline std::string getIDAsStr(spv::Id id, Module *module = nullptr) {
  std::string id_str = "%" + std::to_string(id);
  if (module) {
    const std::string name = module->getName(id);
    if (!name.empty()) {
      id_str += "[%" + name + "]";
    }
  }
  return id_str;
}

/// @brief An interface for builders of extended instruction sets.
class ExtInstSetHandler {
 public:
  /// @brief Constructor.
  ///
  /// @param builder `Builder` object that will own this object.
  /// @param module The module being translated.
  ExtInstSetHandler(Builder &builder, Module &module)
      : builder(builder), module(module) {}

  virtual ~ExtInstSetHandler() {}

  /// @brief A hook called once all instructions in the module have been
  /// visited and the IR has been finalized by the 'main' builder.
  ///
  /// Note that handlers may further alter the IR and the order in which they
  /// are called is not deterministic.
  virtual llvm::Error finishModuleProcessing() {
    return llvm::Error::success();
  }

  /// @brief Create an OpenCL extended instruction transformation to LLVM IR.
  ///
  /// @param opc The OpCode object to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  virtual llvm::Error create(const OpExtInst &opc) = 0;

 protected:
  /// @brief `spirv_ll::Builder` that owns this object.
  spirv_ll::Builder &builder;

  /// @brief Reference to the module being translated.
  spirv_ll::Module &module;
};

struct MangleInfo {
  enum class ForceSignInfo {
    None,
    // Override the type's sign with a signed integer. Only valid on integer
    // scalar or integer vector types.
    ForceSigned,
    // Override the type's sign with an unsigned integer. Only valid on
    // integer scalar or integer vector types.
    ForceUnsigned,
  };

  /// @brief Bitmask enum for type qualifiers.
  enum TypeQualifier : uint8_t { NONE = 0, CONST = 0x1, VOLATILE = 0x2 };

  MangleInfo(spv::Id id) : id(id) {}

  MangleInfo(spv::Id id, ForceSignInfo fSign) : id(id), forceSign(fSign) {}

  MangleInfo(spv::Id id, uint8_t typeQuals) : id(id), typeQuals(typeQuals) {}

  MangleInfo(spv::Id id, ForceSignInfo fSign, uint8_t typeQuals)
      : id(id), typeQuals(typeQuals), forceSign(fSign) {}

  /// @brief Constructs a force-signed type
  static MangleInfo getSigned(spv::Id id) {
    return MangleInfo(id, ForceSignInfo::ForceSigned);
  }
  /// @brief Constructs a force-unsigned type
  static MangleInfo getUnsigned(spv::Id id) {
    return MangleInfo(id, ForceSignInfo::ForceUnsigned);
  }

  /// @brief Returns the desired signedness of this type.
  bool getSignedness(const Module &module) const;

  /// @brief The result id or result type's id.
  spv::Id id;
  /// @brief Qualifiers to mangle in with this type (if it's a pointer).
  uint8_t typeQuals = TypeQualifier::NONE;
  /// @brief Signedness override, applicable on integer scalar or integer
  /// vector types.
  ForceSignInfo forceSign = ForceSignInfo::None;
};

/// @brief Get the name of an integer type.
///
/// @param ty The `llvm::Type` representing the integer.
/// @param isSigned Flag to specify the signedness of the integer.
///
/// @return Returns a string containing the integer type name.
std::string getIntTypeName(llvm::Type *ty, bool isSigned);

/// @brief Get the name of a floating point type.
///
/// @param ty The `llvm::Type` representing the floating point.
///
/// @return Returns a string containing the floating point type name.
std::string getFPTypeName(llvm::Type *ty);

/// @brief Class used for generating the LLVM IR from the SPIR-V IR
///
/// This class holds the SpvContext, the IRBuilder, and the LLVM Module
/// necessary for generating the IR, as well as all the functions that convert
/// SPIR-V OpCodes to LLVM IR.
class Builder {
 public:
  // Constructors and assignment operators

  /// @brief Constructor
  ///
  /// @param[in] context The `spirv_ll::Context` to build within.
  /// @param[in] module The `spirv_ll::Module` to build IR from.
  /// @param[in] deviceInfo Information about the target device.
  Builder(spirv_ll::Context &context, spirv_ll::Module &module,
          const spirv_ll::DeviceInfo &deviceInfo);

  // Getters and setters

  /// @return The LLVM IRBuilder used by this builder
  llvm::IRBuilder<> &getIRBuilder();

  /// @brief Get the function the builder is currently working on
  ///
  /// @return The function if one has been declared, otherwise null
  llvm::Function *getCurrentFunction();

  /// @brief Set the function the builder is currently working on
  /// @param function The function
  void setCurrentFunction(llvm::Function *function);

  /// @brief Pop a function argument off the arg list, for use in
  /// OpFunctionParameter
  ///
  /// @return A pointer to the next argument in the list
  llvm::Value *popFunctionArg();

  /// @brief Push a builtin ID to `CurrentFunctionBuiltinIDs`
  ///
  /// @param id the builtin ID to push to the list
  void pushBuiltinID(spv::Id id);

  /// @brief Return a reference to the list of builtin IDs used in
  /// `CurrentFunction`
  ///
  /// @return Reference to `CurrentFunctionBuiltinIDs`
  BuiltinIDList &getBuiltinIDList();

  /// @brief Create an OpCode for translation from the SPIR-V binary stream.
  ///
  /// @tparam Op The type of OpCode to create.
  /// @param op The base object to create the derived OpCode from.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <class Op>
  llvm::Error create(const OpCode &op) {
    return create<Op>(module.create<Op>(op));
  }

  /// @brief Create an OpCode transformation to LLVM IR.
  ///
  /// @tparam Op The type of OpCode to create.
  /// @param op The OpCode to translate.
  ///
  /// @return Returns an `llvm::Error` object representing either success, or
  /// an error value.
  template <class Op>
  llvm::Error create(const Op *op);

  /// @brief Populate the incoming edges/values for the given Phi node
  /// @param op The SpirV Op for the Phi node
  void populatePhi(const OpPhi &op);

  /// @brief A unification of the four very similar access chain functions
  ///
  /// @param opc The OpCode representing the access chain instruction
  /// being translated
  void accessChain(const OpCode &opc);

  /// @brief Represents a lexical scope, used for debug information.
  struct LexicalScopeTy {
    /// @brief The scope, represented in LLVM metadata; could be a function or
    /// block scope but is not specified here. Must not be nullptr in a valid
    /// scope.
    llvm::Metadata *scope = nullptr;
    /// @brief An optional scope, representing where the scope was inlined. May
    /// be nullptr.
    llvm::Metadata *inlined_at = nullptr;
  };

  /// @brief Get the currently active debug scope in the current function the
  /// builder is currently working on
  ///
  /// @return The function if one has been declared, otherwise std::nullopt
  std::optional<LexicalScopeTy> getCurrentFunctionLexicalScope() const;

  /// @brief Set the currently active lexical scope in the current function the
  /// builder is currently working on; std::nullopt signals no active scope, or
  /// the closing of an open one.
  void setCurrentFunctionLexicalScope(std::optional<LexicalScopeTy>);

  /// @brief Called at the end of a lexical scope for book-keeping.
  /// @param closing_line_range True if any open line range should be closed at
  /// the same time.
  void closeCurrentLexicalScope(bool closing_line_range = true);

  /// @brief A type containing an OpLine line range and the beginning of the
  /// range it corresponds to.
  struct LineRangeBeginTy {
    /// @brief A pointer to the OpLine that this line range corresponds to.
    const OpLine *op_line = nullptr;
    /// @brief An optional iterator pointing to the first instruction the range
    /// applies to. Ranges may be open before a block has begun, in which case
    /// this will be std::nullopt.
    std::optional<llvm::BasicBlock::iterator> range_begin = std::nullopt;
  };

  /// @brief Get the currently active debug scope in the current function the
  /// builder is currently working on
  ///
  /// @return The function if one has been declared, otherwise std::nullopt
  std::optional<LineRangeBeginTy> getCurrentOpLineRange() const;

  /// @brief Set the currently active OpLine range; std::nullopt signals no
  /// active OpLine range, or the closing of an open one.
  void setCurrentOpLineRange(std::optional<LineRangeBeginTy>);

  /// @brief At the closing of a scope, apply debug information to instructions
  /// within the closed scope.
  void applyDebugInfoAtClosedRangeOrScope();

  /// @brief Return a `DIType` object that represents the given type
  ///
  /// @param tyID spv::Id `Type` to get `DIType` from
  ///
  /// @return pointer to `DIType` derived from the given `Type`
  llvm::DIType *getDIType(spv::Id tyID);

  /// @brief Gets (or creates) a DIFile for the given OpLine.
  llvm::DIFile *getOrCreateDIFile(const OpLine *op_line);

  /// @brief Gets (or creates) a DICompileUnit for the given OpLine.
  llvm::DICompileUnit *getOrCreateDICompileUnit(const OpLine *op_line);

  /// @brief Gets (or creates) a DISubprogram for the given function and
  /// OpLine.
  llvm::DISubprogram *getOrCreateDebugFunctionScope(llvm::Function &function,
                                                    const OpLine *op_line);

  /// @brief Gets (or creates) a DILexicalBlock for the given function and
  /// OpLine.
  llvm::DILexicalBlock *getOrCreateDebugBasicBlockScope(llvm::BasicBlock &bb,
                                                        const OpLine *op_line);

  /// @brief Called once all instructions in the module have been visited in
  /// order during the first pass through the SPIR-V binary.
  llvm::Error finishModuleProcessing();

  /// @brief Gets (or creates) the BasicBlock for a spv::Id OpLabel.
  llvm::BasicBlock *getOrCreateBasicBlock(spv::Id label);

  /// @brief Generates code in a basic block to initialize a builtin variable.
  ///
  /// @param builtin SPIR-V builtin enum denoting which builtin to initialize.
  /// @param builtinType LLVM `Type` of the builtin variable.
  /// @param initBlock Basic block to generate the init code in.
  void generateBuiltinInitBlock(spv::BuiltIn builtin, llvm::Type *builtinType,
                                llvm::BasicBlock *initBlock);

  /// @brief Attempts to replace uses of a builtin global variable with calls to
  /// the relevant work item function.
  ///
  /// There are a couple of common cases for builtin variable access that can
  /// be translated directly into calls to the relevant function instead of
  /// resorting to a relatively inefficient builtin init block. Specifically
  /// the cases handled by this function are a load followed by extract element
  /// instructions and a GEP followed by a load.
  ///
  /// @param builtinGlobal Builtin global variable to try and replace.
  /// @param kind SPIR-V enum denoting which builtin the variable represents.
  bool replaceBuiltinUsesWithCalls(llvm::GlobalVariable *builtinGlobal,
                                   spv::BuiltIn kind);

  /// @brief Creates a call to a builtin function.
  ///
  /// No function name mangling is performed, see
  /// `SpvBuilder::createMangledBuiltinCall`.
  ///
  /// @param name Name of the builtin function
  /// @param retTy Builtin function return type.
  /// @param args List of the builtin function parameter values.
  /// @param convergent True if the called builtin is convergent
  ///
  /// @return Returns a pointer to a call instruction instance.
  llvm::CallInst *createBuiltinCall(llvm::StringRef name, llvm::Type *retTy,
                                    llvm::ArrayRef<llvm::Value *> args,
                                    bool convergent = false);

  /// @brief Creates a call to a mangled builtin function.
  ///
  /// @note In order to correctly mangle builtin function names the list of
  /// SPIR-V ID's used as inputs to the builtin function must also be provided.
  ///
  /// @param name Name of the builtin function, `name` will be mangled.
  /// @param retTy Builtin function return type.
  /// @param retOp The ID of the return type opcode.
  /// @param args List of the builtin function parameter values.
  /// @param mangleInfo List of the builtin function parameter mangling infos.
  /// @param convergent True if the called builtin is convergent
  /// `args`.
  ///
  /// @return Returns a pointer to a call instruction instance.
  llvm::CallInst *createMangledBuiltinCall(
      llvm::StringRef name, llvm::Type *retTy, MangleInfo retOp,
      llvm::ArrayRef<llvm::Value *> args, llvm::ArrayRef<MangleInfo> mangleInfo,
      bool convergent = false);

  /// @brief Helper function for constructing calls to conversion builtins.
  ///
  /// Generates the appropriate function call to convert `value` into `retTy`.
  ///
  /// @param value Argument to pass to the conversion builtin.
  /// @param argMangleInfo SPIR-V ID of `value`.
  /// @param retTy Type that `value` will be converted into.
  /// @param retMangleInfo SPIR-V ID of `retTy`.
  /// @param resultId Result ID of the conversion, for checking decorations.
  /// @param saturated Whether we already know this is a saturated conversion.
  ///
  /// @return Returns a pointer to a call instruction instance.
  llvm::CallInst *createConversionBuiltinCall(
      llvm::Value *value, llvm::ArrayRef<MangleInfo> argMangleInfo,
      llvm::Type *retTy, MangleInfo retMangleInfo, spv::Id resultId,
      bool saturated = false);

  /// @brief Creates a call to an image access builtin.
  ///
  /// These builtins need a suffix on their name appropriate to the pixel type
  /// being accessed, this wrapper around `createMangledBuiltinCall` adds that
  /// suffix.
  ///
  /// @param name Name of the access function: `read_image` or `write_image`.
  /// @param retTy Builtin function return type.
  /// @param retOp The ID of the return type opcode.
  /// @param args List of the builtin function parameter values.
  /// @param argMangleInfo List of the builtin function parameter SPIR-V ID's.
  /// @param pixelTypeOp OpCode object representing the type of the pixel being
  /// accessed.
  ///
  /// @return Returns a pointer to a call instruction instance.
  llvm::CallInst *createImageAccessBuiltinCall(
      std::string name, llvm::Type *retTy, MangleInfo retMangleInfo,
      llvm::ArrayRef<llvm::Value *> args,
      llvm::ArrayRef<MangleInfo> argMangleInfo,
      const spirv_ll::OpTypeVector *pixelTypeOp);

  /// @brief Creates a call to an OpenCL builtin.
  ///
  /// @param opcode The OpenCL builtin function to call.
  /// @param resultType Result type of the call.
  /// @param params Array of parameters to pass to the call.
  llvm::Value *createOCLBuiltinCall(OpenCLLIB::Entrypoints opcode,
                                    spv::Id resultType,
                                    llvm::ArrayRef<spv::Id> params);

  /// @brief Get rounding mode suffix for a conversion function.
  ///
  /// @param roundingMode FP rounding mode to get the suffix for.
  ///
  /// @return Returns a string containing the suffix.
  std::string getFPRoundingModeSuffix(uint32_t roundingMode) {
    switch (roundingMode) {
      case spv::FPRoundingModeRTE:
        return "_rte";
      case spv::FPRoundingModeRTZ:
        return "_rtz";
      case spv::FPRoundingModeRTP:
        return "_rtp";
      case spv::FPRoundingModeRTN:
        return "_rtn";
      default:
        llvm_unreachable("unsupported FPRoundingMode decoration");
    }
  }

  /// @brief Applies the mangled length to a function name.
  ///
  /// @param name Function name to apply mangled length to.
  std::string applyMangledLength(llvm::StringRef name) {
    return "_Z" + std::to_string(name.size()) + name.str();
  }

  /// @brief Returns true if the type is substitutable during mangling.
  bool isSubstitutableArgType(llvm::Type *ty) {
    return !ty->isIntegerTy() && !ty->isFloatingPointTy();
  }

  /// @brief Get the mangled name prefix of a pointer type.
  ///
  /// @param ty The `llvm::Type` representing the pointer.
  /// @param qualifier TypeQualifiers to mangle with pointer with (optional).
  ///
  /// @return Returns a string containing the mangled pointer prefix.
  std::string getMangledPointerPrefix(
      llvm::Type *ty, uint8_t qualifier = MangleInfo::TypeQualifier::NONE) {
    SPIRV_LL_ASSERT(ty->isPointerTy(), "mangler: not a pointer type");
    std::string mangled = "P";
    if (auto addrspace = ty->getPointerAddressSpace()) {
      mangled += "U3AS" + std::to_string(addrspace);
    }
    if (qualifier & MangleInfo::VOLATILE) {
      mangled += "V";
    }
    if (qualifier & MangleInfo::CONST) {
      mangled += "K";
    }
    return mangled;
  }

  /// @brief Get the mangled name prefix of a vector type.
  ///
  /// @param ty The `llvm::Type` representing the vector.
  ///
  /// @return Returns a string containing the mangled vector prefix.
  std::string getMangledVecPrefix(llvm::Type *ty) {
    SPIRV_LL_ASSERT(ty->isVectorTy(), "mangler: not a vector type");
    const uint32_t numElements = multi_llvm::getVectorNumElements(ty);
    switch (numElements) {
      case 2:
        return "Dv2_";
      case 3:
        return "Dv3_";
      case 4:
        return "Dv4_";
      case 8:
        return "Dv8_";
      case 16:
        return "Dv16_";
      default:
        llvm_unreachable("mangler: unsupported vector width");
    }
  }

  /// @brief Get the mangled vector name prefix of a type, if a vector type.
  /// Else returns an empty string.
  std::string getMangledVecPrefixIfVec(llvm::Type *ty) {
    return ty->isVectorTy() ? getMangledVecPrefix(ty) : "";
  }

  /// @brief Get the mangled name of an integer type.
  ///
  /// @param ty The `llvm::Type` representing the integer.
  /// @param isSigned Flag to specify the signedness of the integer.
  ///
  /// @return Returns a string containing the mangled integer name.
  std::string getMangledIntName(llvm::Type *ty, bool isSigned) {
    auto elemTy = ty->isVectorTy() ? multi_llvm::getVectorElementType(ty) : ty;
    SPIRV_LL_ASSERT(elemTy->isIntegerTy(), "mangler: not an integer type");
    std::string name = ty->isVectorTy() ? getMangledVecPrefix(ty) : "";
    switch (elemTy->getIntegerBitWidth()) {
      case 8:
        // Ignore the explicit `signed char` case 'a' since it never occurs in
        // builtin function signatures.
        name += isSigned ? "c" : "h";
        break;
      case 16:
        name += isSigned ? "s" : "t";
        break;
      case 32:
        name += isSigned ? "i" : "j";
        break;
      case 64:
        name += isSigned ? "l" : "m";
        break;
      default:
        llvm_unreachable("mangler: unsupported integer bitwidth");
    }
    return name;
  }

  /// @brief Get the mangled name of a floating point type.
  ///
  /// @param ty The `llvm::Type` representing the floating point.
  ///
  /// @return Returns a string containing the mangled floating point name.
  std::string getMangledFPName(llvm::Type *ty) {
    auto elemTy = ty->isVectorTy() ? multi_llvm::getVectorElementType(ty) : ty;
    SPIRV_LL_ASSERT(elemTy->isFloatingPointTy(),
                    "mangler: not a floating-point type");
    std::string name = ty->isVectorTy() ? getMangledVecPrefix(ty) : "";
    switch (elemTy->getScalarSizeInBits()) {
      case 16:
        name += "Dh";
        break;
      case 32:
        name += "f";
        break;
      case 64:
        name += "d";
        break;
      default:
        llvm_unreachable("mangler: unsupported floating-point type");
    }
    return name;
  }

  /// @brief Get the mangled name of a sampler struct type.
  ///
  /// @param ty The `llvm::Type` representing the sampler image type.
  ///
  /// @return Returns a string containing the mangled sampler struct name.
  std::string getMangledSamplerName(llvm::Type *ty) {
    SPIRV_LL_ASSERT(ty->isIntegerTy(), "mangler: not a sampler type");
    (void)ty;
    return "11ocl_sampler";
  }

  /// @brief Join and possibly substitute mangled argument names.
  ///
  /// @param names List of mangled argument names.
  ///
  /// @return Returns a string containing the list of possibly substituted
  /// argument names.
  std::string joinMangledArgNames(llvm::ArrayRef<std::string> names) {
    std::string joined;
    llvm::SmallVector<llvm::StringRef, 16> subs;
    for (size_t nameIndex = 0; nameIndex < names.size(); nameIndex++) {
      const llvm::StringRef name(names[nameIndex]);
      if (name.starts_with("Dv")) {
        auto found = std::find(subs.begin(), subs.end(), name);
        if (found == subs.end()) {
          subs.push_back(name);
          joined += name;
        } else {
          auto arg = std::find(names.begin(), names.end(), *found);
          SPIRV_LL_ASSERT(arg != names.end(), "");
          auto argIndex = std::distance(names.begin(), arg);
          if (argIndex == 0) {
            // Omit the index when the substitute type is the first argument
            joined += "S_";
          } else {
            // Subsequent substitutions start at index 0
            joined += "S" + std::to_string(argIndex - 1) + "_";
          }
        }
      } else {
        joined += name;
      }
    }
    return joined;
  }

 private:
  /// @brief Definitions of OpenCL `mem_fence_flags` for barrier intructions
  enum MemFenceFlags { LOCAL_MEM_FENCE = 1u, GLOBAL_MEM_FENCE = 2u };

  /// @brief Definitions of OpenCL `memory_scope` for fence instruction.
  enum MemFenceScopes : uint32_t {
    WORK_ITEM = 1u,
    SUB_GROUP = 2u,
    WORK_GROUP = 3u,
    DEVICE = 4u,
    ALL_SVM_DEVICES = 5u,
    ALL_DEVICES = 6u,
    MEM_FENCE_SCOPES_MAX = 7u
  };

  /// @brief Generate the mangled function name.
  ///
  /// @param name Name of the function.
  /// @param args List of argument values.
  /// @param ids List of argument SPIR-V ID's.
  ///
  /// @return Returns a string containing the mangled function name.
  std::string getMangledFunctionName(std::string name,
                                     llvm::ArrayRef<llvm::Value *> args,
                                     llvm::ArrayRef<MangleInfo> argMangleInfo);

  /// @brief State to maintain a list of substitutable mangled types.
  struct SubstitutableType {
    /// @brief The type which is substitutable.
    llvm::Type *ty;
    /// @brief The argument index of the substitutable type.
    std::size_t index;
    /// @brief The opcode of the substitutable type.
    std::optional<MangleInfo> mangleInfo;
  };

  /// @brief Checks if function parameter can be substituted.
  ///
  /// @param ty Function parameter type.
  /// @param subTys List of substitutable types previously defined.
  /// @param op SPIR-V type of the function paramater.
  ///
  /// @return Returns a pointer to a SubstitutableType if the types match or
  /// nullptr if the function parameter can not be substituted.
  const SubstitutableType *substitutableArg(
      llvm::Type *ty, const llvm::ArrayRef<SubstitutableType> &subTys,
      std::optional<MangleInfo> mangleInfo);

  /// @brief Generate the mangled name for a function parameter type.
  ///
  /// @param ty Function parameter type.
  /// @param op SPIR-V type of the function paramater.
  /// @param subTys List of substitutable types previously defined.
  ///
  /// @return Returns a string containing the mangled name.
  std::string getMangledTypeName(llvm::Type *ty,
                                 std::optional<MangleInfo> mangleInfo,
                                 llvm::ArrayRef<SubstitutableType> subTys);

  /// @brief Creates a declaration for a builtin function inside of the current
  /// module and returns a pointer to the created function declaration
  ///
  /// @param name The mangled name of the builtin function to be declared
  /// @param ty The type of the function
  /// @param convergent True if the declared builtin is convergent
  ///
  /// @return A forward-declared llvm::Function with mangled name and function
  /// type ty.
  llvm::Function *declareBuiltinFunction(const std::string &name,
                                         llvm::FunctionType *ty,
                                         bool convergent = false);

  /// @brief Generates the IR for a binary (two operand) atomic instruction
  ///
  /// All of these instructions are identical, this aims to reduce all of that
  /// shared code into one helper function.
  ///
  /// @param op The `OpCode` object created for the instruction, this takes the
  /// `OpResult` type because we need to use the IdResult and this way we can
  /// roll two things into one function parameter
  /// @param pointerID ID of the pointer operand
  /// @param valueID ID of the value operand
  /// @param function String name of the atomic builtin function to call
  /// @param args_are_signed Signedness of the arguments
  void generateBinaryAtomic(const OpResult *op, spv::Id pointerID,
                            spv::Id valueID, const std::string &function,
                            bool args_are_signed);

  /// @brief Helper function for handling OpGroup.* operations.
  ///
  /// This function should be used to handle any of the following:
  /// OpGroupIAdd, OpGroupFAdd, OpGroupSMin, OpGroupUMin, OpGroupFMin,
  /// OpGroupSMax, OpGroupUMax, OpGroupFMax.
  ///
  /// Although this function is named generateReduction, it should also be used
  /// to handle any inclusive/exclusive scans since they are encoded in the same
  /// instructions as reductions in SPIR-V, although it doesn't do this at
  /// present.
  ///
  /// @tparam T Type of the SPIR-V op.
  /// @param op The SPIR-V op to generate LLVM IR for.
  /// @param name The name of the op e.g. add for OpGroupIAdd.
  template <typename T>
  void generateReduction(
      const T *op, const std::string &name,
      MangleInfo::ForceSignInfo signInfo = MangleInfo::ForceSignInfo::None);

  /// @brief Helper function for handling OpGroup(Any|All) operations.
  ///
  /// This function should be used to handle any of the following:
  /// OpGroupAny, OpGroupAll.
  ///
  /// @tparam T Type of the SPIR-V op.
  /// @param op The SPIR-V op to generate LLVM IR for.
  /// @param name The name of the op e.g. any for OpGroupAny.
  template <typename T>
  void generatePredicate(const T *op, const std::string &name);

  /// @brief Helper function for checking if the result of an access chain
  /// has decorations that need to be applied to it.
  ///
  /// This is to accomodate `OpMemberDecorate`. Since members of structs don't
  /// start with IDs to decorate we instead need to watch for accesses to
  /// structs. This checks if an `OpAccessChain` is going to create a pointer to
  /// a member that has been decorated, and if it is it applies the decoration
  /// to the new pointer.
  ///
  /// @param structTy struct type the AccessChain is indexing into.
  /// @param indexes List of AccessChain indexes.
  /// @param resultID ID of the AccessChain's resulting pointer value.
  void checkMemberDecorations(
      llvm::Type *structTy, const llvm::SmallVector<llvm::Value *, 8> &indexes,
      spv::Id resultID);

  /// @brief Generates IR for all `OpSpecConstantOp` instructions that had been
  /// deferred.
  ///
  /// This must be done on a per-function basis to ensure the constants are in
  /// scope everywhere as they should be, the instructions are generated at the
  /// very top of the function.
  void generateSpecConstantOps();

  /// @brief Generate the IR needed to give entry point parameters global scope.
  ///
  /// This is called after the first basic block in a function is created. If
  /// the function is an entry point it has stores generated to put its
  /// arguments in global variables, otherwise it has loads generated to get the
  /// contents of those globals into local scope. This is only necessary for
  /// GLCompute modules, as their inputs are represented as global pointers but
  /// they must be translated into entry point parameters for core.
  void handleGlobalParameters();

  /// @brief Registers an extended instruction set handler with an instruction
  /// set ID.
  ///
  /// Each handler is created only once per set.
  template <typename Handler, typename... Args>
  void registerExtInstHandler(ExtendedInstrSet Set, Args... args) {
    auto &builder = ext_inst_handlers[Set];
    if (!builder) {
      builder =
          std::make_unique<Handler>(*this, module, std::forward<Args>(args)...);
    }
  }
  /// @brief Add debug metadata to the appropriate instructions
  void addDebugInfoToModule();

  /// @brief Replaces all references to global builtin variables with a
  /// thread-safe function local definition
  void replaceBuiltinGlobals();

  /// @brief Finalizes and adds any metadata to LLVM that was generated by
  /// SpvBuilder
  void finalizeMetadata();

  /// @brief Returns an extended instruction set handler for the instruction
  /// set ID.
  ///
  /// @return A pointer to the handler if registered; nullptr otherwise.
  spirv_ll::ExtInstSetHandler *getExtInstHandler(ExtendedInstrSet set) const;

  /// @brief Determine the return type of a relational builtin from its operand.
  ///
  /// @param operand An operand that will be passed to the relational builtin.
  ///
  /// Some relational builtins such as `isordered` take multiple operands but
  /// they never take operands of different types so this function can deal with
  /// all cases.
  llvm::Type *getRelationalReturnType(llvm::Value *operand);

  /// @brief The `spirv_ll::Context` to built within.
  spirv_ll::Context &context;
  /// @brief The `spirv_ll::Module` being built.
  spirv_ll::Module &module;
  /// @brief The `spirv_ll::DeviceInfo` to target.
  const spirv_ll::DeviceInfo &deviceInfo;
  /// @brief The IRBuilder used to generate the LLVM IR
  llvm::IRBuilder<> IRBuilder;
  /// @brief The DIBuilder used to generate the LLVM IR debug instructions
  llvm::DIBuilder DIBuilder;
  /// @brief Function the builder is currently working on
  llvm::Function *CurrentFunction;
  /// @brief A copy of the current function's argument list
  llvm::SmallVector<llvm::Value *, 8> CurrentFunctionArgs;
  /// @brief Current debug scope of the function the builder is currently
  /// working on (or std::nullopt if no debug scope is active)
  std::optional<LexicalScopeTy> CurrentFunctionLexicalScope;
  /// @brief Current line range - marked by the beginning of an OpLine
  /// instruction - (or std::nullopt if no line range is active)
  std::optional<LineRangeBeginTy> CurrentOpLineRange;
  /// @brief A list of the builtin IDs specified at `CurrentFunction`'s creation
  BuiltinIDList CurrentFunctionBuiltinIDs;
  /// @brief Registered extended instruction set handlers.
  std::unordered_map<spirv_ll::ExtendedInstrSet,
                     std::unique_ptr<spirv_ll::ExtInstSetHandler>>
      ext_inst_handlers;
};
}  // namespace spirv_ll

#endif  // SPIRV_LL_SPV_BUILDER_H_INCLUDED
