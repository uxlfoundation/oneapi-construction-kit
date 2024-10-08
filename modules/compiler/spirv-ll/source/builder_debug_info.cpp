
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

// Note - this consumer/translator has been written to primarily be compatible
// with DebugInfo and OpenCL.DebugInfo.100 instructions found in SPIR-V
// binaries produced by the official Khronos LLVM IR -> SPIR-V translator:
// llvm-spirv.
//
// As such, it contains several workarounds for bugs in that tool. It also
// expects certain underspecified aspects of the SPIR-V specifications in the
// format that llvm-spirv produces: e.g., undocumented 'Debug Operations'
// expression mappings and how DebugTypeArray is encoded.
//
// Because the only way to understand llvm-spirv's encoding and decoding
// process/quirks was to read its code, some of the code in this translator has
// been adapted from llvm-spirv.

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/builder_debug_info.h>
#include <spirv-ll/module.h>
#include <spirv-ll/opcodes.h>
#include <spirv/unified1/OpenCLDebugInfo100.h>
#include <spirv/unified1/spirv.hpp>

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace spirv_ll {

static std::unordered_map<uint32_t, llvm::dwarf::TypeKind> DebugEncodingMap = {
    {OpenCLDebugInfo100Unspecified, static_cast<llvm::dwarf::TypeKind>(0)},
    {OpenCLDebugInfo100Address, llvm::dwarf::DW_ATE_address},
    {OpenCLDebugInfo100Boolean, llvm::dwarf::DW_ATE_boolean},
    {OpenCLDebugInfo100Float, llvm::dwarf::DW_ATE_float},
    {OpenCLDebugInfo100Signed, llvm::dwarf::DW_ATE_signed},
    {OpenCLDebugInfo100SignedChar, llvm::dwarf::DW_ATE_signed_char},
    {OpenCLDebugInfo100Unsigned, llvm::dwarf::DW_ATE_unsigned},
    {OpenCLDebugInfo100UnsignedChar, llvm::dwarf::DW_ATE_unsigned_char},
};

static std::unordered_map<uint32_t, llvm::dwarf::Tag> DebugQualifierMap = {
    {OpenCLDebugInfo100ConstType, llvm::dwarf::DW_TAG_const_type},
    {OpenCLDebugInfo100VolatileType, llvm::dwarf::DW_TAG_volatile_type},
    {OpenCLDebugInfo100RestrictType, llvm::dwarf::DW_TAG_restrict_type},
    {OpenCLDebugInfo100AtomicType, llvm::dwarf::DW_TAG_atomic_type},
};

static std::unordered_map<uint32_t, llvm::dwarf::LocationAtom>
    DebugOperationMap = {
        {OpenCLDebugInfo100Deref, llvm::dwarf::DW_OP_deref},
        {OpenCLDebugInfo100Plus, llvm::dwarf::DW_OP_plus},
        {OpenCLDebugInfo100Minus, llvm::dwarf::DW_OP_minus},
        {OpenCLDebugInfo100PlusUconst, llvm::dwarf::DW_OP_plus_uconst},
        {OpenCLDebugInfo100BitPiece, llvm::dwarf::DW_OP_bit_piece},
        {OpenCLDebugInfo100Swap, llvm::dwarf::DW_OP_swap},
        {OpenCLDebugInfo100Xderef, llvm::dwarf::DW_OP_xderef},
        {OpenCLDebugInfo100StackValue, llvm::dwarf::DW_OP_stack_value},
        {OpenCLDebugInfo100Constu, llvm::dwarf::DW_OP_constu},
        {OpenCLDebugInfo100Fragment, llvm::dwarf::DW_OP_LLVM_fragment},
        // Note - the Khronos translator supports the following opcodes that
        // aren't defined in the specification, primarily because LLVM supports
        // them. We must either support them or not emit expressions. This is
        // the better option.
        // Note however that we must keep this list in sync with the Khronos
        // translator - the key values are *not* just the unsigned values of
        // the LLVM enumerators! See e.g., DW_OP_const1u which is '8' in
        // LLVM/DWARF, but since the DebugInfo spec encodes
        // OpenCLDebugInfo100Constu in that spot, the Khronos tool has chosen
        // '12' instead.
        {10, llvm::dwarf::DW_OP_LLVM_convert},
        {11, llvm::dwarf::DW_OP_addr},
        {12, llvm::dwarf::DW_OP_const1u},
        {13, llvm::dwarf::DW_OP_const1s},
        {14, llvm::dwarf::DW_OP_const2u},
        {15, llvm::dwarf::DW_OP_const2s},
        {16, llvm::dwarf::DW_OP_const4u},
        {17, llvm::dwarf::DW_OP_const4s},
        {18, llvm::dwarf::DW_OP_const8u},
        {19, llvm::dwarf::DW_OP_const8s},
        {20, llvm::dwarf::DW_OP_consts},
        {21, llvm::dwarf::DW_OP_dup},
        {22, llvm::dwarf::DW_OP_drop},
        {23, llvm::dwarf::DW_OP_over},
        {24, llvm::dwarf::DW_OP_pick},
        {25, llvm::dwarf::DW_OP_rot},
        {26, llvm::dwarf::DW_OP_abs},
        {27, llvm::dwarf::DW_OP_and},
        {28, llvm::dwarf::DW_OP_div},
        {29, llvm::dwarf::DW_OP_mod},
        {30, llvm::dwarf::DW_OP_mul},
        {31, llvm::dwarf::DW_OP_neg},
        {32, llvm::dwarf::DW_OP_not},
        {33, llvm::dwarf::DW_OP_or},
        {34, llvm::dwarf::DW_OP_shl},
        {35, llvm::dwarf::DW_OP_shr},
        {36, llvm::dwarf::DW_OP_shra},
        {37, llvm::dwarf::DW_OP_xor},
        {38, llvm::dwarf::DW_OP_bra},
        {39, llvm::dwarf::DW_OP_eq},
        {40, llvm::dwarf::DW_OP_ge},
        {41, llvm::dwarf::DW_OP_gt},
        {42, llvm::dwarf::DW_OP_le},
        {43, llvm::dwarf::DW_OP_lt},
        {44, llvm::dwarf::DW_OP_ne},
        {45, llvm::dwarf::DW_OP_skip},
        {46, llvm::dwarf::DW_OP_lit0},
        {47, llvm::dwarf::DW_OP_lit1},
        {48, llvm::dwarf::DW_OP_lit2},
        {49, llvm::dwarf::DW_OP_lit3},
        {50, llvm::dwarf::DW_OP_lit4},
        {51, llvm::dwarf::DW_OP_lit5},
        {52, llvm::dwarf::DW_OP_lit6},
        {53, llvm::dwarf::DW_OP_lit7},
        {54, llvm::dwarf::DW_OP_lit8},
        {55, llvm::dwarf::DW_OP_lit9},
        {56, llvm::dwarf::DW_OP_lit10},
        {57, llvm::dwarf::DW_OP_lit11},
        {58, llvm::dwarf::DW_OP_lit12},
        {59, llvm::dwarf::DW_OP_lit13},
        {60, llvm::dwarf::DW_OP_lit14},
        {61, llvm::dwarf::DW_OP_lit15},
        {62, llvm::dwarf::DW_OP_lit16},
        {63, llvm::dwarf::DW_OP_lit17},
        {64, llvm::dwarf::DW_OP_lit18},
        {65, llvm::dwarf::DW_OP_lit19},
        {66, llvm::dwarf::DW_OP_lit20},
        {67, llvm::dwarf::DW_OP_lit21},
        {68, llvm::dwarf::DW_OP_lit22},
        {69, llvm::dwarf::DW_OP_lit23},
        {70, llvm::dwarf::DW_OP_lit24},
        {71, llvm::dwarf::DW_OP_lit25},
        {72, llvm::dwarf::DW_OP_lit26},
        {73, llvm::dwarf::DW_OP_lit27},
        {74, llvm::dwarf::DW_OP_lit28},
        {75, llvm::dwarf::DW_OP_lit29},
        {76, llvm::dwarf::DW_OP_lit30},
        {77, llvm::dwarf::DW_OP_lit31},
        {78, llvm::dwarf::DW_OP_reg0},
        {79, llvm::dwarf::DW_OP_reg1},
        {80, llvm::dwarf::DW_OP_reg2},
        {81, llvm::dwarf::DW_OP_reg3},
        {82, llvm::dwarf::DW_OP_reg4},
        {83, llvm::dwarf::DW_OP_reg5},
        {84, llvm::dwarf::DW_OP_reg6},
        {85, llvm::dwarf::DW_OP_reg7},
        {86, llvm::dwarf::DW_OP_reg8},
        {87, llvm::dwarf::DW_OP_reg9},
        {88, llvm::dwarf::DW_OP_reg10},
        {89, llvm::dwarf::DW_OP_reg11},
        {90, llvm::dwarf::DW_OP_reg12},
        {91, llvm::dwarf::DW_OP_reg13},
        {92, llvm::dwarf::DW_OP_reg14},
        {93, llvm::dwarf::DW_OP_reg15},
        {94, llvm::dwarf::DW_OP_reg16},
        {95, llvm::dwarf::DW_OP_reg17},
        {96, llvm::dwarf::DW_OP_reg18},
        {97, llvm::dwarf::DW_OP_reg19},
        {98, llvm::dwarf::DW_OP_reg20},
        {99, llvm::dwarf::DW_OP_reg21},
        {100, llvm::dwarf::DW_OP_reg22},
        {101, llvm::dwarf::DW_OP_reg23},
        {102, llvm::dwarf::DW_OP_reg24},
        {103, llvm::dwarf::DW_OP_reg25},
        {104, llvm::dwarf::DW_OP_reg26},
        {105, llvm::dwarf::DW_OP_reg27},
        {106, llvm::dwarf::DW_OP_reg28},
        {107, llvm::dwarf::DW_OP_reg29},
        {108, llvm::dwarf::DW_OP_reg30},
        {109, llvm::dwarf::DW_OP_reg31},
        {110, llvm::dwarf::DW_OP_breg0},
        {111, llvm::dwarf::DW_OP_breg1},
        {112, llvm::dwarf::DW_OP_breg2},
        {113, llvm::dwarf::DW_OP_breg3},
        {114, llvm::dwarf::DW_OP_breg4},
        {115, llvm::dwarf::DW_OP_breg5},
        {116, llvm::dwarf::DW_OP_breg6},
        {117, llvm::dwarf::DW_OP_breg7},
        {118, llvm::dwarf::DW_OP_breg8},
        {119, llvm::dwarf::DW_OP_breg9},
        {120, llvm::dwarf::DW_OP_breg10},
        {121, llvm::dwarf::DW_OP_breg11},
        {122, llvm::dwarf::DW_OP_breg12},
        {123, llvm::dwarf::DW_OP_breg13},
        {124, llvm::dwarf::DW_OP_breg14},
        {125, llvm::dwarf::DW_OP_breg15},
        {126, llvm::dwarf::DW_OP_breg16},
        {127, llvm::dwarf::DW_OP_breg17},
        {128, llvm::dwarf::DW_OP_breg18},
        {129, llvm::dwarf::DW_OP_breg19},
        {130, llvm::dwarf::DW_OP_breg20},
        {131, llvm::dwarf::DW_OP_breg21},
        {132, llvm::dwarf::DW_OP_breg22},
        {133, llvm::dwarf::DW_OP_breg23},
        {134, llvm::dwarf::DW_OP_breg24},
        {135, llvm::dwarf::DW_OP_breg25},
        {136, llvm::dwarf::DW_OP_breg26},
        {137, llvm::dwarf::DW_OP_breg27},
        {138, llvm::dwarf::DW_OP_breg28},
        {139, llvm::dwarf::DW_OP_breg29},
        {140, llvm::dwarf::DW_OP_breg30},
        {141, llvm::dwarf::DW_OP_breg31},
        {142, llvm::dwarf::DW_OP_regx},
        // Note - not supporting 143 -> Fbreg
        {144, llvm::dwarf::DW_OP_bregx},
        {145, llvm::dwarf::DW_OP_piece},
        {146, llvm::dwarf::DW_OP_deref_size},
        {147, llvm::dwarf::DW_OP_xderef_size},
        {148, llvm::dwarf::DW_OP_nop},
        {149, llvm::dwarf::DW_OP_push_object_address},
        {150, llvm::dwarf::DW_OP_call2},
        {151, llvm::dwarf::DW_OP_call4},
        {152, llvm::dwarf::DW_OP_call_ref},
        {153, llvm::dwarf::DW_OP_form_tls_address},
        {154, llvm::dwarf::DW_OP_call_frame_cfa},
        {155, llvm::dwarf::DW_OP_implicit_value},
        {156, llvm::dwarf::DW_OP_implicit_pointer},
        {157, llvm::dwarf::DW_OP_addrx},
        {158, llvm::dwarf::DW_OP_constx},
        {159, llvm::dwarf::DW_OP_entry_value},
        {160, llvm::dwarf::DW_OP_const_type},
        {161, llvm::dwarf::DW_OP_regval_type},
        {162, llvm::dwarf::DW_OP_deref_type},
        {163, llvm::dwarf::DW_OP_xderef_type},
        {164, llvm::dwarf::DW_OP_reinterpret},
        {165, llvm::dwarf::DW_OP_LLVM_arg},
        {166, llvm::dwarf::DW_OP_LLVM_implicit_pointer},
        {167, llvm::dwarf::DW_OP_LLVM_tag_offset},
};

static llvm::DINode::DIFlags translateLRValueReferenceFlags(
    uint32_t spv_flags) {
  llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;
  if (spv_flags & OpenCLDebugInfo100FlagLValueReference) {
    flags |= llvm::DINode::FlagLValueReference;
  }
  if (spv_flags & OpenCLDebugInfo100FlagRValueReference) {
    flags |= llvm::DINode::FlagRValueReference;
  }
  return flags;
}

static llvm::DINode::DIFlags translateAccessFlags(uint32_t spv_flags) {
  // This is a two-bit combination flag:
  //   Protected: 1 << 0
  //   Private: 1 << 1
  //   Public: (1 << 0) | (1 << 1)
  constexpr auto OpenCLDebugInfo100AccessMask = OpenCLDebugInfo100FlagIsPublic;

  llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;
  if ((spv_flags & OpenCLDebugInfo100AccessMask) ==
      OpenCLDebugInfo100FlagIsPublic) {
    flags |= llvm::DINode::FlagPublic;
  } else if ((spv_flags & OpenCLDebugInfo100AccessMask) ==
             OpenCLDebugInfo100FlagIsProtected) {
    flags |= llvm::DINode::FlagProtected;
  } else if ((spv_flags & OpenCLDebugInfo100AccessMask) ==
             OpenCLDebugInfo100FlagIsPrivate) {
    flags |= llvm::DINode::FlagPrivate;
  }
  return flags;
}

bool DebugInfoBuilder::isDebugInfoNone(spv::Id id) const {
  auto *const op = module.get_or_null(id);
  if (!op || !isa<OpExtInst>(op)) {
    return false;
  }
  auto *const variable_op_ext_inst = cast<OpExtInst>(op);
  return variable_op_ext_inst && isDebugInfoSet(variable_op_ext_inst->Set()) &&
         variable_op_ext_inst->Instruction() == OpenCLDebugInfo100DebugInfoNone;
}

bool DebugInfoBuilder::isDebugInfoSet(uint32_t set_id) const {
  auto set = module.getExtendedInstrSet(set_id);
  return set == ExtendedInstrSet::DebugInfo ||
         set == ExtendedInstrSet::OpenCLDebugInfo100;
}

llvm::DIBuilder &DebugInfoBuilder::getDefaultDIBuilder() const {
  assert(debug_builder_map.size() != 0 && "No DIBuilders");
  return *debug_builder_map.begin()->second;
}

llvm::Expected<std::optional<uint64_t>> DebugInfoBuilder::getConstantIntValue(
    spv::Id id) const {
  if (isDebugInfoNone(id)) {
    return std::nullopt;
  }
  llvm::Value *const constant_value = module.getValue(id);
  if (!llvm::isa<llvm::ConstantInt>(constant_value)) {
    return makeStringError("Op " + getIDAsStr(id, &module) +
                           " is not an integer OpConstant");
  }
  return llvm::cast<llvm::ConstantInt>(constant_value)->getZExtValue();
}

//===----------------------------------------------------------------------===//
// 4.2 Compilation Unit
//===----------------------------------------------------------------------===//

class DebugCompilationUnit : public OpExtInst {
 public:
  DebugCompilationUnit(const OpCode &other) : OpExtInst(other) {}
  uint32_t Version() const { return getOpExtInstOperand(0); }
  uint32_t DWARFVersion() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  spv::SourceLanguage SourceLanguage() const {
    return static_cast<spv::SourceLanguage>(getOpExtInstOperand(3));
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugCompilationUnit *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }

  const std::string flags = "";
  unsigned lang = llvm::dwarf::DW_LANG_OpenCL;
  switch (op->SourceLanguage()) {
    default:
      break;
    case spv::SourceLanguageOpenCL_CPP:
      lang = llvm::dwarf::SourceLanguage::DW_LANG_C_plus_plus_14;
      break;
    case spv::SourceLanguageSYCL:
    case spv::SourceLanguageCPP_for_OpenCL:
      lang = llvm::dwarf::SourceLanguage::DW_LANG_C_plus_plus_17;
      break;
  }

  llvm::DIFile *const file = file_or_error.get();

  auto *dib = debug_builder_map[op->IdResult()].get();
  assert(dib && "Should have already created a builder for this compile unit");

  const llvm::StringRef module_process = module.getModuleProcess();
  static constexpr char producer_prefix[] = "Debug info producer: ";

  std::string producer = "spirv";
  if (module_process.starts_with(producer_prefix)) {
    producer =
        module_process.drop_front(llvm::StringRef(producer_prefix).size());
  }

  llvm::DICompileUnit *di_cu =
      dib->createCompileUnit(lang, file, producer,
                             /*isOptimized*/ false, flags, /*RV*/ 0);

  if (!module.llvmModule->getModuleFlag("Dwarf Version")) {
    module.llvmModule->addModuleFlag(llvm::Module::Max, "Dwarf Version",
                                     op->DWARFVersion());
  }

  if (!module.llvmModule->getModuleFlag("Debug Info Version")) {
    module.llvmModule->addModuleFlag(llvm::Module::Warning,
                                     "Debug Info Version", 3);
  }

  return di_cu;
}

class DebugSource : public OpExtInst {
 public:
  DebugSource(const OpCode &other) : OpExtInst(other) {}
  spv::Id File() const { return getOpExtInstOperand(0); }
  std::optional<spv::Id> Text() const {
    return 1 < opExtInstOperandCount() ? getOpExtInstOperand(1)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugSource *op) {
  const std::string filePath = module.getDebugString(op->File()).value_or("");
  const std::string fileName =
      filePath.substr(filePath.find_last_of("\\/") + 1);
  const std::string fileDir = filePath.substr(0, filePath.find_last_of("\\/"));

  // Checksum parsing. We need to pass a llvm::StringRef to the LLVM API, so
  // need some std::string to hold it. It only needs to last as long as the API
  // call, as LLVM will convert the string to metadata with its own storage.
  std::string checksum_str_storage;
  std::optional<llvm::DIFile::ChecksumInfo<llvm::StringRef>> checksum;

  if (auto text_id = op->Text()) {
    // Text, e.g., %61 = OpString "//__CSK_MD5:8040a97cda029467f3f64c25e932a46e"
    if (std::optional<std::string> text_str = module.getDebugString(*text_id)) {
      checksum_str_storage = *text_str;
      const llvm::StringRef text = checksum_str_storage;

      static constexpr char checksum_kind_prefix[] = {"//__CSK_"};
      size_t kind_pos = text.find(checksum_kind_prefix);
      if (kind_pos != llvm::StringRef::npos) {
        const size_t colon_pos = text.find(":", kind_pos);
        kind_pos += std::string("//__").size();
        auto checksum_kind_str = text.substr(kind_pos, colon_pos - kind_pos);
        auto checksum_str = text.substr(colon_pos).ltrim(':');
        if (auto checksum_kind =
                llvm::DIFile::getChecksumKind(checksum_kind_str)) {
          const size_t checksum_end_pos =
              checksum_str.find_if_not(llvm::isHexDigit);
          checksum.emplace(checksum_kind.value(),
                           checksum_str.substr(0, checksum_end_pos));
        }
      }
    }
  }

  // It doesn't matter which DIBuilder we use to create a DIFile, as they
  // exist independently from the CompileUnit hierarchy.
  return getDefaultDIBuilder().createFile(fileName, fileDir, checksum);
}

//===----------------------------------------------------------------------===//
// 4.3 Type Instructions
//===----------------------------------------------------------------------===//

class DebugTypeBasic : public OpExtInst {
 public:
  DebugTypeBasic(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Size() const { return getOpExtInstOperand(1); }
  uint32_t Encoding() const { return getOpExtInstOperand(2); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeBasic *op) {
  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugTypeBasic " +
        getIDAsStr(op->IdResult(), &module));
  }

  auto encoding_iter = DebugEncodingMap.find(op->Encoding());
  if (encoding_iter == DebugEncodingMap.end() || encoding_iter->second == 0) {
    return getDIBuilder(op).createUnspecifiedType(*name);
  }
  const llvm::dwarf::TypeKind encoding = encoding_iter->second;

  auto size_or_error = getConstantIntValue(op->Size());
  if (auto err = size_or_error.takeError()) {
    return std::move(err);
  }
  // Without a size, we can't create a type.
  const std::optional<uint64_t> size = size_or_error.get();
  if (!size) {
    return nullptr;
  }

  return getDIBuilder(op).createBasicType(*name, *size, encoding);
}

class DebugTypePointer : public OpExtInst {
 public:
  DebugTypePointer(const OpCode &other) : OpExtInst(other) {}
  spv::Id BaseType() const { return getOpExtInstOperand(0); }
  uint32_t StorageClass() const {
    return static_cast<spv::StorageClass>(getOpExtInstOperand(1));
  }
  uint32_t Flags() const { return getOpExtInstOperand(2); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypePointer *op) {
  auto base_ty = translateDebugInst<llvm::DIType>(op->BaseType());
  if (auto err = base_ty.takeError()) {
    return std::move(err);
  }
  std::optional<unsigned> addrspace;
  auto addrspace_or_error =
      module.translateStorageClassToAddrSpace(op->StorageClass());
  if (auto err = addrspace_or_error.takeError()) {
    // Silently consume this error. We know that llvm-spirv will use ~0 to
    // represent "no address space", despite this being invalid SPIR-V.
    llvm::consumeError(std::move(err));
  } else {
    addrspace = addrspace_or_error.get();
  }

  const uint32_t flags = op->Flags();
  llvm::DIType *type = nullptr;
  llvm::DIBuilder &dib = getDIBuilder(op);

  if (flags & OpenCLDebugInfo100FlagLValueReference) {
    type = dib.createReferenceType(llvm::dwarf::DW_TAG_reference_type, *base_ty,
                                   /*size*/ 0, /*align*/ 0, addrspace);
  } else if (flags & OpenCLDebugInfo100FlagRValueReference) {
    type = dib.createReferenceType(llvm::dwarf::DW_TAG_rvalue_reference_type,
                                   *base_ty, /*size*/ 0,
                                   /*align*/ 0, addrspace);
  } else {
    // This is 32, 64, or 0 if no memory model is specified.
    const uint64_t size = module.getAddressingModel();
    type = dib.createPointerType(*base_ty, size, /*align*/ 0, addrspace);
  }

  if (flags & OpenCLDebugInfo100FlagObjectPointer) {
    type = dib.createObjectPointerType(type);
  } else if (flags & OpenCLDebugInfo100FlagArtificial) {
    type = dib.createArtificialType(type);
  }

  return type;
}

class DebugTypeQualifier : public OpExtInst {
 public:
  DebugTypeQualifier(const OpCode &other) : OpExtInst(other) {}
  spv::Id BaseType() const { return getOpExtInstOperand(0); }
  uint32_t TypeQualifier() const { return getOpExtInstOperand(1); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeQualifier *op) {
  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->BaseType());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  const llvm::dwarf::Tag tag = DebugQualifierMap[op->TypeQualifier()];
  llvm::DIType *const base_ty = base_ty_or_error.get();
  return getDIBuilder(op).createQualifiedType(tag, base_ty);
}

static uint64_t getDerivedSizeInBits(const llvm::DIType *Ty) {
  if (auto size = Ty->getSizeInBits()) {
    return size;
  }
  if (auto *DT = llvm::dyn_cast<const llvm::DIDerivedType>(Ty)) {
    if (auto *BT = llvm::dyn_cast<const llvm::DIType>(DT->getRawBaseType())) {
      return getDerivedSizeInBits(BT);
    }
  }
  return 0;
}

class DebugTypeArray : public OpExtInst {
 public:
  DebugTypeArray(const OpCode &other) : OpExtInst(other) {}
  spv::Id BaseType() const { return getOpExtInstOperand(0); }

  llvm::SmallVector<std::pair<spv::Id, spv::Id>, 4> ComponentCounts() const {
    // This is underspecified by SPIR-V, but to accommodate multi-dimensional
    // arrays, llvm-spirv encodes the array ops like:
    // { BaseType, upperBound1, upperBound2, ..., upperBoundN,
    //             lowerBound1, lowerBound2, ..., lowerBoundN }
    // We expect and consume/translate only this form.
    llvm::SmallVector<std::pair<spv::Id, spv::Id>, 4> component_counts;
    const size_t num_component_operands = (opExtInstOperandCount() - 1) / 2;

    for (uint16_t i = 1, e = num_component_operands + 1; i != e; i++) {
      component_counts.push_back(
          {getOpExtInstOperand(i),
           getOpExtInstOperand(i + num_component_operands)});
    }
    return component_counts;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeArray *op) {
  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->BaseType());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *base_ty = base_ty_or_error.get();
  llvm::DIBuilder &dib = getDIBuilder(op);

  size_t total_count = 1;
  llvm::SmallVector<llvm::Metadata *, 8> subscripts;

  for (auto [upperb_id, lowerb_id] : op->ComponentCounts()) {
    // Assume that the operand is either DebugInfoNone or OpConstant.
    auto upperb_or_err = getConstantIntValue(upperb_id);
    if (auto err = upperb_or_err.takeError()) {
      return std::move(err);
    }
    if (std::optional<uint64_t> upperb = upperb_or_err.get()) {
      const uint64_t count = *upperb;
      auto lowerb_or_err = getConstantIntValue(lowerb_id);
      if (auto err = lowerb_or_err.takeError()) {
        return std::move(err);
      }
      // The lower bound might be DebugInfoNone, in which we take it to be
      // zero.
      const uint64_t lower_bound = lowerb_or_err.get().value_or(0);
      subscripts.push_back(dib.getOrCreateSubrange(lower_bound, count));
      // Update the total element count of the array.
      //   count = -1 means that the array is empty
      total_count *= count > 0 ? static_cast<size_t>(count) : 0;
    }
  }

  const size_t size = getDerivedSizeInBits(base_ty) * total_count;
  const llvm::DINodeArray subscript_array = dib.getOrCreateArray(subscripts);

  return dib.createArrayType(size, /*AlignInBits*/ 0, base_ty, subscript_array);
}

class DebugTypeVector : public OpExtInst {
 public:
  DebugTypeVector(const OpCode &other) : OpExtInst(other) {}
  spv::Id BaseType() const { return getOpExtInstOperand(0); }
  uint32_t ComponentCount() const { return getOpExtInstOperand(1); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeVector *op) {
  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->BaseType());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const base_ty = base_ty_or_error.get();
  if (!base_ty) {
    return nullptr;
  }

  const uint32_t component_count = op->ComponentCount();
  const uint32_t size_count = (component_count == 3) ? 4 : component_count;
  const uint32_t size = getDerivedSizeInBits(base_ty) * size_count;

  llvm::DIBuilder &dib = getDIBuilder(op);
  llvm::Metadata *subscripts[1] = {dib.getOrCreateSubrange(0, component_count)};
  const llvm::DINodeArray subscript_array = dib.getOrCreateArray(subscripts);

  return dib.createVectorType(size, /*AlignInBits*/ 0, base_ty,
                              subscript_array);
}

class DebugTypedef : public OpExtInst {
 public:
  DebugTypedef(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id BaseType() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  static constexpr size_t ScopeIdx = 5;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypedef *op) {
  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->BaseType());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const base_ty = base_ty_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError("Could not find OpString 'Name' for DebugTypedef " +
                           getIDAsStr(op->IdResult(), &module));
  }

  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();
  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  return getDIBuilder(op).createTypedef(base_ty, *name, file, op->Line(),
                                        scope);
}

class DebugTypeFunction : public OpExtInst {
 public:
  DebugTypeFunction(const OpCode &other) : OpExtInst(other) {}
  uint32_t Flags() const { return getOpExtInstOperand(0); }
  spv::Id ReturnType() const { return getOpExtInstOperand(1); }
  llvm::SmallVector<spv::Id, 4> ParameterTypes() const {
    llvm::SmallVector<spv::Id, 4> parameters;
    for (uint16_t i = 2, e = opExtInstOperandCount(); i != e; i++) {
      parameters.push_back(getOpExtInstOperand(i));
    }
    return parameters;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeFunction *op) {
  const llvm::DINode::DIFlags flags =
      translateLRValueReferenceFlags(op->Flags());

  llvm::SmallVector<llvm::Metadata *, 16> elements;
  for (const spv::Id param_ty_id : op->ParameterTypes()) {
    auto param = translateDebugInst(param_ty_id);
    if (auto err = param.takeError()) {
      return std::move(err);
    }
    elements.push_back(*param);
  }

  llvm::DIBuilder &dib = getDIBuilder(op);
  const llvm::DITypeRefArray param_types = dib.getOrCreateTypeArray(elements);

  return dib.createSubroutineType(param_types, flags);
}

class DebugTypeEnum : public OpExtInst {
 public:
  DebugTypeEnum(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id UnderlyingType() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  static constexpr size_t ScopeIdx = 5;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  spv::Id Size() const { return getOpExtInstOperand(6); }
  uint32_t Flags() const { return getOpExtInstOperand(7); }
  llvm::SmallVector<std::pair<spv::Id, spv::Id>, 4> Enumerators() const {
    llvm::SmallVector<std::pair<spv::Id, spv::Id>, 4> enumerators;
    for (uint16_t i = 8, e = opExtInstOperandCount(); i + 1 < e; i += 2) {
      enumerators.push_back(
          {getOpExtInstOperand(i), getOpExtInstOperand(i + 1)});
    }
    return enumerators;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeEnum *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  const uint32_t spv_flags = op->Flags();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError("Could not find OpString 'Name' for DebugTypeEnum " +
                           getIDAsStr(op->IdResult(), &module));
  }

  llvm::DIBuilder &dib = getDIBuilder(op);

  const uint32_t align_in_bits = 0;

  auto size_or_error = getConstantIntValue(op->Size());
  if (auto err = size_or_error.takeError()) {
    return std::move(err);
  }
  auto size = size_or_error.get();
  // Without a size, we can't create a type.
  if (!size) {
    return nullptr;
  }
  const uint64_t size_in_bits = *size;

  if (spv_flags & OpenCLDebugInfo100FlagFwdDecl) {
    return dib.createForwardDecl(llvm::dwarf::DW_TAG_enumeration_type, *name,
                                 scope, file, op->Line(), /*RuntimeLang*/ 0,
                                 size_in_bits, align_in_bits);
  }

  llvm::SmallVector<llvm::Metadata *, 16> elements;
  for (auto [value_id, name_id] : op->Enumerators()) {
    std::optional<std::string> enumerator_name = module.getDebugString(name_id);
    if (!enumerator_name) {
      return makeStringError(
          "Could not find OpString 'Name' for DebugTypeEnum " +
          getIDAsStr(op->IdResult(), &module));
    }
    const uint64_t enumerator_val = 0;
    elements.push_back(dib.createEnumerator(*enumerator_name, enumerator_val));
  }
  const llvm::DINodeArray enumerators = dib.getOrCreateArray(elements);

  auto underlying_ty_or_error =
      translateDebugInst<llvm::DIType>(op->UnderlyingType());
  if (auto err = underlying_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *underlying_type = underlying_ty_or_error.get();
  ;
  return dib.createEnumerationType(scope, *name, file, op->Line(), size_in_bits,
                                   align_in_bits, enumerators, underlying_type);
}

class DebugTypeComposite : public OpExtInst {
 public:
  DebugTypeComposite(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  uint32_t Tag() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  static constexpr size_t ScopeIdx = 5;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  spv::Id LinkageName() const { return getOpExtInstOperand(6); }
  spv::Id Size() const { return getOpExtInstOperand(7); }
  uint32_t Flags() const { return getOpExtInstOperand(8); }
  llvm::SmallVector<spv::Id, 4> Members() const {
    llvm::SmallVector<spv::Id, 4> members;
    for (uint16_t i = 9, e = opExtInstOperandCount(); i != e; i++) {
      members.push_back(getOpExtInstOperand(i));
    }
    return members;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeComposite *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  const uint32_t spv_flags = op->Flags();
  llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;
  if (spv_flags & OpenCLDebugInfo100FlagFwdDecl) {
    flags |= llvm::DINode::FlagFwdDecl;
  }
  if (spv_flags & OpenCLDebugInfo100FlagTypePassByValue) {
    flags |= llvm::DINode::FlagTypePassByValue;
  }
  if (spv_flags & OpenCLDebugInfo100FlagTypePassByReference) {
    flags |= llvm::DINode::FlagTypePassByReference;
  }

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugTypeComposite " +
        getIDAsStr(op->IdResult(), &module));
  }
  // Allow this not to be set. We've seen llvm-spirv produce this, but it's
  // unclear whether or not it's invalid to do so.
  const std::string linkage_name =
      module.getDebugString(op->LinkageName()).value_or("");

  const uint64_t align = 0;

  auto size_or_error = getConstantIntValue(op->Size());
  if (auto err = size_or_error.takeError()) {
    return std::move(err);
  }
  // Without a size, we can't create a type.
  const std::optional<uint64_t> size = size_or_error.get();
  if (!size) {
    return nullptr;
  }

  llvm::DIType *const derived_from = nullptr;
  llvm::DICompositeType *composite_type = nullptr;

  // Create a composite type with an empty set of elements. We'll fix these up
  // later (see below and finalizeCompositeTypes) as they are currently
  // (possibly) forward references to other IDs we haven't visited yet.
  llvm::DIBuilder &dib = getDIBuilder(op);

  switch (op->Tag()) {
    case OpenCLDebugInfo100Class:
      // TODO: This would ideally be createClassType, but LLVM has a bug where
      // it creates a composite type with the llvm::dwarf::DW_TAG_struct_type
      // tag instead.
      composite_type = dib.createReplaceableCompositeType(
          llvm::dwarf::DW_TAG_class_type, *name, scope, file, op->Line(),
          /*RuntimeLang*/ 0, *size, align, flags, linkage_name);
      composite_type = llvm::MDNode::replaceWithDistinct(
          llvm::TempDICompositeType(composite_type));
      break;
    case OpenCLDebugInfo100Structure:
      composite_type = dib.createStructType(
          scope, *name, file, op->Line(), *size, align, flags, derived_from,
          /*Elements*/ llvm::DINodeArray(), /*RunTimeLang*/ 0,
          /*VTableHolder*/ nullptr, linkage_name);
      break;
    case OpenCLDebugInfo100Union:
      composite_type =
          dib.createUnionType(scope, *name, file, op->Line(), *size, align,
                              flags, llvm::DINodeArray(),
                              /*RunTimeLang*/ 0, linkage_name);
      break;
    default:
      break;
  }

  // Make a note of this composite type, so that we'll come back to it later
  // once all the forward references are resolved.
  composite_types.push_back(op->IdResult());

  return composite_type;
}

class DebugTypeMember : public OpExtInst {
 public:
  DebugTypeMember(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Type() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  spv::Id Scope() const { return getOpExtInstOperand(5); }
  spv::Id Offset() const { return getOpExtInstOperand(6); }
  spv::Id Size() const { return getOpExtInstOperand(7); }
  uint32_t Flags() const { return getOpExtInstOperand(8); }
  std::optional<spv::Id> Value() const {
    return 9 < opExtInstOperandCount() ? getOpExtInstOperand(9)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeMember *op) {
  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugTypeMember " +
        getIDAsStr(op->IdResult(), &module));
  }

  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  const uint32_t spv_flags = op->Flags();
  llvm::DINode::DIFlags flags = translateAccessFlags(spv_flags);
  if (spv_flags & OpenCLDebugInfo100FlagStaticMember) {
    flags |= llvm::DINode::FlagStaticMember;
  }

  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->Type());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const base_ty = base_ty_or_error.get();

  if (auto op_val = op->Value();
      (spv_flags & OpenCLDebugInfo100FlagStaticMember) && op_val) {
    llvm::Value *val = module.getValue(*op_val);
    if (!llvm::isa_and_present<llvm::Constant>(val)) {
      return makeStringError(
          "'Value' " + getIDAsStr(*op_val, &module) + " of DebugTypeMember " +
          getIDAsStr(op->IdResult(), &module) + " is not an OpConstant");
    }
#if LLVM_VERSION_GREATER_EQUAL(18, 0)
    return getDIBuilder(op).createStaticMemberType(
        scope, *name, file, op->Line(), base_ty, flags,
        cast<llvm::Constant>(val), llvm::dwarf::DW_TAG_variable);
#else
    return getDIBuilder(op).createStaticMemberType(scope, *name, file,
                                                   op->Line(), base_ty, flags,
                                                   cast<llvm::Constant>(val));
#endif
  }

  auto size_or_error = getConstantIntValue(op->Size());
  if (auto err = size_or_error.takeError()) {
    return std::move(err);
  }
  // Without a size, we can't create a type.
  const std::optional<uint64_t> size = size_or_error.get();
  if (!size) {
    return nullptr;
  }
  const uint64_t alignment = 0;

  auto offset_or_error = getConstantIntValue(op->Offset());
  if (auto err = offset_or_error.takeError()) {
    return std::move(err);
  }
  // Without an offset, we can't create a type.
  const std::optional<uint64_t> offset = offset_or_error.get();
  if (!offset) {
    return nullptr;
  }

  return getDIBuilder(op).createMemberType(scope, *name, file, op->Line(),
                                           *size, alignment, *offset, flags,
                                           base_ty);
}

class DebugTypeInheritance : public OpExtInst {
 public:
  DebugTypeInheritance(const OpCode &other) : OpExtInst(other) {}
  spv::Id Child() const { return getOpExtInstOperand(0); }
  static constexpr size_t ParentIdx = 1;
  spv::Id Parent() const { return getOpExtInstOperand(ParentIdx); }
  spv::Id Offset() const { return getOpExtInstOperand(2); }
  spv::Id Size() const { return getOpExtInstOperand(3); }
  uint32_t Flags() const { return getOpExtInstOperand(4); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeInheritance *op) {
  const llvm::DINode::DIFlags flags = translateAccessFlags(op->Flags());

  auto child_or_error = translateDebugInst<llvm::DIType>(op->Child());
  if (auto err = child_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const child = child_or_error.get();

  auto parent_or_error = translateDebugInst<llvm::DIType>(op->Parent());
  if (auto err = parent_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const parent = parent_or_error.get();

  auto offset_or_error = getConstantIntValue(op->Offset());
  if (auto err = offset_or_error.takeError()) {
    return std::move(err);
  }
  // Without an offset, we can't continue.
  const std::optional<uint64_t> offset = offset_or_error.get();
  if (!offset) {
    return nullptr;
  }

  return getDIBuilder(op).createInheritance(child, parent, *offset,
                                            /*VBPtrOffset*/ 0, flags);
}

class DebugTypePtrToMember : public OpExtInst {
 public:
  DebugTypePtrToMember(const OpCode &other) : OpExtInst(other) {}
  spv::Id MemberType() const { return getOpExtInstOperand(0); }
  static constexpr size_t ParentIdx = 1;
  spv::Id Parent() const { return getOpExtInstOperand(ParentIdx); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypePtrToMember *op) {
  auto member_ty_or_error = translateDebugInst<llvm::DIType>(op->MemberType());
  if (auto err = member_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const member_ty = member_ty_or_error.get();
  auto base_ty_or_error = translateDebugInst<llvm::DIType>(op->Parent());
  if (auto err = base_ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const base_ty = base_ty_or_error.get();
  return getDIBuilder(op).createMemberPointerType(member_ty, base_ty, 0);
}

//===----------------------------------------------------------------------===//
// 4.4 Templates
//===----------------------------------------------------------------------===//

class DebugTypeTemplate : public OpExtInst {
 public:
  DebugTypeTemplate(const OpCode &other) : OpExtInst(other) {}
  spv::Id Target() const { return getOpExtInstOperand(0); }
  llvm::SmallVector<spv::Id, 4> Parameters() const {
    llvm::SmallVector<spv::Id, 4> parameters;
    for (uint16_t i = 1, e = opExtInstOperandCount(); i != e; i++) {
      parameters.push_back(getOpExtInstOperand(i));
    }
    return parameters;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeTemplate *op) {
  const spv::Id target_id = op->Target();
  auto target_or_error = translateDebugInst(target_id);
  if (auto err = target_or_error.takeError()) {
    return std::move(err);
  }
  llvm::MDNode *target = target_or_error.get();

  llvm::SmallVector<llvm::Metadata *, 8> param_elts;
  for (const spv::Id param_id : op->Parameters()) {
    if (!param_id) {
      return nullptr;
    }
    auto param_or_error = translateDebugInst(param_id);
    if (auto err = param_or_error.takeError()) {
      return std::move(err);
    }
    if (!param_or_error.get()) {
      return nullptr;
    }
    param_elts.push_back(param_or_error.get());
  }
  llvm::DIBuilder &dib = getDIBuilder(op);
  const llvm::DINodeArray template_params = dib.getOrCreateArray(param_elts);

  if (auto *comp = llvm::dyn_cast_if_present<llvm::DICompositeType>(target)) {
    dib.replaceArrays(comp, comp->getElements(), template_params);
    return comp;
  }

  if (llvm::isa_and_present<llvm::DISubprogram>(target)) {
    // This constant matches with one used in
    // llvm::DISubprogram::getRawTemplateParams()
    constexpr unsigned template_params_idx = 9;
    target->replaceOperandWith(template_params_idx, template_params.get());
    return target;
  }

  return makeStringError("Unhandled template type");
}

class DebugTypeTemplateParameter : public OpExtInst {
 public:
  DebugTypeTemplateParameter(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id ActualType() const { return getOpExtInstOperand(1); }
  spv::Id Value() const { return getOpExtInstOperand(2); }
  spv::Id Source() const { return getOpExtInstOperand(3); }
  uint32_t Line() const { return getOpExtInstOperand(4); }
  uint32_t Column() const { return getOpExtInstOperand(5); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeTemplateParameter *op) {
  // We can't know the scope in which this template parameter type is defined.
  llvm::DIScope *const scope = nullptr;

  auto ty_or_error = translateDebugInst<llvm::DIType>(op->ActualType());
  if (auto err = ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const ty = ty_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugTypeTemplateParameter " +
        getIDAsStr(op->IdResult(), &module));
  }

  if (!isDebugInfoNone(op->Value())) {
    llvm::Value *const value_op = module.getValue(op->Value());
    if (!llvm::isa_and_present<llvm::Constant>(value_op)) {
      return makeStringError("'Value' " + getIDAsStr(op->Value(), &module) +
                             " of DebugTypeTemplateParameter " +
                             getIDAsStr(op->IdResult(), &module) +
                             " is not an OpConstant");
    }
    return getDIBuilder(op).createTemplateValueParameter(
        scope, *name, ty, /*IsDefault*/ false,
        llvm::cast<llvm::Constant>(value_op));
  }

  return getDIBuilder(op).createTemplateTypeParameter(scope, *name, ty,
                                                      /*IsDefault*/ false);
}

class DebugTypeTemplateTemplateParameter : public OpExtInst {
 public:
  DebugTypeTemplateTemplateParameter(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id TemplateName() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeTemplateTemplateParameter *op) {
  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError("Could not find OpString 'Name' " +
                           getIDAsStr(op->Name(), &module) +
                           " for DebugTypeTemplateTemplateParameter " +
                           getIDAsStr(op->IdResult(), &module));
  }

  std::optional<std::string> template_name =
      module.getDebugString(op->TemplateName());
  if (!template_name) {
    return makeStringError("Could not find OpString 'TemplateName' " +
                           getIDAsStr(op->TemplateName(), &module) +
                           " for DebugTypeTemplateTemplateParameter " +
                           getIDAsStr(op->IdResult(), &module));
  }

  // Note: while this SPIR-V instruction has a 'Source' representing the
  // program, LLVM expects either no context or a DICompileUnit.
  llvm::DIScope *const context = nullptr;

  return getDIBuilder(op).createTemplateTemplateParameter(
      context, *name, /*ty*/ nullptr, *template_name);
}

class DebugTypeTemplateParameterPack : public OpExtInst {
 public:
  DebugTypeTemplateParameterPack(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Source() const { return getOpExtInstOperand(1); }
  uint32_t Line() const { return getOpExtInstOperand(2); }
  uint32_t Column() const { return getOpExtInstOperand(3); }
  llvm::SmallVector<spv::Id, 4> TemplateParameters() const {
    llvm::SmallVector<spv::Id, 4> template_parameters;
    for (uint16_t i = 4, e = opExtInstOperandCount(); i != e; i++) {
      template_parameters.push_back(getOpExtInstOperand(i));
    }
    return template_parameters;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugTypeTemplateParameterPack *op) {
  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Source());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for "
        "DebugTypeTemplateParameterPack " +
        getIDAsStr(op->IdResult(), &module));
  }

  llvm::SmallVector<llvm::Metadata *, 8> pack_elements;
  for (const spv::Id param_id : op->TemplateParameters()) {
    auto param_or_error = translateDebugInst(param_id);
    if (auto err = param_or_error.takeError()) {
      return std::move(err);
    }
    pack_elements.push_back(param_or_error.get());
  }

  llvm::DIBuilder &dib = getDIBuilder(op);
  const llvm::DINodeArray pack = dib.getOrCreateArray(pack_elements);
  return dib.createTemplateParameterPack(scope, *name,
                                         /*Ty*/ nullptr, pack);
}

//===----------------------------------------------------------------------===//
// 4.5 Global Variables
//===----------------------------------------------------------------------===//

class DebugGlobalVariable : public OpExtInst {
 public:
  DebugGlobalVariable(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Type() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  spv::Id Scope() const { return getOpExtInstOperand(5); }
  spv::Id LinkageName() const { return getOpExtInstOperand(6); }
  spv::Id Variable() const { return getOpExtInstOperand(7); }
  uint32_t Flags() const { return getOpExtInstOperand(8); }
  std::optional<spv::Id> StaticMemberDecl() const {
    return 9 < opExtInstOperandCount() ? getOpExtInstOperand(9)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugGlobalVariable *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugGlobalVariable " +
        getIDAsStr(op->IdResult(), &module));
  }
  std::optional<std::string> linkage_name =
      module.getDebugString(op->LinkageName());
  if (!linkage_name) {
    return makeStringError(
        "Could not find OpString 'LinkageName' for DebugGlobalVariable " +
        getIDAsStr(op->IdResult(), &module));
  }

  auto ty_or_error = translateDebugInst<llvm::DIType>(op->Type());
  if (auto err = ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *const ty = ty_or_error.get();

  llvm::DIDerivedType *static_member_decl_ty = nullptr;
  if (auto static_member_decl = op->StaticMemberDecl()) {
    auto smd_or_error =
        translateDebugInst<llvm::DIDerivedType>(*static_member_decl);
    if (auto err = smd_or_error.takeError()) {
      return std::move(err);
    }
    static_member_decl_ty = smd_or_error.get();
  }

  const uint32_t spv_flags = op->Flags();
  const bool is_local = spv_flags & OpenCLDebugInfo100FlagIsLocal;
  const bool is_definition = spv_flags & OpenCLDebugInfo100FlagIsDefinition;

  llvm::DIGlobalVariableExpression *const var_decl =
      getDIBuilder(op).createGlobalVariableExpression(
          scope, *name, *linkage_name, file, op->Line(), ty, is_local,
          is_definition, /*expr*/ nullptr, static_member_decl_ty);

  // The 'Variable' is the <id> of the source global variable or constant
  // described by this instruction. If the variable is optimized out, this
  // operand must be DebugInfoNone.
  if (!isDebugInfoNone(op->Variable())) {
    // This could be a global variable or constant. We can only attach debug
    // info to global variables.
    llvm::Value *const var = module.getValue(op->Variable());
    auto *const global_var = llvm::dyn_cast_or_null<llvm::GlobalVariable>(var);
    if (global_var && !global_var->hasMetadata("dbg")) {
      global_var->addMetadata("dbg", *var_decl);
    }
  }

  return var_decl;
}

//===----------------------------------------------------------------------===//
// 4.6 Functions
//===----------------------------------------------------------------------===//

class DebugFunctionDeclaration : public OpExtInst {
 public:
  DebugFunctionDeclaration(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Type() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  spv::Id Scope() const { return getOpExtInstOperand(5); }
  spv::Id LinkageName() const { return getOpExtInstOperand(6); }
  uint32_t Flags() const { return getOpExtInstOperand(7); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugFunctionDeclaration *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  auto ty_or_error = translateDebugInst<llvm::DISubroutineType>(op->Type());
  if (auto err = ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DISubroutineType *const ty = ty_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugFunctionDeclaration " +
        getIDAsStr(op->IdResult(), &module));
  }
  std::optional<std::string> linkage_name =
      module.getDebugString(op->LinkageName());
  if (!linkage_name) {
    return makeStringError(
        "Could not find OpString 'LinkageName' for DebugFunctionDeclaration " +
        getIDAsStr(op->IdResult(), &module));
  }

  const uint32_t spv_flags = op->Flags();
  llvm::DINode::DIFlags flags = translateAccessFlags(spv_flags) |
                                translateLRValueReferenceFlags(spv_flags);
  if (spv_flags & OpenCLDebugInfo100FlagArtificial) {
    flags |= llvm::DINode::FlagArtificial;
  }
  if (spv_flags & OpenCLDebugInfo100FlagExplicit) {
    flags |= llvm::DINode::FlagExplicit;
  }
  if (spv_flags & OpenCLDebugInfo100FlagPrototyped) {
    flags |= llvm::DINode::FlagPrototyped;
  }

  const bool is_definition = spv_flags & OpenCLDebugInfo100FlagIsDefinition;
  const bool is_optimized = spv_flags & OpenCLDebugInfo100FlagIsOptimized;
  const bool is_local = spv_flags & OpenCLDebugInfo100FlagIsLocal;
  const llvm::DISubprogram::DISPFlags subprogram_flags =
      llvm::DISubprogram::toSPFlags(is_local, is_definition, is_optimized);

  llvm::DIBuilder &dib = getDIBuilder(op);

  // Here we create fake array of template parameters. If it was plain nullptr,
  // the template parameter operand would be removed in DISubprogram::getImpl.
  // But we want it to be there, because if there is DebugTypeTemplate
  // instruction refering to this function, transTypeTemplate method must be
  // able to replace the template parameter operand, thus it must be in the
  // operands list.
  const llvm::SmallVector<llvm::Metadata *, 8> elts;
  const llvm::DINodeArray template_params = dib.getOrCreateArray(elts);
  const llvm::DITemplateParameterArray template_params_array =
      template_params.get();

  llvm::DISubprogram *subprogram = nullptr;
  if (scope && (llvm::isa<llvm::DICompositeType>(scope) ||
                llvm::isa<llvm::DINamespace>(scope))) {
    subprogram =
        dib.createMethod(scope, *name, *linkage_name, file, op->Line(), ty,
                         /*VTableIndex*/ 0, /*ThisAdjustment*/ 0,
                         /*VTableHolder*/ nullptr, flags, subprogram_flags,
                         template_params_array);
  } else {
    // Since a function declaration doesn't have any retained nodes, resolve
    // the temporary placeholder for them immediately.
    subprogram = dib.createTempFunctionFwdDecl(
        scope, *name, *linkage_name, file, op->Line(), ty, /*ScopeLine*/ 0,
        flags, subprogram_flags, template_params_array);
    llvm::TempMDNode fwd_decl(cast<llvm::MDNode>(subprogram));
    subprogram = dib.replaceTemporary(std::move(fwd_decl), subprogram);
  }

  return subprogram;
}

class DebugFunction : public OpExtInst {
 public:
  DebugFunction(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Type() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  static constexpr size_t ScopeIdx = 5;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  spv::Id LinkageName() const { return getOpExtInstOperand(6); }
  uint32_t Flags() const { return getOpExtInstOperand(7); }
  uint32_t ScopeLine() const { return getOpExtInstOperand(8); }
  spv::Id Function() const { return getOpExtInstOperand(9); }
  std::optional<spv::Id> Declaration() const {
    return 10 < opExtInstOperandCount() ? getOpExtInstOperand(10)
                                        : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugFunction *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  std::optional<std::string> name = module.getDebugString(op->Name());
  if (!name) {
    return makeStringError("Could not find OpString 'Name' for DebugFunction " +
                           getIDAsStr(op->IdResult(), &module));
  }
  std::optional<std::string> linkage_name =
      module.getDebugString(op->LinkageName());
  if (!linkage_name) {
    return makeStringError(
        "Could not find OpString 'LinkageName' for DebugFunction " +
        getIDAsStr(op->IdResult(), &module));
  }

  const uint32_t spv_flags = op->Flags();
  llvm::DINode::DIFlags flags = translateAccessFlags(spv_flags) |
                                translateLRValueReferenceFlags(spv_flags);
  if (spv_flags & OpenCLDebugInfo100FlagArtificial) {
    flags |= llvm::DINode::FlagArtificial;
  }
  if (spv_flags & OpenCLDebugInfo100FlagExplicit) {
    flags |= llvm::DINode::FlagExplicit;
  }
  if (spv_flags & OpenCLDebugInfo100FlagPrototyped) {
    flags |= llvm::DINode::FlagPrototyped;
  }

  const bool is_definition = spv_flags & OpenCLDebugInfo100FlagIsDefinition;
  const bool is_optimized = spv_flags & OpenCLDebugInfo100FlagIsOptimized;
  const bool is_local = spv_flags & OpenCLDebugInfo100FlagIsLocal;
  const bool is_main_subprogram = module.getEntryPoint(op->Function());
  const llvm::DISubprogram::DISPFlags subprogram_flags =
      llvm::DISubprogram::toSPFlags(
          is_local, is_definition, is_optimized,
          /*virtuality*/ llvm::DISubprogram::SPFlagNonvirtual,
          is_main_subprogram);

  auto ty_or_error = translateDebugInst<llvm::DISubroutineType>(op->Type());
  if (auto err = ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DISubroutineType *const ty = ty_or_error.get();

  llvm::DIBuilder &dib = getDIBuilder(op);

  // Here we create fake array of template parameters. If it was plain
  // nullptr, the template parameter operand would be removed in
  // DISubprogram::getImpl. But we want it to be there, because if there is
  // DebugTypeTemplate instruction refering to this function,
  // transTypeTemplate method must be able to replace the template parameter
  // operand, thus it must be in the operands list.
  const llvm::SmallVector<llvm::Metadata *, 8> elts;
  const llvm::DINodeArray template_params = dib.getOrCreateArray(elts);
  const llvm::DITemplateParameterArray template_params_array =
      template_params.get();

  llvm::DISubprogram *decl = nullptr;
  if (auto decl_id = op->Declaration()) {
    auto decl_or_error = translateDebugInst<llvm::DISubprogram>(*decl_id);
    if (auto err = decl_or_error.takeError()) {
      return std::move(err);
    }
    decl = decl_or_error.get();
  }

  if (scope &&
      (llvm::isa<llvm::DICompositeType>(scope) ||
       llvm::isa<llvm::DINamespace>(scope)) &&
      !is_definition) {
    return dib.createMethod(scope, *name, *linkage_name, file, op->Line(), ty,
                            /*VTableIndex*/ 0, /*ThisAdjustment*/ 0,
                            /*VTableHolder*/ nullptr, flags, subprogram_flags,
                            template_params_array);
  } else {
    return dib.createFunction(scope, *name, *linkage_name, file, op->Line(), ty,
                              op->ScopeLine(), flags, subprogram_flags,
                              template_params_array, decl,
                              /*ThrownTypes*/ nullptr,
                              /*Annotations*/ nullptr);
  }
}

//===----------------------------------------------------------------------===//
// 4.7 Location Information
//===----------------------------------------------------------------------===//

class DebugLexicalBlock : public OpExtInst {
 public:
  DebugLexicalBlock(const OpCode &other) : OpExtInst(other) {}
  spv::Id Source() const { return getOpExtInstOperand(0); }
  uint32_t Line() const { return getOpExtInstOperand(1); }
  uint32_t Column() const { return getOpExtInstOperand(2); }
  static constexpr size_t ScopeIdx = 3;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  std::optional<spv::Id> Name() const {
    return 4 < opExtInstOperandCount() ? getOpExtInstOperand(4)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugLexicalBlock *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  if (auto name_id = op->Name()) {
    // This indicates a C++ namespace. The name may be empty.
    std::optional<std::string> name = module.getDebugString(*name_id);
    if (!name) {
      return makeStringError(
          "Could not find OpString 'Name' for DebugLexicalBlock " +
          getIDAsStr(op->IdResult(), &module));
    }
    return getDIBuilder(op).createNameSpace(scope, *name,
                                            /*InlinedNamespace*/ false);
  }
  return getDIBuilder(op).createLexicalBlock(scope, file, op->Line(),
                                             op->Column());
}

class DebugLexicalBlockDiscriminator : public OpExtInst {
 public:
  DebugLexicalBlockDiscriminator(const OpCode &other) : OpExtInst(other) {}
  spv::Id Source() const { return getOpExtInstOperand(0); }
  uint32_t Discriminator() const { return getOpExtInstOperand(1); }
  static constexpr size_t ScopeIdx = 2;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugLexicalBlockDiscriminator *op) {
  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  if (!scope) {
    return nullptr;
  }

  return getDIBuilder(op).createLexicalBlockFile(scope, file,
                                                 op->Discriminator());
}

class DebugScope : public OpExtInst {
 public:
  DebugScope(const OpCode &other) : OpExtInst(other) {}
  static constexpr size_t ScopeIdx = 0;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  std::optional<spv::Id> InlinedAt() const {
    return 1 < opExtInstOperandCount() ? getOpExtInstOperand(1)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Error DebugInfoBuilder::create<DebugScope>(const OpExtInst &opc) {
  auto *op = module.create<DebugScope>(opc);

  // Close any current scope.
  builder.closeCurrentLexicalScope(/*closing_line_range*/ false);

  llvm::Metadata *scope = nullptr;
  llvm::Metadata *inlined_at = nullptr;

  auto scope_or_error = translateDebugInst(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return err;
  }
  scope = scope_or_error.get();

  if (auto inlined_at_id = op->InlinedAt()) {
    auto inlined_at_or_error = translateDebugInst(*inlined_at_id);
    if (auto err = inlined_at_or_error.takeError()) {
      return err;
    }
    inlined_at = inlined_at_or_error.get();
  }

  // If we don't have a valid scope we can't proceed
  if (!scope) {
    return llvm::Error::success();
  }

  builder.setCurrentFunctionLexicalScope(
      Builder::LexicalScopeTy{scope, inlined_at});

  return llvm::Error::success();
}

class DebugNoScope : public OpExtInst {
 public:
  DebugNoScope(const OpCode &other) : OpExtInst(other) {}
};

template <>
llvm::Error DebugInfoBuilder::create<DebugNoScope>(const OpExtInst &) {
  builder.closeCurrentLexicalScope(/*closing_line_range*/ false);
  return llvm::Error::success();
}

class DebugInlinedAt : public OpExtInst {
 public:
  DebugInlinedAt(const OpCode &other) : OpExtInst(other) {}
  uint32_t Line() const { return getOpExtInstOperand(0); }
  static constexpr size_t ScopeIdx = 1;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  std::optional<spv::Id> Inlined() const {
    return 2 < opExtInstOperandCount() ? getOpExtInstOperand(2)
                                       : std::optional<spv::Id>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugInlinedAt *op) {
  const unsigned column = 0;

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  // If we don't have a valid scope we can't proceed
  if (!scope) {
    return nullptr;
  }

  llvm::Metadata *inlined = nullptr;
  if (auto inlined_id = op->Inlined()) {
    auto inlined_or_error = translateDebugInst(*inlined_id);
    if (auto err = inlined_or_error.takeError()) {
      return std::move(err);
    }
    inlined = inlined_or_error.get();
  }

  return llvm::DILocation::getDistinct(*module.context.llvmContext, op->Line(),
                                       column, scope, inlined);
}

//===----------------------------------------------------------------------===//
// 4.8 Local Variables
//===----------------------------------------------------------------------===//

class DebugLocalVariable : public OpExtInst {
 public:
  DebugLocalVariable(const OpCode &other) : OpExtInst(other) {}
  spv::Id Name() const { return getOpExtInstOperand(0); }
  spv::Id Type() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  uint32_t Line() const { return getOpExtInstOperand(3); }
  uint32_t Column() const { return getOpExtInstOperand(4); }
  static constexpr size_t ScopeIdx = 5;
  spv::Id Scope() const { return getOpExtInstOperand(ScopeIdx); }
  uint32_t Flags() const { return getOpExtInstOperand(6); }
  std::optional<uint32_t> ArgNumber() const {
    return 7 < opExtInstOperandCount() ? getOpExtInstOperand(7)
                                       : std::optional<uint32_t>();
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugLocalVariable *op) {
  std::optional<std::string> name = module.getDebugString(op->Name());

  if (!name) {
    return makeStringError(
        "Could not find OpString 'Name' for DebugLocalVariable " +
        getIDAsStr(op->IdResult(), &module));
  }

  const uint32_t spv_flags = op->Flags();
  llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;

  if (spv_flags & OpenCLDebugInfo100FlagArtificial) {
    flags |= llvm::DINode::FlagArtificial;
  }
  if (spv_flags & OpenCLDebugInfo100FlagObjectPointer) {
    flags |= llvm::DINode::FlagObjectPointer;
  }

  auto ty_or_error = translateDebugInst<llvm::DIType>(op->Type());
  if (auto err = ty_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIType *ty = ty_or_error.get();
  // This type might well be 'DebugInfoNone', which translates to nullptr.
  // In such a case, we can't proceed with this expression.
  if (!ty) {
    return nullptr;
  }

  auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
  if (auto err = file_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIFile *const file = file_or_error.get();

  auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
  if (auto err = scope_or_error.takeError()) {
    return std::move(err);
  }
  llvm::DIScope *const scope = scope_or_error.get();

  const uint32_t line = op->Line();

  if (auto arg_number = op->ArgNumber()) {
    // This is a parameter
    return getDIBuilder(op).createParameterVariable(
        scope, *name, *arg_number, file, line, ty,
        /*AlwaysPreserve*/ true, flags);
  }

  // Otherwise, this is a local variable
  return getDIBuilder(op).createAutoVariable(scope, *name, file, line, ty,
                                             /*AlwaysPreserve*/ true, flags);
}

class DebugDeclare : public OpExtInst {
 public:
  DebugDeclare(const OpCode &other) : OpExtInst(other) {}
  spv::Id LocalVariable() const { return getOpExtInstOperand(0); }
  spv::Id Variable() const { return getOpExtInstOperand(1); }
  spv::Id Expression() const { return getOpExtInstOperand(2); }
};

template <>
llvm::Error DebugInfoBuilder::create<DebugDeclare>(const OpExtInst &opc) {
  auto *op = module.create<DebugDeclare>(opc);

  llvm::Value *variable = module.getValue(op->Variable());

  // We must pass a non-null value to the debug intrinsics. If we don't have
  // one (it might be DebugInfoNone), bail here.
  if (!variable) {
    return llvm::Error::success();
  }

  auto di_local_or_error =
      translateDebugInst<llvm::DILocalVariable>(op->LocalVariable());
  if (auto err = di_local_or_error.takeError()) {
    return err;
  }
  llvm::DILocalVariable *di_local = di_local_or_error.get();

  auto expr_or_error = translateDebugInst<llvm::DIExpression>(op->Expression());
  if (auto err = expr_or_error.takeError()) {
    return err;
  }

  llvm::DIExpression *di_expr = expr_or_error.get();

  auto &IB = builder.getIRBuilder();
  auto *insert_bb = IB.GetInsertBlock();
  if (!insert_bb) {
    return makeStringError("DebugDeclare " +
                           getIDAsStr(op->IdResult(), &module) +
                           " not located in basic block");
  }

  auto insert_pt = IB.GetInsertPoint();

  const llvm::DebugLoc di_loc = llvm::DILocation::get(
      module.llvmModule->getContext(), di_local->getLine(),
      /*Column=*/0, di_local->getScope());

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
  assert(
      !module.llvmModule->IsNewDbgInfoFormat &&
      "Expected module to remain in old debug info format while being built");
  llvm::DbgInstPtr dbg_declare;
#else
  llvm::Instruction *dbg_declare;
#endif
  if (insert_pt == insert_bb->end()) {
    dbg_declare = getDIBuilder(op).insertDeclare(
        /*Storage*/ variable, di_local, di_expr, di_loc, insert_bb);
  } else {
    dbg_declare = getDIBuilder(op).insertDeclare(
        /*Storage*/ variable, di_local, di_expr, di_loc, &*insert_pt);
  }

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
  module.addID(opc.IdResult(), op, dbg_declare.get<llvm::Instruction *>());
#else
  module.addID(opc.IdResult(), op, dbg_declare);
#endif
  return llvm::Error::success();
}

class DebugValue : public OpExtInst {
 public:
  DebugValue(const OpCode &other) : OpExtInst(other) {}
  spv::Id LocalVariable() const { return getOpExtInstOperand(0); }
  spv::Id Variable() const { return getOpExtInstOperand(1); }
  spv::Id Expression() const { return getOpExtInstOperand(2); }
  llvm::SmallVector<spv::Id, 4> Indexes() const {
    llvm::SmallVector<spv::Id, 4> indexes;
    for (uint16_t i = 3, e = opExtInstOperandCount(); i != e; i++) {
      indexes.push_back(getOpExtInstOperand(i));
    }
    return indexes;
  }
};

template <>
llvm::Error DebugInfoBuilder::create<DebugValue>(const OpExtInst &opc) {
  auto *op = module.create<DebugValue>(opc);
  llvm::Value *variable = module.getValue(op->Variable());
  if (!variable) {
    return makeStringError(
        "Could not get LocalVariable " + getIDAsStr(op->Variable(), &module) +
        " for DebugValue " + getIDAsStr(op->IdResult(), &module));
  }

  auto di_local_or_error =
      translateDebugInst<llvm::DILocalVariable>(op->LocalVariable());
  if (auto err = di_local_or_error.takeError()) {
    return err;
  }
  llvm::DILocalVariable *di_local = di_local_or_error.get();

  auto &IB = builder.getIRBuilder();
  auto *insert_bb = IB.GetInsertBlock();
  if (!insert_bb) {
    return makeStringError("DebugValue " + getIDAsStr(op->IdResult(), &module) +
                           " not located in block");
  }

  auto insert_pt = IB.GetInsertPoint();
  const llvm::DebugLoc di_loc = llvm::DILocation::get(
      module.llvmModule->getContext(), di_local->getLine(),
      /*Column=*/0, di_local->getScope());

  auto expr_or_error = translateDebugInst<llvm::DIExpression>(op->Expression());
  if (auto err = expr_or_error.takeError()) {
    return err;
  }

  llvm::DIExpression *di_expr = expr_or_error.get();

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
  assert(
      !module.llvmModule->IsNewDbgInfoFormat &&
      "Expected module to remain in old debug info format while being built");
  llvm::DbgInstPtr dbg_value;
#else
  llvm::Instruction *dbg_value;
#endif
  if (insert_pt == insert_bb->end()) {
    dbg_value = getDIBuilder(op).insertDbgValueIntrinsic(
        variable, di_local, di_expr, di_loc, insert_bb);
  } else {
    dbg_value = getDIBuilder(op).insertDbgValueIntrinsic(
        variable, di_local, di_expr, di_loc, &*insert_pt);
  }

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
  module.addID(opc.IdResult(), op, dbg_value.get<llvm::Instruction *>());
#else
  module.addID(opc.IdResult(), op, dbg_value);
#endif
  return llvm::Error::success();
}

class DebugOperation : public OpExtInst {
 public:
  DebugOperation(const OpCode &other) : OpExtInst(other) {}
  spv::Id Operation() const { return getOpExtInstOperand(0); }
  llvm::SmallVector<spv::Id, 4> Operands() const {
    llvm::SmallVector<uint32_t, 4> operands;
    for (uint16_t i = 1, e = opExtInstOperandCount(); i != e; i++) {
      operands.push_back(getOpExtInstOperand(i));
    }
    return operands;
  }
};

class DebugExpression : public OpExtInst {
 public:
  DebugExpression(const OpCode &other) : OpExtInst(other) {}
  llvm::SmallVector<spv::Id, 4> Operation() const {
    llvm::SmallVector<spv::Id, 4> operations;
    for (uint16_t i = 0, e = opExtInstOperandCount(); i != e; i++) {
      operations.push_back(getOpExtInstOperand(i));
    }
    return operations;
  }
};

template <>
llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translate(
    const DebugExpression *op) {
  std::vector<uint64_t> address_expr_ops;

  for (const spv::Id operation_id : op->Operation()) {
    auto *operation_op =
        cast<DebugOperation>(module.get<OpExtInst>(operation_id));
    SPIRV_LL_ASSERT_PTR(operation_op);
    const uint32_t operation = operation_op->Operation();
    address_expr_ops.push_back(DebugOperationMap[operation]);
    for (uint32_t operand : operation_op->Operands()) {
      address_expr_ops.push_back(operand);
    }
  }
  const llvm::ArrayRef<uint64_t> address_expr(address_expr_ops.data(),
                                              address_expr_ops.size());

  return getDIBuilder(op).createExpression(address_expr);
}

//===----------------------------------------------------------------------===//
// 4.9 Macros
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// 4.10 Imported Entities
//===----------------------------------------------------------------------===//

// Note: llvm-spirv generates ImportedEntity instructions with an extra
// dummy parameter in the 3rd position! We work around this by optionally
// skipping it, depending on the number of operands in the instruction.
class DebugImportedEntity : public OpExtInst {
 public:
  DebugImportedEntity(const OpCode &other) : OpExtInst(other) {
    dummy_offset = opExtInstOperandCount() == 7 ? 0 : 1;
  }
  spv::Id Name() const { return getOpExtInstOperand(0); }
  uint32_t Tag() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2 + dummy_offset); }
  spv::Id Entity() const { return getOpExtInstOperand(3 + dummy_offset); }
  uint32_t Line() const { return getOpExtInstOperand(4 + dummy_offset); }
  uint32_t Column() const { return getOpExtInstOperand(5 + dummy_offset); }
  spv::Id Scope() const { return getOpExtInstOperand(getScopeIdx()); }

  size_t getScopeIdx() const { return 6 + dummy_offset; }

 private:
  size_t dummy_offset = 0;
};

template <>
llvm::Error DebugInfoBuilder::create<DebugImportedEntity>(
    const OpExtInst &opc) {
  auto *op = module.create<DebugImportedEntity>(opc);
  uint32_t line = op->Line();

  auto entity_or_error = translateDebugInst<llvm::DINode>(op->Entity());
  if (auto err = entity_or_error.takeError()) {
    return err;
  }
  llvm::DINode *const entity = entity_or_error.get();

  llvm::DIFile *file = nullptr;
  if (op->Source()) {
    auto file_or_error = translateDebugInst<llvm::DIFile>(op->Source());
    if (auto err = file_or_error.takeError()) {
      return err;
    }
    file = file_or_error.get();
  }
  // If we haven't a file, we can't have a non-zero line number. LLVM asserts on
  // this.
  if (!file) {
    line = 0;
  }

  llvm::DIScope *scope = nullptr;
  if (op->Scope()) {
    auto scope_or_error = translateDebugInst<llvm::DIScope>(op->Scope());
    if (auto err = scope_or_error.takeError()) {
      return err;
    }
    scope = scope_or_error.get();
  }

  llvm::DIBuilder &dib = getDIBuilder(op);

  if (op->Tag() == OpenCLDebugInfo100ImportedModule) {
    if (!entity) {
      dib.createImportedModule(
          scope, static_cast<llvm::DIImportedEntity *>(nullptr), file, line);
      return llvm::Error::success();
    }
    if (auto *const di_module = llvm::dyn_cast<llvm::DIModule>(entity)) {
      dib.createImportedModule(scope, di_module, file, line);
      return llvm::Error::success();
    }
    if (auto *const di_ie = llvm::dyn_cast<llvm::DIImportedEntity>(entity)) {
      dib.createImportedModule(scope, di_ie, file, line);
      return llvm::Error::success();
    }
    if (auto *const di_namespace = llvm::dyn_cast<llvm::DINamespace>(entity)) {
      dib.createImportedModule(scope, di_namespace, file, line);
      return llvm::Error::success();
    }
    return makeStringError("Unhandled imported module");
  }

  if (op->Tag() == OpenCLDebugInfo100ImportedDeclaration) {
    std::optional<std::string> name = module.getDebugString(op->Name());
    if (!name) {
      return makeStringError(
          "Could not find OpString 'Name' for DebugImportedEntity " +
          getIDAsStr(op->IdResult(), &module));
    }
    if (auto *const di_glob =
            llvm::dyn_cast_if_present<llvm::DIGlobalVariableExpression>(
                entity)) {
      dib.createImportedDeclaration(scope, di_glob->getVariable(), file, line,
                                    *name);
    } else {
      dib.createImportedDeclaration(scope, entity, file, line, *name);
    }
    return llvm::Error::success();
  }

  return makeStringError("Unexpected imported entity kind");
}

llvm::Expected<llvm::MDNode *>
DebugInfoBuilder::translateTemplateTemplateParameterOrTemplateParameterPack(
    const OpExtInst *op) {
  // Try and infer whether this is a DebugTypeTemplateTemplateParameter
  // or a DebugTypeTemplateParameterPack. We have to be careful while doing
  // this.

  // Firstly, only ParameterPacks can legally have more than 5 operands,
  // through their variadic 'TemplateParameters' operands.
  if (op->opExtInstOperandCount() > 5) {
    return translate(cast<DebugTypeTemplateParameterPack>(op));
  }
  // If a DebugTypeTemplateParameterPack has one template parameter, it has
  // 10 operands - the same as DebugTypeTemplateTemplateParameter - so we
  // must look harder.
  // The second operand is either:
  // * <id> Source -> DebugSource (DebugTypeTemplateParameterPack)
  // * <id> TemplateName -> OpString -> (DebugTypeTemplateTemplateParameter)
  // Since both must be of ID type, there shouldn't be any potential
  // confusion about whether it's an ID or a literal number, as we'd find
  // if we were to try and intuit the tenth operand:
  // * <id> Parameter (DebugTypeTemplateParameterPack)
  // * Literal Number Column (DebugTypeTemplateTemplateParameter)
  const spv::Id op2_id = op->getOpExtInstOperand(1);

  // Check for DebugSource. If we find one, it's (almost) definitely a
  // DebugTypeTemplateParameterPack, or an invalid binary.
  static const std::unordered_set<ExtendedInstrSet> debug_info_opcodes = {
      ExtendedInstrSet::DebugInfo, ExtendedInstrSet::OpenCLDebugInfo100};

  if (module.isOpExtInst(op2_id, OpenCLDebugInfo100DebugSource,
                         debug_info_opcodes)) {
    return translate(cast<DebugTypeTemplateParameterPack>(op));
  }

  // Check for OpString. If we find one, it's (almost) definitely a
  // DebugTypeTemplateTemplateParameter, or an invalid binary.
  if (const auto second_op_str = module.getDebugString(op2_id)) {
    return translate(cast<DebugTypeTemplateTemplateParameter>(op));
  }

  const bool couldOp2BeDebugSource = module.isOpExtInst(
      op2_id, {OpenCLDebugInfo100DebugInfoNone, OpenCLDebugInfo100DebugSource},
      debug_info_opcodes);
  const bool couldOp5BeDebugTypeTemplateParameter =
      module.isOpExtInst(op->getOpExtInstOperand(4),
                         {OpenCLDebugInfo100DebugInfoNone,
                          OpenCLDebugInfo100DebugTypeTemplateParameter},
                         debug_info_opcodes);

  // If the 2nd operand is a DebugSource and the 5th is a
  // DebugTypeTemplateParameter, it's very likely a
  // DebugTypeTemplateParameterPack.
  const bool couldBeDebugTypeTemplateParameterPack =
      couldOp2BeDebugSource && couldOp5BeDebugTypeTemplateParameter;

  const bool couldOp2BeOptimizedOutTemplateName = isDebugInfoNone(op2_id);
  const bool couldOp3BeDebugSource = module.isOpExtInst(
      op->getOpExtInstOperand(2),
      {OpenCLDebugInfo100DebugInfoNone, OpenCLDebugInfo100DebugSource},
      debug_info_opcodes);

  // If the 2nd operand is a DebugInfoNone (we know it's not an OpString) and
  // the 3rd is a DebugSource, it's very likely a
  // DebugTypeTemplateTemplateParameter.
  const bool couldBeDebugTypeTemplateTemplateParameter =
      couldOp2BeOptimizedOutTemplateName && couldOp3BeDebugSource;

  // If only one opcode is likely, choose to translate as that one.
  if (couldBeDebugTypeTemplateParameterPack &&
      !couldBeDebugTypeTemplateTemplateParameter) {
    return translate(cast<DebugTypeTemplateParameterPack>(op));
  }

  if (!couldBeDebugTypeTemplateParameterPack &&
      couldBeDebugTypeTemplateTemplateParameter) {
    return translate(cast<DebugTypeTemplateTemplateParameter>(op));
  }

  // If both opcodes are still possible, or neither are possible, give up and
  // conservatively return nullptr.
  return nullptr;
}

llvm::Expected<llvm::MDNode *> DebugInfoBuilder::translateDebugInstImpl(
    const OpExtInst *op) {
  assert(isDebugInfoSet(op->Set()) && "Unexpected extended instruction set");

#define TRANSLATE_CASE(Opcode, DebugInst) \
  case Opcode:                            \
    return translate(cast<DebugInst>(op));

  switch (op->Instruction()) {
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeBasic, DebugTypeBasic)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypePointer, DebugTypePointer)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeQualifier, DebugTypeQualifier)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeArray, DebugTypeArray)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeVector, DebugTypeVector)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugSource, DebugSource)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypedef, DebugTypedef)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeFunction, DebugTypeFunction)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeEnum, DebugTypeEnum)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugGlobalVariable, DebugGlobalVariable)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugFunctionDeclaration,
                   DebugFunctionDeclaration)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugFunction, DebugFunction)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugCompilationUnit, DebugCompilationUnit)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugLexicalBlock, DebugLexicalBlock)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugLexicalBlockDiscriminator,
                   DebugLexicalBlockDiscriminator)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugInlinedAt, DebugInlinedAt)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugLocalVariable, DebugLocalVariable)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugExpression, DebugExpression)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeComposite, DebugTypeComposite)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeMember, DebugTypeMember)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeInheritance, DebugTypeInheritance)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypePtrToMember, DebugTypePtrToMember)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeTemplateParameter,
                   DebugTypeTemplateParameter)
    TRANSLATE_CASE(OpenCLDebugInfo100DebugTypeTemplate, DebugTypeTemplate)
    case OpenCLDebugInfo100DebugInfoNone:
      // DebugInfoNone is translated to 'nullptr'. All consumers have to
      // accommodate this as a valid value; various LLVM APIs accept nullptr as
      // a valid value, others will assert on null values.
      return nullptr;
    case OpenCLDebugInfo100DebugInlinedVariable:
    case OpenCLDebugInfo100DebugMacroDef:
    case OpenCLDebugInfo100DebugMacroUndef:
      // Note: LLVM has no meaningful translation for DebugInlinedVariable,
      // DebugMacroDef, or DebugMacroUndef.
      return nullptr;
    case OpenCLDebugInfo100DebugTypeTemplateTemplateParameter:
      if (workarounds & Workarounds::TemplateTemplateSwappedWithParameterPack) {
        return translateTemplateTemplateParameterOrTemplateParameterPack(op);
      }
      return translate(cast<DebugTypeTemplateTemplateParameter>(op));
    case OpenCLDebugInfo100DebugTypeTemplateParameterPack: {
      if (workarounds & Workarounds::TemplateTemplateSwappedWithParameterPack) {
        return translateTemplateTemplateParameterOrTemplateParameterPack(op);
      }
      return translate(cast<DebugTypeTemplateParameterPack>(op));
    }
    default:
      break;
  }

#undef TRANSLATE_CASE

  const spirv_ll::ExtendedInstrSet set = module.getExtendedInstrSet(op->Set());

  return makeStringError(llvm::formatv(
      "Couldn't convert {0} instruction %{1} with opcode {2}",
      set == ExtendedInstrSet::DebugInfo ? "DebugInfo" : "OpenCL.DebugInfo.100",
      op->IdResult(), op->Instruction()));
}

// 'Creates' a DebugInfo instruction. We limit this behaviour to instructions
// which act as root nodes for other DebugInfo instructions:
//
// 1. DebugValue, DebugDeclare, DebugScope & DebugNoScope, which interleave
// with other instructions inside a basic block.
// 2. DebugImportedEntity & DebugTypeTemplate which may be leaves and aren't
// referenced by any other nodes.
// 3. DebugCompilationUnit, to create DIBuilders on the fly.
// 4. DebugFunction, to register and create debug functions before we visit the
// OpFunction that they reference.
//
// All other nodes are visited through the process of creating these above
// nodes. They are visited through the 'translateDebugInst' API, and are
// cached as they may be multiply referenced.
llvm::Error DebugInfoBuilder::create(const OpExtInst &opc) {
  // Most of this code *should* work for the DebugInfo instruction set, with a
  // few tweaks to account for the differences. However, we haven't thoroughly
  // tested that instruction set as there is a dearth of producers and test
  // cases.
  // Until such a time as we can test and update this builder, we
  // conversatively only handle the OpenCL.DebugInfo.100 set.
  if (module.getExtendedInstrSet(opc.Set()) !=
      ExtendedInstrSet::OpenCLDebugInfo100) {
    return llvm::Error::success();
  }

#define CREATE_CASE(Opcode, ExtInst)           \
  case OpenCLDebugInfo100Instructions::Opcode: \
    return create<ExtInst>(opc);

  switch (opc.Instruction()) {
    CREATE_CASE(OpenCLDebugInfo100DebugValue, DebugValue)
    CREATE_CASE(OpenCLDebugInfo100DebugDeclare, DebugDeclare)
    CREATE_CASE(OpenCLDebugInfo100DebugScope, DebugScope)
    CREATE_CASE(OpenCLDebugInfo100DebugNoScope, DebugNoScope)
    CREATE_CASE(OpenCLDebugInfo100DebugImportedEntity, DebugImportedEntity)
    case OpenCLDebugInfo100Instructions::OpenCLDebugInfo100DebugCompilationUnit:
      debug_builder_map[opc.IdResult()] =
          std::make_unique<llvm::DIBuilder>(*module.llvmModule);
      break;
    case OpenCLDebugInfo100Instructions::OpenCLDebugInfo100DebugFunction: {
      // Translate and register the DISubprogram for the function.
      auto subprogram_or_error =
          translateDebugInst<llvm::DISubprogram>(opc.IdResult());
      if (auto err = subprogram_or_error.takeError()) {
        return err;
      }
      llvm::DISubprogram *const subprogram = subprogram_or_error.get();
      if (!subprogram) {
        return llvm::Error::success();
      }
      module.addDebugFunctionScope(cast<DebugFunction>(&opc)->Function(),
                                   subprogram);
      break;
    }
    case OpenCLDebugInfo100Instructions::OpenCLDebugInfo100DebugTypeTemplate:
      // These describe an instantiated template of class, struct, or function
      // in C++. These are not necessarily referenced by other nodes, so we
      // handle them in 'create'.
      // Unfortunately, despite the specification saying that forward
      // references are not allowed in general, we have seen that in real-world
      // SPIR-V binaries that nodes can in fact forward-reference such
      // 'dangling' DebugTypeTemplate instructions, e.g.,
      //     11 ExtInst 15 3634 2 DebugTypeQualifier 3589 0
      //     7 ExtInst 15 3589 2 DebugTypeTemplate 3643 3644
      // As such, we collect all DebugTypeTemplate nodes and process them at
      // the very end. If any are referenced by other nodes in the mean time
      // we'll process them, but if those are forward referenced, we'll crash.
      template_types.push_back(opc.IdResult());
      break;
    default:
      break;
  }

#undef CREATE_CASE
  return llvm::Error::success();
}

llvm::DIBuilder &DebugInfoBuilder::getDIBuilder(const OpExtInst *op) const {
  assert(debug_builder_map.size() != 0 && "No DIBuilders");
  llvm::DIBuilder &default_dib = getDefaultDIBuilder();

  assert(isDebugInfoSet(op->Set()) && "Unexpected extended instruction set");

  const auto getScopeIdOpIdx =
      [&](const OpExtInst *op) -> std::optional<size_t> {
    assert(isDebugInfoSet(op->Set()) && "Unexpected extended instruction set");
    switch (op->Instruction()) {
      default:
        return std::nullopt;
      case OpenCLDebugInfo100DebugTypedef:
        return DebugTypedef::ScopeIdx;
      case OpenCLDebugInfo100DebugTypeEnum:
        return DebugTypeEnum::ScopeIdx;
      case OpenCLDebugInfo100DebugTypeComposite:
        return DebugTypeComposite::ScopeIdx;
      case OpenCLDebugInfo100DebugTypeInheritance:
        return DebugTypeInheritance::ParentIdx;
      case OpenCLDebugInfo100DebugTypePtrToMember:
        return DebugTypePtrToMember::ParentIdx;
      case OpenCLDebugInfo100DebugFunction:
        return DebugFunction::ScopeIdx;
      case OpenCLDebugInfo100DebugLexicalBlock:
        return DebugLexicalBlock::ScopeIdx;
      case OpenCLDebugInfo100DebugLexicalBlockDiscriminator:
        return DebugLexicalBlockDiscriminator::ScopeIdx;
      case OpenCLDebugInfo100DebugScope:
        return DebugScope::ScopeIdx;
      case OpenCLDebugInfo100DebugInlinedAt:
        return DebugInlinedAt::ScopeIdx;
      case OpenCLDebugInfo100DebugLocalVariable:
        return DebugLocalVariable::ScopeIdx;
      case OpenCLDebugInfo100DebugImportedEntity:
        return cast<DebugImportedEntity>(op)->getScopeIdx();
    }
    return std::nullopt;
  };

  // Look up the chain of scopes until we find a registered compilation unit.
  // Note that we assume that a DebugInfo instruction within the set we're
  // interested in only refers to scopes within the same set.
  while (op->Instruction() != OpenCLDebugInfo100DebugCompilationUnit) {
    // It doesn't matter what we do with 'none' - we can bail here.
    if (op->Instruction() == OpenCLDebugInfo100DebugInfoNone) {
      return default_dib;
    }

    // Try to move up the scope chain.
    if (auto scope_id_idx = getScopeIdOpIdx(op)) {
      const spv::Id scope_id = op->getOpExtInstOperand(*scope_id_idx);
      if (auto *scope_op = module.get_or_null(scope_id)) {
        if (auto *scope_ext_op = dyn_cast<OpExtInst>(scope_op)) {
          op = scope_ext_op;
          continue;
        }
      }
    }

    // If we couldn't infer the scope, bail out and use the default DIBuilder
    break;
  }

  return default_dib;
}

llvm::Error DebugInfoBuilder::finishModuleProcessing() {
  if (auto err = finalizeCompositeTypes()) {
    return err;
  }

  // Forcibly translate all DebugTypeTemplate instructions. They may be
  // dangling and not referenced from any 'root' node. However, as noted above
  // where they are collected, we process them at finalization because some
  // SPIR-V binaries forward-reference these nodes illegally.
  for (auto id : template_types) {
    if (auto err = translateDebugInst(id).takeError()) {
      return err;
    }
  }

  // Finalize all of our DIBuilder instances
  for (auto &[_, di_builder] : debug_builder_map) {
    di_builder->finalize();
  }

  return llvm::Error::success();
}

llvm::Error DebugInfoBuilder::finalizeCompositeTypes() {
  // Note; this list might grow as we iterate over it (if members themselves
  // reference hereto unvisited DebugTypeComposite instructions).
  for (size_t i = 0; i != composite_types.size(); i++) {
    const spv::Id id = composite_types[i];
    assert(debug_info_cache.find(id) != debug_info_cache.end());
    auto *composite_type =
        llvm::cast<llvm::DICompositeType>(debug_info_cache[id]);

    // Grab the DebugTypeComposite for this ID.
    const DebugTypeComposite *op = module.get<DebugTypeComposite>(id);

    llvm::SmallVector<llvm::Metadata *, 8> element_tys;
    for (const spv::Id member_id : op->Members()) {
      auto member_or_error = translateDebugInst(member_id);
      if (auto err = member_or_error.takeError()) {
        return err;
      }
      element_tys.emplace_back(member_or_error.get());
    }
    llvm::DIBuilder &dib = getDIBuilder(op);
    const llvm::DINodeArray elements = dib.getOrCreateArray(element_tys);
    dib.replaceArrays(composite_type, elements);
  }
  return llvm::Error::success();
}

}  // namespace spirv_ll
