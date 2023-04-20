// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/creation_apis_helper.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/opaque_pointers.h>
#include <multi_llvm/optional_helper.h>
#include <multi_llvm/vector_type_helper.h>

#include <cmath>
#include <set>

// For compatibility with the Android NDK, we need to use the C ilogb function.
namespace stdcompat {
#ifdef __ANDROID__
// Note: This function accepts double only as its argument
using ::ilogb;
#else
using std::ilogb;
#endif  // __ANDROID__
}  // namespace stdcompat

namespace {
/// @brief Identifiers for recognized OpenCL builtins.
enum CLBuiltinID : compiler::utils::BuiltinID {
  // Non-standard Builtin Functions
  /// @brief Internal builtin 'convert_half_to_float'.
  eCLBuiltinConvertHalfToFloat = compiler::utils::eFirstTargetBuiltin,
  /// @brief Internal builtin 'convert_float_to_half'.
  eCLBuiltinConvertFloatToHalf,
  /// @brief Internal builtin 'convert_float_to_half_rte'
  eCLBuiltinConvertFloatToHalfRte,
  /// @brief Internal builtin 'convert_float_to_half_rtz'
  eCLBuiltinConvertFloatToHalfRtz,
  /// @brief Internal builtin 'convert_float_to_half_rtp'
  eCLBuiltinConvertFloatToHalfRtp,
  /// @brief Internal builtin 'convert_float_to_half_rtn'
  eCLBuiltinConvertFloatToHalfRtn,
  /// @brief Internal builtin 'convert_half_to_double'.
  eCLBuiltinConvertHalfToDouble,
  /// @brief Internal builtin 'convert_double_to_half'.
  eCLBuiltinConvertDoubleToHalf,
  /// @brief Internal builtin 'convert_double_to_half_rte'
  eCLBuiltinConvertDoubleToHalfRte,
  /// @brief Internal builtin 'convert_double_to_half_rtz'
  eCLBuiltinConvertDoubleToHalfRtz,
  /// @brief Internal builtin 'convert_double_to_half_rtp'
  eCLBuiltinConvertDoubleToHalfRtp,
  /// @brief Internal builtin 'convert_double_to_half_rtn'
  eCLBuiltinConvertDoubleToHalfRtn,

  // 6.2.3 Explicit Conversions
  /// @brief OpenCL builtin `convert_char`
  eCLBuiltinConvertChar,
  /// @brief OpenCL builtin `convert_short`
  eCLBuiltinConvertShort,
  /// @brief OpenCL builtin `convert_int`
  eCLBuiltinConvertInt,
  /// @brief OpenCL builtin `convert_long`
  eCLBuiltinConvertLong,
  /// @brief OpenCL builtin `convert_uchar`
  eCLBuiltinConvertUChar,
  /// @brief OpenCL builtin `convert_ushort`
  eCLBuiltinConvertUShort,
  /// @brief OpenCL builtin `convert_uint`
  eCLBuiltinConvertUInt,
  /// @brief OpenCL builtin `convert_ulong`
  eCLBuiltinConvertULong,

  // 6.12.1 Work-Item Functions
  /// @brief OpenCL builtin 'get_work_dim'.
  eCLBuiltinGetWorkDim,
  /// @brief OpenCL builtin 'get_group_id'.
  eCLBuiltinGetGroupId,
  /// @brief OpenCL builtin 'get_global_size'.
  eCLBuiltinGetGlobalSize,
  /// @brief OpenCL builtin 'get_global_offset'.
  eCLBuiltinGetGlobalOffset,
  /// @brief OpenCL builtin 'get_local_id'.
  eCLBuiltinGetLocalId,
  /// @brief OpenCL builtin 'get_local_size'.
  eCLBuiltinGetLocalSize,
  /// @brief OpenCL builtin 'get_num_groups'.
  eCLBuiltinGetNumGroups,
  /// @brief OpenCL builtin 'get_global_id'.
  eCLBuiltinGetGlobalId,
  /// @brief OpenCL builtin 'get_local_linear_id' (OpenCL >= 2.0).
  eCLBuiltinGetLocalLinearId,
  /// @brief OpenCL builtin 'get_global_linear_id' (OpenCL >= 2.0).
  eCLBuiltinGetGlobalLinearId,
  /// @brief OpenCL builtin 'get_sub_group_local_id' (OpenCL >= 3.0).
  eCLBuiltinGetSubgroupLocalId,

  // 6.12.2 Math Functions
  /// @brief OpenCL builtin 'fmax'.
  eCLBuiltinFMax,
  /// @brief OpenCL builtin 'fmin'.
  eCLBuiltinFMin,
  /// @brief OpenCL builtin 'fract'.
  eCLBuiltinFract,
  /// @brief OpenCL builtin 'frexp'.
  eCLBuiltinFrexp,
  /// @brief OpenCL builtin 'lgamma_r'.
  eCLBuiltinLGammaR,
  /// @brief OpenCL builtin 'modf'.
  eCLBuiltinModF,
  /// @brief OpenCL builtin 'sincos'.
  eCLBuiltinSinCos,
  /// @brief OpenCL builtin 'remquo'.
  eCLBuiltinRemquo,

  // 6.12.3 Integer Functions
  /// @brief OpenCL builtin 'add_sat'.
  eCLBuiltinAddSat,
  /// @brief OpenCL builtin 'sub_sat'.
  eCLBuiltinSubSat,

  // 6.12.5 Geometric Builtin-in Functions
  /// @brief OpenCL builtin 'dot'.
  eCLBuiltinDot,
  /// @brief OpenCL builtin 'cross'.
  eCLBuiltinCross,
  /// @brief OpenCL builtin 'length'.
  eCLBuiltinLength,
  /// @brief OpenCL builtin 'distance'.
  eCLBuiltinDistance,
  /// @brief OpenCL builtin 'normalize'.
  eCLBuiltinNormalize,
  /// @brief OpenCL builtin 'fast_length'.
  eCLBuiltinFastLength,
  /// @brief OpenCL builtin 'fast_distance'.
  eCLBuiltinFastDistance,
  /// @brief OpenCL builtin 'fast_normalize'.
  eCLBuiltinFastNormalize,

  // 6.12.6 Relational Functions
  /// @brief OpenCL builtin 'all'.
  eCLBuiltinAll,
  /// @brief OpenCL builtin 'any'.
  eCLBuiltinAny,
  /// @brief OpenCL builtin 'isequal'.
  eCLBuiltinIsEqual,
  /// @brief OpenCL builtin 'isnotequal'.
  eCLBuiltinIsNotEqual,
  /// @brief OpenCL builtin 'isgreater'.
  eCLBuiltinIsGreater,
  /// @brief OpenCL builtin 'isgreaterequal'.
  eCLBuiltinIsGreaterEqual,
  /// @brief OpenCL builtin 'isless'.
  eCLBuiltinIsLess,
  /// @brief OpenCL builtin 'islessequal'.
  eCLBuiltinIsLessEqual,
  /// @brief OpenCL builtin 'islessgreater'.
  eCLBuiltinIsLessGreater,
  /// @brief OpenCL builtin 'isordered'.
  eCLBuiltinIsOrdered,
  /// @brief OpenCL builtin 'isunordered'.
  eCLBuiltinIsUnordered,
  /// @brief OpenCL builtin 'isfinite'.
  eCLBuiltinIsFinite,
  /// @brief OpenCL builtin 'isinf'.
  eCLBuiltinIsInf,
  /// @brief OpenCL builtin 'isnan'.
  eCLBuiltinIsNan,
  /// @brief OpenCL builtin 'isnormal'.
  eCLBuiltinIsNormal,
  /// @brief OpenCL builtin 'signbit'.
  eCLBuiltinSignBit,
  /// @brief OpenCL builtin `select`.
  eCLBuiltinSelect,

  // 6.12.8 Synchronization Functions
  /// @brief OpenCL builtin 'barrier'.
  eCLBuiltinBarrier,
  /// @brief OpenCL builtin 'mem_fence'.
  eCLBuiltinMemFence,
  /// @brief OpenCL builtin 'read_mem_fence'.
  eCLBuiltinReadMemFence,
  /// @brief OpenCL builtin 'write_mem_fence'.
  eCLBuiltinWriteMemFence,
  /// @brief OpenCL builtin 'atomic_work_item_fence'.
  eCLBuiltinAtomicWorkItemFence,
  /// @brief OpenCL builtin 'sub_group_barrier'.
  eCLBuiltinSubGroupBarrier,
  /// @brief OpenCL builtin 'work_group_barrier'.
  eCLBuiltinWorkGroupBarrier,

  // 6.12.10 Async Copies and Prefetch Functions
  /// @brief OpenCL builtin 'async_work_group_copy'.
  eCLBuiltinAsyncWorkGroupCopy,
  /// @brief OpenCL builtin 'async_work_group_strided_copy'.
  eCLBuiltinAsyncWorkGroupStridedCopy,
  /// @brief OpenCL builtin 'wait_group_events'.
  eCLBuiltinWaitGroupEvents,

  // 6.12.11 Atomic Functions
  /// @brief OpenCL builtins 'atomic_add', 'atom_add'.
  eCLBuiltinAtomicAdd,
  /// @brief OpenCL builtins 'atomic_sub', 'atom_sub'.
  eCLBuiltinAtomicSub,
  /// @brief OpenCL builtins 'atomic_xchg', 'atom_xchg'.
  eCLBuiltinAtomicXchg,
  /// @brief OpenCL builtins 'atomic_inc', 'atom_inc'.
  eCLBuiltinAtomicInc,
  /// @brief OpenCL builtins 'atomic_dec', 'atom_dec'.
  eCLBuiltinAtomicDec,
  /// @brief OpenCL builtins 'atomic_cmpxchg', 'atom_cmpxchg'.
  eCLBuiltinAtomicCmpxchg,
  /// @brief OpenCL builtins 'atomic_min', 'atom_min'.
  eCLBuiltinAtomicMin,
  /// @brief OpenCL builtins 'atomic_max', 'atom_max'.
  eCLBuiltinAtomicMax,
  /// @brief OpenCL builtins 'atomic_and', 'atom_and'.
  eCLBuiltinAtomicAnd,
  /// @brief OpenCL builtins 'atomic_or', 'atom_or'.
  eCLBuiltinAtomicOr,
  /// @brief OpenCL builtins 'atomic_xor', 'atom_xor'.
  eCLBuiltinAtomicXor,

  // 6.12.12 Miscellaneous Vector Functions
  eCLBuiltinShuffle,
  eCLBuiltinShuffle2,

  // 6.12.13 printf
  /// @brief OpenCL builtin 'printf'.
  eCLBuiltinPrintf,

  // 6.15.16 Work-group Collective Functions
  /// @brief OpenCL builtin 'work_group_all'.
  eCLBuiltinWorkgroupAll,
  /// @brief OpenCL builtin 'work_group_any'.
  eCLBuiltinWorkgroupAny,
  /// @brief OpenCL builtin 'work_group_broadcast'.
  eCLBuiltinWorkgroupBroadcast,
  /// @brief OpenCL builtin 'work_group_reduce_add'.
  eCLBuiltinWorkgroupReduceAdd,
  /// @brief OpenCL builtin 'work_group_reduce_min'.
  eCLBuiltinWorkgroupReduceMin,
  /// @brief OpenCL builtin 'work_group_reduce_max'.
  eCLBuiltinWorkgroupReduceMax,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_add'.
  eCLBuiltinWorkgroupScanAddInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_add'.
  eCLBuiltinWorkgroupScanAddExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_min'.
  eCLBuiltinWorkgroupScanMinInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_min'.
  eCLBuiltinWorkgroupScanMinExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_max'.
  eCLBuiltinWorkgroupScanMaxInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_max'.
  eCLBuiltinWorkgroupScanMaxExclusive,

  /// @brief OpenCL builtin 'work_group_reduce_mul'.
  eCLBuiltinWorkgroupReduceMul,
  /// @brief OpenCL builtin 'work_group_reduce_and'.
  eCLBuiltinWorkgroupReduceAnd,
  /// @brief OpenCL builtin 'work_group_reduce_or'.
  eCLBuiltinWorkgroupReduceOr,
  /// @brief OpenCL builtin 'work_group_reduce_xor'.
  eCLBuiltinWorkgroupReduceXor,
  /// @brief OpenCL builtin 'work_group_reduce_logical_and'.
  eCLBuiltinWorkgroupReduceLogicalAnd,
  /// @brief OpenCL builtin 'work_group_reduce_logical_or'.
  eCLBuiltinWorkgroupReduceLogicalOr,
  /// @brief OpenCL builtin 'work_group_reduce_logical_xor'.
  eCLBuiltinWorkgroupReduceLogicalXor,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_mul'.
  eCLBuiltinWorkgroupScanMulInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_mul'.
  eCLBuiltinWorkgroupScanMulExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_and'.
  eCLBuiltinWorkgroupScanAndInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_and'.
  eCLBuiltinWorkgroupScanAndExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_or'.
  eCLBuiltinWorkgroupScanOrInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_or'.
  eCLBuiltinWorkgroupScanOrExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_xor'.
  eCLBuiltinWorkgroupScanXorInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_xor'.
  eCLBuiltinWorkgroupScanXorExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_logical_and'.
  eCLBuiltinWorkgroupScanLogicalAndInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_logical_and'.
  eCLBuiltinWorkgroupScanLogicalAndExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_logical_or'.
  eCLBuiltinWorkgroupScanLogicalOrInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_logical_or'.
  eCLBuiltinWorkgroupScanLogicalOrExclusive,
  /// @brief OpenCL builtin 'work_group_scan_inclusive_logical_xor'.
  eCLBuiltinWorkgroupScanLogicalXorInclusive,
  /// @brief OpenCL builtin 'work_group_scan_exclusive_logical_xor'.
  eCLBuiltinWorkgroupScanLogicalXorExclusive,

  // 6.15.19 Subgroup Collective Functions
  /// @brief OpenCL builtin 'sub_group_all'.
  eCLBuiltinSubgroupAll,
  /// @brief OpenCL builtin 'sub_group_any'.
  eCLBuiltinSubgroupAny,
  /// @brief OpenCL builtin 'sub_group_broadcast'.
  eCLBuiltinSubgroupBroadcast,
  /// @brief OpenCL builtin 'sub_group_reduce_add'.
  eCLBuiltinSubgroupReduceAdd,
  /// @brief OpenCL builtin 'sub_group_reduce_min'.
  eCLBuiltinSubgroupReduceMin,
  /// @brief OpenCL builtin 'sub_group_reduce_max'.
  eCLBuiltinSubgroupReduceMax,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_add'.
  eCLBuiltinSubgroupScanAddInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_add'.
  eCLBuiltinSubgroupScanAddExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_min'.
  eCLBuiltinSubgroupScanMinInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_min'.
  eCLBuiltinSubgroupScanMinExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_max'.
  eCLBuiltinSubgroupScanMaxInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_max'.
  eCLBuiltinSubgroupScanMaxExclusive,

  /// @brief OpenCL builtin 'sub_group_reduce_mul'.
  eCLBuiltinSubgroupReduceMul,
  /// @brief OpenCL builtin 'sub_group_reduce_and'.
  eCLBuiltinSubgroupReduceAnd,
  /// @brief OpenCL builtin 'sub_group_reduce_or'.
  eCLBuiltinSubgroupReduceOr,
  /// @brief OpenCL builtin 'sub_group_reduce_xor'.
  eCLBuiltinSubgroupReduceXor,
  /// @brief OpenCL builtin 'sub_group_reduce_logical_and'.
  eCLBuiltinSubgroupReduceLogicalAnd,
  /// @brief OpenCL builtin 'sub_group_reduce_logical_or'.
  eCLBuiltinSubgroupReduceLogicalOr,
  /// @brief OpenCL builtin 'sub_group_reduce_logical_xor'.
  eCLBuiltinSubgroupReduceLogicalXor,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_mul'.
  eCLBuiltinSubgroupScanMulInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_mul'.
  eCLBuiltinSubgroupScanMulExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_and'.
  eCLBuiltinSubgroupScanAndInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_and'.
  eCLBuiltinSubgroupScanAndExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_or'.
  eCLBuiltinSubgroupScanOrInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_or'.
  eCLBuiltinSubgroupScanOrExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_xor'.
  eCLBuiltinSubgroupScanXorInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_xor'.
  eCLBuiltinSubgroupScanXorExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_logical_and'.
  eCLBuiltinSubgroupScanLogicalAndInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_logical_and'.
  eCLBuiltinSubgroupScanLogicalAndExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_logical_or'.
  eCLBuiltinSubgroupScanLogicalOrInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_logical_or'.
  eCLBuiltinSubgroupScanLogicalOrExclusive,
  /// @brief OpenCL builtin 'sub_group_scan_inclusive_logical_xor'.
  eCLBuiltinSubgroupScanLogicalXorInclusive,
  /// @brief OpenCL builtin 'sub_group_scan_exclusive_logical_xor'.
  eCLBuiltinSubgroupScanLogicalXorExclusive,

  // GLSL builtin functions
  eCLBuiltinCodeplayFindLSB,
  eCLBuiltinCodeplayFindMSB,
  eCLBuiltinCodeplayBitReverse,
  eCLBuiltinCodeplayFaceForward,
  eCLBuiltinCodeplayReflect,
  eCLBuiltinCodeplayRefract,
  eCLBuiltinCodeplayPackNormalizeChar4,
  eCLBuiltinCodeplayPackNormalizeUchar4,
  eCLBuiltinCodeplayPackNormalizeShort2,
  eCLBuiltinCodeplayPackNormalizeUshort2,
  eCLBuiltinCodeplayPackHalf2,
  eCLBuiltinCodeplayUnpackNormalize,
  eCLBuiltinCodeplayUnpackHalf2,

  // 6.12.7 Vector Data Load and Store Functions
  eCLBuiltinVLoad,
  eCLBuiltinVLoadHalf,
  eCLBuiltinVStore,
  eCLBuiltinVStoreHalf,

  // 6.3 Conversions & Type Casting Examples
  eCLBuiltinAs,
};
}  // namespace

namespace {
using namespace llvm;
using namespace compiler::utils;

// Returns whether the given integer is a valid vector width in OpenCL.
// Matches 2, 3, 4, 8, 16.
bool isValidVecWidth(unsigned w) {
  return (w == 3 || (w >= 2 && w <= 16 && llvm::isPowerOf2_32(w)));
}

/// @brief Copy global variables to a module on demand.
class GlobalValueMaterializer final : public llvm::ValueMaterializer {
 public:
  /// @brief Create a new global variable materializer.
  /// @param[in] M Module to materialize the variables in.
  GlobalValueMaterializer(Module &M) : DestM(M) {}

  /// @brief List of variables created during materialization.
  const std::vector<GlobalVariable *> &variables() const { return Variables; }

  /// @brief Materialize the given value.
  ///
  /// @param[in] V Value to materialize.
  ///
  /// @return A value that lives in the destination module, or nullptr if the
  /// given value could not be materialized (e.g. it is not a global variable).
  Value *materialize(Value *V) override final {
    GlobalVariable *GV = dyn_cast<GlobalVariable>(V);
    if (!GV) {
      return nullptr;
    }
    GlobalVariable *NewGV = DestM.getGlobalVariable(GV->getName());
    if (!NewGV) {
      NewGV = new GlobalVariable(
          DestM, GV->getValueType(), GV->isConstant(), GV->getLinkage(),
          (Constant *)nullptr, GV->getName(), (GlobalVariable *)nullptr,
          GV->getThreadLocalMode(), GV->getType()->getAddressSpace());
      NewGV->copyAttributesFrom(GV);
      Variables.push_back(GV);
    }
    return NewGV;
  }

 private:
  /// @brief Modules to materialize variables in.
  Module &DestM;
  /// @brief Materialized variables.
  std::vector<GlobalVariable *> Variables;
};
}  // namespace

namespace compiler {
namespace utils {
using namespace llvm;

std::unique_ptr<BILangInfoConcept> createCLBuiltinInfo(Module *Builtins) {
  return std::make_unique<CLBuiltinInfo>(Builtins);
}

CLBuiltinInfo::CLBuiltinInfo(Module *builtins)
    : Loader(std::make_unique<SimpleCLBuiltinLoader>(builtins)) {}

CLBuiltinInfo::~CLBuiltinInfo() = default;

/// @brief Create a call instruction to the given builtin and set the correct
/// calling convention.
///
/// This function is intended as a helper function for creating calls to
/// builtins. For each call generated we need to set the calling convention
/// manually, which can lead to code bloat. This function will create the call
/// instruction and then it will either copy the calling convention for the
/// called function (if possible) or set it to the default value of spir_func.
///
/// @param[in] B The IRBuilder to use when creating the CallInst
/// @param[in] Builtin The Function to call
/// @param[in] Args The call arguments
/// @param[in] NameStr The name for the new CallInst
/// @return The newly emitted CallInst
static CallInst *CreateBuiltinCall(IRBuilder<> &B, Function *Builtin,
                                   ArrayRef<Value *> Args,
                                   Twine const &NameStr = "") {
  CallInst *CI =
      B.CreateCall(Builtin->getFunctionType(), Builtin, Args, NameStr);
  CI->setCallingConv(Builtin->getCallingConv());
  return CI;
}

struct CLBuiltinEntry {
  /// @brief Identifier for the builtin function.
  BuiltinID ID;
  /// @brief OpenCL name of the builtin function.
  const char *opencl_name;
};

/// @brief Information about known OpenCL builtins.
static const CLBuiltinEntry Builtins[] = {
    // Non-standard Builtin Functions
    {eCLBuiltinConvertHalfToFloat, "convert_half_to_float"},
    {eCLBuiltinConvertFloatToHalf, "convert_float_to_half"},
    {eCLBuiltinConvertFloatToHalfRte, "convert_float_to_half_rte"},
    {eCLBuiltinConvertFloatToHalfRtz, "convert_float_to_half_rtz"},
    {eCLBuiltinConvertFloatToHalfRtp, "convert_float_to_half_rtp"},
    {eCLBuiltinConvertFloatToHalfRtn, "convert_float_to_half_rtn"},
    {eCLBuiltinConvertHalfToDouble, "convert_half_to_double"},
    {eCLBuiltinConvertDoubleToHalf, "convert_double_to_half"},
    {eCLBuiltinConvertDoubleToHalfRte, "convert_double_to_half_rte"},
    {eCLBuiltinConvertDoubleToHalfRtz, "convert_double_to_half_rtz"},
    {eCLBuiltinConvertDoubleToHalfRtp, "convert_double_to_half_rtp"},
    {eCLBuiltinConvertDoubleToHalfRtn, "convert_double_to_half_rtn"},

    // 6.2.3 Explicit Conversions
    {eCLBuiltinConvertChar, "convert_char"},
    {eCLBuiltinConvertShort, "convert_short"},
    {eCLBuiltinConvertInt, "convert_int"},
    {eCLBuiltinConvertLong, "convert_long"},
    {eCLBuiltinConvertUChar, "convert_uchar"},
    {eCLBuiltinConvertUShort, "convert_ushort"},
    {eCLBuiltinConvertUInt, "convert_uint"},
    {eCLBuiltinConvertULong, "convert_ulong"},

    // 6.12.1 Work-Item Functions
    {eCLBuiltinGetWorkDim, "get_work_dim"},
    {eCLBuiltinGetGroupId, "get_group_id"},
    {eCLBuiltinGetGlobalSize, "get_global_size"},
    {eCLBuiltinGetGlobalOffset, "get_global_offset"},
    {eCLBuiltinGetLocalId, "get_local_id"},
    {eCLBuiltinGetLocalSize, "get_local_size"},
    {eCLBuiltinGetNumGroups, "get_num_groups"},
    {eCLBuiltinGetGlobalId, "get_global_id"},
    {eCLBuiltinGetLocalLinearId, "get_local_linear_id"},
    {eCLBuiltinGetGlobalLinearId, "get_global_linear_id"},
    {eCLBuiltinGetSubgroupLocalId, "get_sub_group_local_id"},

    // 6.12.2 Math Functions
    {eCLBuiltinFMax, "fmax"},
    {eCLBuiltinFMin, "fmin"},
    {eCLBuiltinFract, "fract"},
    {eCLBuiltinFrexp, "frexp"},
    {eCLBuiltinLGammaR, "lgamma_r"},
    {eCLBuiltinModF, "modf"},
    {eCLBuiltinSinCos, "sincos"},
    {eCLBuiltinRemquo, "remquo"},

    // 6.12.3 Integer Functions
    {eCLBuiltinAddSat, "add_sat"},
    {eCLBuiltinSubSat, "sub_sat"},

    // 6.12.5 Geometric Functions
    {eCLBuiltinDot, "dot"},
    {eCLBuiltinCross, "cross"},
    {eCLBuiltinLength, "length"},
    {eCLBuiltinDistance, "distance"},
    {eCLBuiltinNormalize, "normalize"},
    {eCLBuiltinFastLength, "fast_length"},
    {eCLBuiltinFastDistance, "fast_distance"},
    {eCLBuiltinFastNormalize, "fast_normalize"},

    // 6.12.6 Relational Functions
    {eCLBuiltinAll, "all"},
    {eCLBuiltinAny, "any"},
    {eCLBuiltinIsEqual, "isequal"},
    {eCLBuiltinIsNotEqual, "isnotequal"},
    {eCLBuiltinIsGreater, "isgreater"},
    {eCLBuiltinIsGreaterEqual, "isgreaterequal"},
    {eCLBuiltinIsLess, "isless"},
    {eCLBuiltinIsLessEqual, "islessequal"},
    {eCLBuiltinIsLessGreater, "islessgreater"},
    {eCLBuiltinIsOrdered, "isordered"},
    {eCLBuiltinIsUnordered, "isunordered"},
    {eCLBuiltinIsFinite, "isfinite"},
    {eCLBuiltinIsInf, "isinf"},
    {eCLBuiltinIsNan, "isnan"},
    {eCLBuiltinIsNormal, "isnormal"},
    {eCLBuiltinSignBit, "signbit"},
    {eCLBuiltinSelect, "select"},

    // 6.12.8 Synchronization Functions
    {eCLBuiltinBarrier, "barrier"},
    {eCLBuiltinMemFence, "mem_fence"},
    {eCLBuiltinReadMemFence, "read_mem_fence"},
    {eCLBuiltinWriteMemFence, "write_mem_fence"},
    {eCLBuiltinAtomicWorkItemFence, "atomic_work_item_fence"},
    {eCLBuiltinSubGroupBarrier, "sub_group_barrier"},
    {eCLBuiltinWorkGroupBarrier, "work_group_barrier"},

    // 6.12.10 Async Copies and Prefetch Functions
    {eCLBuiltinAsyncWorkGroupCopy, "async_work_group_copy"},
    {eCLBuiltinAsyncWorkGroupStridedCopy, "async_work_group_strided_copy"},
    {eCLBuiltinWaitGroupEvents, "wait_group_events"},

    // 6.12.11 Atomic Functions
    {eCLBuiltinAtomicAdd, "atom_add"},
    {eCLBuiltinAtomicSub, "atom_sub"},
    {eCLBuiltinAtomicXchg, "atom_xchg"},
    {eCLBuiltinAtomicInc, "atom_inc"},
    {eCLBuiltinAtomicDec, "atom_dec"},
    {eCLBuiltinAtomicCmpxchg, "atom_cmpxchg"},
    {eCLBuiltinAtomicMin, "atom_min"},
    {eCLBuiltinAtomicMax, "atom_max"},
    {eCLBuiltinAtomicAnd, "atom_and"},
    {eCLBuiltinAtomicOr, "atom_or"},
    {eCLBuiltinAtomicXor, "atom_xor"},
    {eCLBuiltinAtomicAdd, "atomic_add"},
    {eCLBuiltinAtomicSub, "atomic_sub"},
    {eCLBuiltinAtomicXchg, "atomic_xchg"},
    {eCLBuiltinAtomicInc, "atomic_inc"},
    {eCLBuiltinAtomicDec, "atomic_dec"},
    {eCLBuiltinAtomicCmpxchg, "atomic_cmpxchg"},
    {eCLBuiltinAtomicMin, "atomic_min"},
    {eCLBuiltinAtomicMax, "atomic_max"},
    {eCLBuiltinAtomicAnd, "atomic_and"},
    {eCLBuiltinAtomicOr, "atomic_or"},
    {eCLBuiltinAtomicXor, "atomic_xor"},

    // 6.11.12 Miscellaneous Vector Functions
    {eCLBuiltinShuffle, "shuffle"},
    {eCLBuiltinShuffle2, "shuffle2"},

    // 6.12.13 printf
    {eCLBuiltinPrintf, "printf"},

    // 6.15.16 Work-group Collective Functions
    {eCLBuiltinWorkgroupAll, "work_group_all"},
    {eCLBuiltinWorkgroupAny, "work_group_any"},
    {eCLBuiltinWorkgroupBroadcast, "work_group_broadcast"},
    {eCLBuiltinWorkgroupReduceAdd, "work_group_reduce_add"},
    {eCLBuiltinWorkgroupReduceMin, "work_group_reduce_min"},
    {eCLBuiltinWorkgroupReduceMax, "work_group_reduce_max"},
    {eCLBuiltinWorkgroupScanAddInclusive, "work_group_scan_inclusive_add"},
    {eCLBuiltinWorkgroupScanAddExclusive, "work_group_scan_exclusive_add"},
    {eCLBuiltinWorkgroupScanMinInclusive, "work_group_scan_inclusive_min"},
    {eCLBuiltinWorkgroupScanMinExclusive, "work_group_scan_exclusive_min"},
    {eCLBuiltinWorkgroupScanMaxInclusive, "work_group_scan_inclusive_max"},
    {eCLBuiltinWorkgroupScanMaxExclusive, "work_group_scan_exclusive_max"},

    /// Provided by SPV_KHR_uniform_group_instructions.
    {eCLBuiltinWorkgroupReduceMul, "work_group_reduce_mul"},
    {eCLBuiltinWorkgroupReduceAnd, "work_group_reduce_and"},
    {eCLBuiltinWorkgroupReduceOr, "work_group_reduce_or"},
    {eCLBuiltinWorkgroupReduceXor, "work_group_reduce_xor"},
    {eCLBuiltinWorkgroupReduceLogicalAnd, "work_group_reduce_logical_and"},
    {eCLBuiltinWorkgroupReduceLogicalOr, "work_group_reduce_logical_or"},
    {eCLBuiltinWorkgroupReduceLogicalXor, "work_group_reduce_logical_xor"},
    {eCLBuiltinWorkgroupScanMulInclusive, "work_group_scan_inclusive_mul"},
    {eCLBuiltinWorkgroupScanMulExclusive, "work_group_scan_exclusive_mul"},
    {eCLBuiltinWorkgroupScanAndInclusive, "work_group_scan_inclusive_and"},
    {eCLBuiltinWorkgroupScanAndExclusive, "work_group_scan_exclusive_and"},
    {eCLBuiltinWorkgroupScanOrInclusive, "work_group_scan_inclusive_or"},
    {eCLBuiltinWorkgroupScanOrExclusive, "work_group_scan_exclusive_or"},
    {eCLBuiltinWorkgroupScanXorInclusive, "work_group_scan_inclusive_xor"},
    {eCLBuiltinWorkgroupScanXorExclusive, "work_group_scan_exclusive_xor"},
    {eCLBuiltinWorkgroupScanLogicalAndInclusive,
     "work_group_scan_inclusive_logical_and"},
    {eCLBuiltinWorkgroupScanLogicalAndExclusive,
     "work_group_scan_exclusive_logical_and"},
    {eCLBuiltinWorkgroupScanLogicalOrInclusive,
     "work_group_scan_inclusive_logical_or"},
    {eCLBuiltinWorkgroupScanLogicalOrExclusive,
     "work_group_scan_exclusive_logical_or"},
    {eCLBuiltinWorkgroupScanLogicalXorInclusive,
     "work_group_scan_inclusive_logical_xor"},
    {eCLBuiltinWorkgroupScanLogicalXorExclusive,
     "work_group_scan_exclusive_logical_xor"},

    // 6.15.19 Subgroup Collective Functions
    {eCLBuiltinSubgroupAll, "sub_group_all"},
    {eCLBuiltinSubgroupAny, "sub_group_any"},
    {eCLBuiltinSubgroupBroadcast, "sub_group_broadcast"},
    {eCLBuiltinSubgroupReduceAdd, "sub_group_reduce_add"},
    {eCLBuiltinSubgroupReduceMin, "sub_group_reduce_min"},
    {eCLBuiltinSubgroupReduceMax, "sub_group_reduce_max"},
    {eCLBuiltinSubgroupScanAddInclusive, "sub_group_scan_inclusive_add"},
    {eCLBuiltinSubgroupScanAddExclusive, "sub_group_scan_exclusive_add"},
    {eCLBuiltinSubgroupScanMinInclusive, "sub_group_scan_inclusive_min"},
    {eCLBuiltinSubgroupScanMinExclusive, "sub_group_scan_exclusive_min"},
    {eCLBuiltinSubgroupScanMaxInclusive, "sub_group_scan_inclusive_max"},
    {eCLBuiltinSubgroupScanMaxExclusive, "sub_group_scan_exclusive_max"},
    /// Provided by SPV_KHR_uniform_group_instructions.
    {eCLBuiltinSubgroupReduceMul, "sub_group_reduce_mul"},
    {eCLBuiltinSubgroupReduceAnd, "sub_group_reduce_and"},
    {eCLBuiltinSubgroupReduceOr, "sub_group_reduce_or"},
    {eCLBuiltinSubgroupReduceXor, "sub_group_reduce_xor"},
    {eCLBuiltinSubgroupReduceLogicalAnd, "sub_group_reduce_logical_and"},
    {eCLBuiltinSubgroupReduceLogicalOr, "sub_group_reduce_logical_or"},
    {eCLBuiltinSubgroupReduceLogicalXor, "sub_group_reduce_logical_xor"},
    {eCLBuiltinSubgroupScanMulInclusive, "sub_group_scan_inclusive_mul"},
    {eCLBuiltinSubgroupScanMulExclusive, "sub_group_scan_exclusive_mul"},
    {eCLBuiltinSubgroupScanAndInclusive, "sub_group_scan_inclusive_and"},
    {eCLBuiltinSubgroupScanAndExclusive, "sub_group_scan_exclusive_and"},
    {eCLBuiltinSubgroupScanOrInclusive, "sub_group_scan_inclusive_or"},
    {eCLBuiltinSubgroupScanOrExclusive, "sub_group_scan_exclusive_or"},
    {eCLBuiltinSubgroupScanXorInclusive, "sub_group_scan_inclusive_xor"},
    {eCLBuiltinSubgroupScanXorExclusive, "sub_group_scan_exclusive_xor"},
    {eCLBuiltinSubgroupScanLogicalAndInclusive,
     "sub_group_scan_inclusive_logical_and"},
    {eCLBuiltinSubgroupScanLogicalAndExclusive,
     "sub_group_scan_exclusive_logical_and"},
    {eCLBuiltinSubgroupScanLogicalOrInclusive,
     "sub_group_scan_inclusive_logical_or"},
    {eCLBuiltinSubgroupScanLogicalOrExclusive,
     "sub_group_scan_exclusive_logical_or"},
    {eCLBuiltinSubgroupScanLogicalXorInclusive,
     "sub_group_scan_inclusive_logical_xor"},
    {eCLBuiltinSubgroupScanLogicalXorExclusive,
     "sub_group_scan_exclusive_logical_xor"},

    // GLSL builtin functions
    {eCLBuiltinCodeplayFaceForward, "codeplay_face_forward"},
    {eCLBuiltinCodeplayReflect, "codeplay_reflect"},
    {eCLBuiltinCodeplayRefract, "codeplay_refract"},
    {eCLBuiltinCodeplayFindLSB, "codeplay_pack_find_lsb"},
    {eCLBuiltinCodeplayFindMSB, "codeplay_pack_find_msb"},
    {eCLBuiltinCodeplayBitReverse, "codeplay_pack_bit_reverse"},
    {eCLBuiltinCodeplayPackNormalizeChar4, "codeplay_pack_normalize_char4"},
    {eCLBuiltinCodeplayPackNormalizeUchar4, "codeplay_pack_normalize_uchar4"},
    {eCLBuiltinCodeplayPackNormalizeShort2, "codeplay_pack_normalize_short2"},
    {eCLBuiltinCodeplayPackNormalizeUshort2, "codeplay_pack_normalize_ushort2"},
    {eCLBuiltinCodeplayPackHalf2, "codeplay_pack_half2"},
    {eCLBuiltinCodeplayUnpackNormalize, "codeplay_unpack_normalize"},
    {eCLBuiltinCodeplayUnpackHalf2, "codeplay_unpack_half2"},

    {eBuiltinInvalid, nullptr},
    {eBuiltinUnknown, nullptr}};

////////////////////////////////////////////////////////////////////////////////

Function *CLBuiltinInfo::declareBuiltin(Module *M, BuiltinID ID, Type *RetTy,
                                        ArrayRef<Type *> ArgTys,
                                        ArrayRef<TypeQualifiers> ArgQuals,
                                        Twine Suffix) {
  // Determine the builtin function name.
  if (!M) {
    return nullptr;
  }
  std::string BuiltinName = getBuiltinName(ID).str();
  if (BuiltinName.empty()) {
    return nullptr;
  }

  // Add the optional suffix.
  SmallVector<char, 16> SuffixVec;
  Suffix.toVector(SuffixVec);
  if (!SuffixVec.empty()) {
    BuiltinName.append(SuffixVec.begin(), SuffixVec.end());
  }

  // Mangle the function name and look it up in the module.
  NameMangler Mangler(&M->getContext());
  std::string MangledName = Mangler.mangleName(BuiltinName, ArgTys, ArgQuals);
  Function *Builtin = M->getFunction(MangledName);

  // Declare the builtin if necessary.
  if (!Builtin) {
    FunctionType *FT = FunctionType::get(RetTy, ArgTys, false);
    M->getOrInsertFunction(MangledName, FT);
    Builtin = M->getFunction(MangledName);
    Builtin->setCallingConv(CallingConv::SPIR_FUNC);
  }
  return Builtin;
}

BuiltinID CLBuiltinInfo::getPrintfBuiltin() const { return eCLBuiltinPrintf; }

BuiltinID CLBuiltinInfo::getSubgroupLocalIdBuiltin() const {
  return eCLBuiltinGetSubgroupLocalId;
}

BuiltinID CLBuiltinInfo::getSubgroupBroadcastBuiltin() const {
  return eCLBuiltinSubgroupBroadcast;
}

Module *CLBuiltinInfo::getBuiltinsModule() {
  if (!Loader) {
    return nullptr;
  }
  return Loader->getBuiltinsModule();
}

Function *CLBuiltinInfo::materializeBuiltin(StringRef BuiltinName,
                                            Module *DestM,
                                            BuiltinMatFlags Flags) {
  // First try to find the builtin in the target module.
  if (DestM) {
    Function *Builtin = DestM->getFunction(BuiltinName);
    // If a builtin was found, it might be either a declaration or a definition.
    // If the definition flag (eBuiltinMatDefinition) is set, we can not return
    // just a declaration.
    if (Builtin &&
        (!(Flags & eBuiltinMatDefinition) || !Builtin->isDeclaration())) {
      return Builtin;
    }
  }

  if (!Loader) {
    return nullptr;
  }
  // Try to find the builtin in the builtins module
  return Loader->materializeBuiltin(BuiltinName, DestM, Flags);
}

BuiltinID CLBuiltinInfo::identifyBuiltin(Function const &F) const {
  NameMangler Mangler(nullptr);
  StringRef const Name = F.getName();
  const CLBuiltinEntry *entry = Builtins;
  StringRef DemangledName = Mangler.demangleName(Name);
  while (entry->ID != eBuiltinInvalid) {
    if (DemangledName.equals(entry->opencl_name)) {
      return entry->ID;
    }
    entry++;
  }

  if (DemangledName == Name) {
    // The function name is not mangled and so it can not be an OpenCL builtin.
    return eBuiltinInvalid;
  }

  Lexer L(Mangler.demangleName(Name));
  if (L.Consume("vload")) {
    unsigned Width = 0;
    if (L.Consume("_half")) {
      // We have both `vload_half` and `vload_halfN` variants.
      if (!L.ConsumeInteger(Width) || isValidVecWidth(Width)) {
        // If there's nothing left to parse we're good to go.
        if (!L.Left()) {
          return eCLBuiltinVLoadHalf;
        }
      }
    } else if (L.ConsumeInteger(Width) && !L.Left() && isValidVecWidth(Width)) {
      // There are no scalar variants of this builtin.
      return eCLBuiltinVLoad;
    }
  } else if (L.Consume("vstore")) {
    unsigned Width = 0;
    if (L.Consume("_half")) {
      // We have both `vstore_half` and `vstore_halfN` variants.
      if (!L.ConsumeInteger(Width) || isValidVecWidth(Width)) {
        // Rounding modes are optional.
        L.Consume("_rte") || L.Consume("_rtz") || L.Consume("_rtp") ||
            L.Consume("_rtn");

        // If there's nothing left to parse we're good to go.
        if (!L.Left()) {
          return eCLBuiltinVStoreHalf;
        }
      }
    } else if (L.ConsumeInteger(Width) && !L.Left() && isValidVecWidth(Width)) {
      // There are no scalar variants of this builtin.
      return eCLBuiltinVStore;
    }
  } else if (L.Consume("as_")) {
    if (L.Consume("char") || L.Consume("uchar") || L.Consume("short") ||
        L.Consume("ushort") || L.Consume("int") || L.Consume("uint") ||
        L.Consume("long") || L.Consume("ulong") || L.Consume("float") ||
        L.Consume("double") || L.Consume("half")) {
      unsigned Width = 0;
      if (!L.ConsumeInteger(Width) || isValidVecWidth(Width)) {
        if (!L.Left()) {
          return eCLBuiltinAs;
        }
      }
    }
  }

  return eBuiltinUnknown;
}

llvm::StringRef CLBuiltinInfo::getBuiltinName(BuiltinID ID) const {
  const CLBuiltinEntry *entry = Builtins;
  while (entry->ID != eBuiltinInvalid) {
    if (ID == entry->ID) {
      return entry->opencl_name;
    }
    entry++;
  }
  return llvm::StringRef();
}

BuiltinUniformity CLBuiltinInfo::isBuiltinUniform(Builtin const &B,
                                                  const CallInst *CI,
                                                  unsigned SimdDimIdx) const {
  ConstantInt *Rank = nullptr;
  switch (B.ID) {
    default:
      break;
    case eCLBuiltinGetWorkDim:
    case eCLBuiltinGetGroupId:
    case eCLBuiltinGetGlobalSize:
    case eCLBuiltinGetGlobalOffset:
    case eCLBuiltinGetLocalSize:
    case eCLBuiltinGetNumGroups:
      return eBuiltinUniformityAlways;
    case eCLBuiltinAsyncWorkGroupCopy:
    case eCLBuiltinAsyncWorkGroupStridedCopy:
    case eCLBuiltinWaitGroupEvents:
      // These builtins will always be uniform within the same workgroup, as
      // otherwise their behaviour is undefined. They might not be across
      // workgroups, but we do not vectorize across workgroups anyway.
      return eBuiltinUniformityAlways;
    case eCLBuiltinGetGlobalId:
    case eCLBuiltinGetLocalId:
      // We need to know the rank of these builtins at compile time.
      if (!CI || CI->arg_empty()) {
        return eBuiltinUniformityNever;
      }
      Rank = dyn_cast<ConstantInt>(CI->getArgOperand(0));
      if (!Rank) {
        // The Rank is some function, which "might" evaluate to zero
        // sometimes, so we let the packetizer sort it out with some
        // conditional magic.
        // TODO Make sure this can never go haywire in weird edge cases.
        // Where we have one get_global_id() dependent on another, this is
        // not packetized correctly. Doing so is very hard!  We should
        // probably just fail to packetize in this case.  We might also be
        // able to return eBuiltinUniformityNever here, in cases where we can
        // prove that the value can never be zero.
        return eBuiltinUniformityMaybeInstanceID;
      }
      // Only vectorize on selected dimension. The value of get_global_id with
      // other ranks is uniform.
      if (Rank->getZExtValue() == SimdDimIdx) {
        return eBuiltinUniformityInstanceID;
      } else {
        return eBuiltinUniformityAlways;
      }
    case eCLBuiltinGetLocalLinearId:
    case eCLBuiltinGetGlobalLinearId:
      // TODO: This is fine for vectorizing in the x-axis, but currently we do
      // not support vectorizing along y or z (see CA-2843).
      return (SimdDimIdx) ? eBuiltinUniformityNever
                          : eBuiltinUniformityInstanceID;
    case eCLBuiltinGetSubgroupLocalId:
      return eBuiltinUniformityInstanceID;
    case eCLBuiltinSubgroupAll:
    case eCLBuiltinSubgroupAny:
    case eCLBuiltinSubgroupReduceAdd:
    case eCLBuiltinSubgroupReduceMax:
    case eCLBuiltinSubgroupReduceMin:
    case eCLBuiltinSubgroupBroadcast:
      return eBuiltinUniformityAlways;
  }

  // Assume that builtins with side effects are varying.
  if (Function *Callee = CI->getCalledFunction()) {
    auto const Props = analyzeBuiltin(*Callee).properties;
    if (Props & eBuiltinPropertySideEffects) {
      return eBuiltinUniformityNever;
    }
  }

  return eBuiltinUniformityLikeInputs;
}

Builtin CLBuiltinInfo::analyzeBuiltin(Function const &Callee) const {
  BuiltinID ID = identifyBuiltin(Callee);

  bool IsConvergent = false;
  unsigned Properties = eBuiltinPropertyNone;
  switch (ID) {
    default:
      // Assume convergence on unknown builtins.
      IsConvergent = true;
      break;
    case eBuiltinUnknown: {
      // Assume convergence on unknown builtins.
      IsConvergent = true;
      // If we know that this is an OpenCL builtin, but we don't have any
      // special information about it, we can determine if it has side effects
      // or not by its return type and its paramaters. This depends on being
      // able to identify all the "special" builtins, such as barriers and
      // fences.
      bool HasSideEffects = false;

      // Void functions have side effects
      if (Callee.getReturnType() == Type::getVoidTy(Callee.getContext())) {
        HasSideEffects = true;
      }
      // Functions that take pointers probably have side effects
      for (const auto &arg : Callee.args()) {
        if (arg.getType()->isPointerTy()) {
          HasSideEffects = true;
        }
      }
      Properties |= HasSideEffects ? eBuiltinPropertySideEffects
                                   : eBuiltinPropertyNoSideEffects;
    } break;
    case eCLBuiltinBarrier:
      IsConvergent = true;
      Properties |= eBuiltinPropertyExecutionFlow;
      Properties |= eBuiltinPropertySideEffects;
      Properties |= eBuiltinPropertyMapToMuxSyncBuiltin;
      break;
    case eCLBuiltinMemFence:
    case eCLBuiltinReadMemFence:
    case eCLBuiltinWriteMemFence:
      Properties |= eBuiltinPropertySupportsInstantiation;
      Properties |= eBuiltinPropertyMapToMuxSyncBuiltin;
      break;
    case eCLBuiltinPrintf:
      Properties |= eBuiltinPropertySideEffects;
      Properties |= eBuiltinPropertySupportsInstantiation;
      break;
    case eCLBuiltinAsyncWorkGroupCopy:
    case eCLBuiltinAsyncWorkGroupStridedCopy:
    case eCLBuiltinWaitGroupEvents:
      // Our implementation of these builtins uses thread checks against
      // specific work-item IDs, so they are convergent.
      IsConvergent = true;
      Properties |= eBuiltinPropertyNoSideEffects;
      break;
    case eCLBuiltinAtomicAdd:
    case eCLBuiltinAtomicSub:
    case eCLBuiltinAtomicXchg:
    case eCLBuiltinAtomicInc:
    case eCLBuiltinAtomicDec:
    case eCLBuiltinAtomicCmpxchg:
    case eCLBuiltinAtomicMin:
    case eCLBuiltinAtomicMax:
    case eCLBuiltinAtomicAnd:
    case eCLBuiltinAtomicOr:
    case eCLBuiltinAtomicXor:
      Properties |= eBuiltinPropertySideEffects;
      Properties |= eBuiltinPropertySupportsInstantiation;
      Properties |= eBuiltinPropertyAtomic;
      break;
    case eCLBuiltinGetWorkDim:
    case eCLBuiltinGetGroupId:
    case eCLBuiltinGetGlobalSize:
    case eCLBuiltinGetGlobalOffset:
    case eCLBuiltinGetNumGroups:
    case eCLBuiltinGetGlobalId:
    case eCLBuiltinGetLocalSize:
    case eCLBuiltinGetLocalLinearId:
    case eCLBuiltinGetSubgroupLocalId:
    case eCLBuiltinGetGlobalLinearId:
      Properties |= eBuiltinPropertyWorkItem;
      Properties |= eBuiltinPropertyRematerializable;
      break;
    case eCLBuiltinGetLocalId:
      Properties |= eBuiltinPropertyWorkItem;
      Properties |= eBuiltinPropertyLocalID;
      Properties |= eBuiltinPropertyRematerializable;
      break;
    case eCLBuiltinDot:
    case eCLBuiltinCross:
    case eCLBuiltinFastDistance:
    case eCLBuiltinFastLength:
    case eCLBuiltinFastNormalize:
      Properties |= eBuiltinPropertyReduction;
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinDistance:
    case eCLBuiltinLength:
    case eCLBuiltinNormalize:
      Properties |= eBuiltinPropertyReduction;
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      // XXX The inline implementation seems to have precision issues. The dot
      // product can overflow to +inf which results in the wrong result.
      // See redmine #6427 and #9115
      // Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinIsEqual:
    case eCLBuiltinIsNotEqual:
    case eCLBuiltinIsGreater:
    case eCLBuiltinIsGreaterEqual:
    case eCLBuiltinIsLess:
    case eCLBuiltinIsLessEqual:
    case eCLBuiltinIsLessGreater:
    case eCLBuiltinIsOrdered:
    case eCLBuiltinIsUnordered:
    case eCLBuiltinIsFinite:
    case eCLBuiltinIsInf:
    case eCLBuiltinIsNan:
    case eCLBuiltinIsNormal:
    case eCLBuiltinSignBit:
      // Scalar variants return '0' or '1', vector variants '0' or '111...1'.
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      Properties |= eBuiltinPropertySupportsInstantiation;
      break;
    case eCLBuiltinAny:
    case eCLBuiltinAll:
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinFract:
    case eCLBuiltinModF:
    case eCLBuiltinSinCos:
      Properties |= eBuiltinPropertyPointerReturnEqualRetTy;
      break;
    case eCLBuiltinFrexp:
    case eCLBuiltinLGammaR:
    case eCLBuiltinRemquo:
      Properties |= eBuiltinPropertyPointerReturnEqualIntRetTy;
      break;
    case eCLBuiltinShuffle:
    case eCLBuiltinShuffle2:
      // While there are vector equivalents for these builtins, they require a
      // modified mask, so we cannot use them by simply packetizing their
      // arguments.
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinFMax:
    case eCLBuiltinFMin:
    case eCLBuiltinAddSat:
    case eCLBuiltinSubSat:
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinCodeplayFaceForward:
    case eCLBuiltinCodeplayReflect:
    case eCLBuiltinCodeplayRefract:
      Properties |= eBuiltinPropertyReduction;
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      break;
    case eCLBuiltinConvertChar:
    case eCLBuiltinConvertShort:
    case eCLBuiltinConvertInt:
    case eCLBuiltinConvertLong:
    case eCLBuiltinConvertUChar:
    case eCLBuiltinConvertUShort:
    case eCLBuiltinConvertUInt:
    case eCLBuiltinConvertULong:
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinVLoad:
    case eCLBuiltinVLoadHalf:
      Properties |= eBuiltinPropertyNoSideEffects;
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinVStore:
    case eCLBuiltinVStoreHalf:
      Properties |= eBuiltinPropertySideEffects;
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinSelect:
    case eCLBuiltinAs:
      // Some of these builtins do have vector equivalents, but since we can
      // emit all variants inline, we mark them as having none for simplicity.
      Properties |= eBuiltinPropertyNoVectorEquivalent;
      Properties |= eBuiltinPropertyCanEmitInline;
      break;
    case eCLBuiltinWorkGroupBarrier:
    case eCLBuiltinSubGroupBarrier:
      IsConvergent = true;
      LLVM_FALLTHROUGH;
    case eCLBuiltinAtomicWorkItemFence:
      Properties |= eBuiltinPropertyMapToMuxSyncBuiltin;
      break;
      // Subgroup collectives
    case eCLBuiltinSubgroupAll:
    case eCLBuiltinSubgroupAny:
    case eCLBuiltinSubgroupBroadcast:
    case eCLBuiltinSubgroupReduceAdd:
    case eCLBuiltinSubgroupReduceMin:
    case eCLBuiltinSubgroupReduceMax:
    case eCLBuiltinSubgroupScanAddInclusive:
    case eCLBuiltinSubgroupScanAddExclusive:
    case eCLBuiltinSubgroupScanMinInclusive:
    case eCLBuiltinSubgroupScanMinExclusive:
    case eCLBuiltinSubgroupScanMaxInclusive:
    case eCLBuiltinSubgroupScanMaxExclusive:
    case eCLBuiltinSubgroupReduceMul:
    case eCLBuiltinSubgroupReduceAnd:
    case eCLBuiltinSubgroupReduceOr:
    case eCLBuiltinSubgroupReduceXor:
    case eCLBuiltinSubgroupReduceLogicalAnd:
    case eCLBuiltinSubgroupReduceLogicalOr:
    case eCLBuiltinSubgroupReduceLogicalXor:
    case eCLBuiltinSubgroupScanMulInclusive:
    case eCLBuiltinSubgroupScanMulExclusive:
    case eCLBuiltinSubgroupScanAndInclusive:
    case eCLBuiltinSubgroupScanAndExclusive:
    case eCLBuiltinSubgroupScanOrInclusive:
    case eCLBuiltinSubgroupScanOrExclusive:
    case eCLBuiltinSubgroupScanXorInclusive:
    case eCLBuiltinSubgroupScanXorExclusive:
    case eCLBuiltinSubgroupScanLogicalAndInclusive:
    case eCLBuiltinSubgroupScanLogicalAndExclusive:
    case eCLBuiltinSubgroupScanLogicalOrInclusive:
    case eCLBuiltinSubgroupScanLogicalOrExclusive:
    case eCLBuiltinSubgroupScanLogicalXorInclusive:
    case eCLBuiltinSubgroupScanLogicalXorExclusive:
      // Work-group collectives
    case eCLBuiltinWorkgroupAll:
    case eCLBuiltinWorkgroupAny:
    case eCLBuiltinWorkgroupBroadcast:
    case eCLBuiltinWorkgroupReduceAdd:
    case eCLBuiltinWorkgroupReduceMin:
    case eCLBuiltinWorkgroupReduceMax:
    case eCLBuiltinWorkgroupScanAddInclusive:
    case eCLBuiltinWorkgroupScanAddExclusive:
    case eCLBuiltinWorkgroupScanMinInclusive:
    case eCLBuiltinWorkgroupScanMinExclusive:
    case eCLBuiltinWorkgroupScanMaxInclusive:
    case eCLBuiltinWorkgroupScanMaxExclusive:
    case eCLBuiltinWorkgroupReduceMul:
    case eCLBuiltinWorkgroupReduceAnd:
    case eCLBuiltinWorkgroupReduceOr:
    case eCLBuiltinWorkgroupReduceXor:
    case eCLBuiltinWorkgroupReduceLogicalAnd:
    case eCLBuiltinWorkgroupReduceLogicalOr:
    case eCLBuiltinWorkgroupReduceLogicalXor:
    case eCLBuiltinWorkgroupScanMulInclusive:
    case eCLBuiltinWorkgroupScanMulExclusive:
    case eCLBuiltinWorkgroupScanAndInclusive:
    case eCLBuiltinWorkgroupScanAndExclusive:
    case eCLBuiltinWorkgroupScanOrInclusive:
    case eCLBuiltinWorkgroupScanOrExclusive:
    case eCLBuiltinWorkgroupScanXorInclusive:
    case eCLBuiltinWorkgroupScanXorExclusive:
    case eCLBuiltinWorkgroupScanLogicalAndInclusive:
    case eCLBuiltinWorkgroupScanLogicalAndExclusive:
    case eCLBuiltinWorkgroupScanLogicalOrInclusive:
    case eCLBuiltinWorkgroupScanLogicalOrExclusive:
    case eCLBuiltinWorkgroupScanLogicalXorInclusive:
    case eCLBuiltinWorkgroupScanLogicalXorExclusive:
      IsConvergent = true;
      break;
  }

  if (!IsConvergent) {
    Properties |= eBuiltinPropertyKnownNonConvergent;
  }

  return Builtin{Callee, ID, (BuiltinProperties)Properties};
}

Function *CLBuiltinInfo::getVectorEquivalent(Builtin const &B, unsigned Width,
                                             Module *M) {
  // Analyze the builtin. Some functions have no vector equivalent.
  auto const Props = B.properties;
  if (Props & eBuiltinPropertyNoVectorEquivalent) {
    return nullptr;
  }

  // Builtin functions have mangled names. If it's not mangled, there will be
  // no vector equivalent.
  NameMangler Mangler(&B.function.getContext(), M);
  SmallVector<Type *, 4> BuiltinArgTypes, BuiltinPointeeTypes;
  SmallVector<TypeQualifiers, 4> BuiltinArgQuals;
  StringRef BuiltinName =
      Mangler.demangleName(B.function.getName(), BuiltinArgTypes,
                           BuiltinPointeeTypes, BuiltinArgQuals);
  if (BuiltinName.empty()) {
    return nullptr;
  }

  // Determine the mangled name of the vector equivalent.
  // This means creating a list of qualified types for the arguments.
  SmallVector<Type *, 4> VectorTypes;
  SmallVector<TypeQualifiers, 4> VectorQuals;
  for (unsigned i = 0; i < BuiltinArgTypes.size(); i++) {
    Type *OldTy = BuiltinArgTypes[i];
    TypeQualifiers OldQuals = BuiltinArgQuals[i];
    if (isa<FixedVectorType>(OldTy)) {
      return nullptr;
    }
    PointerType *OldPtrTy = dyn_cast<PointerType>(OldTy);
    if (OldPtrTy) {
      if (auto *const PtrRetPointeeTy =
              getPointerReturnPointeeTy(B.function, Props)) {
        auto *OldPointeeTy = BuiltinPointeeTypes[i];
        (void)OldPointeeTy;
        assert(
            OldPointeeTy && OldPointeeTy == PtrRetPointeeTy &&
            multi_llvm::isOpaqueOrPointeeTypeMatches(OldPtrTy, OldPointeeTy) &&
            "Demangling inconsistency");
        if (!FixedVectorType::isValidElementType(PtrRetPointeeTy)) {
          return nullptr;
        }
        Type *NewEleTy = FixedVectorType::get(PtrRetPointeeTy, Width);
        Type *NewType = PointerType::get(NewEleTy, OldPtrTy->getAddressSpace());
        TypeQualifiers NewQuals;
        TypeQualifiers EleQuals = OldQuals;
        NewQuals.push_back(EleQuals.pop_front());  // Pointer qualifier
        NewQuals.push_back(eTypeQualNone);         // Vector qualifier
        NewQuals.push_back(EleQuals);

        VectorTypes.push_back(NewType);
        VectorQuals.push_back(NewQuals);

        continue;
      }
    }

    if (!FixedVectorType::isValidElementType(OldTy)) {
      return nullptr;
    }
    TypeQualifiers NewQuals;
    Type *NewType = FixedVectorType::get(OldTy, Width);
    NewQuals.push_back(eTypeQualNone);  // Vector qualifier
    NewQuals.push_back(OldQuals);       // Element qualifier

    VectorTypes.push_back(NewType);
    VectorQuals.push_back(NewQuals);
  }

  // Handle special builtin naming equivalents.
  std::string EquivNameBase = BuiltinName.str();
  StringRef FirstChunk;
  Lexer L(BuiltinName);
  if (L.ConsumeUntil('_', FirstChunk)) {
    bool AsBuiltin = FirstChunk.equals("as");
    bool ConvertBuiltin = FirstChunk.equals("convert");
    if (!L.Consume("_")) {
      return nullptr;
    }
    StringRef SecondChunkNoWidth;
    if (!L.ConsumeAlpha(SecondChunkNoWidth)) {
      return nullptr;
    }
    if (AsBuiltin || ConvertBuiltin) {
      // as_* and convert_* builtins have vector equivalents, with a vector
      // width suffix. Add the width suffix to the scalar builtin name.
      if (AsBuiltin && L.Left()) {
        return nullptr;
      }
      Twine WidthText(Width);
      EquivNameBase.insert(L.CurrentPos(), WidthText.str());
    }
  }

  std::string EquivName =
      Mangler.mangleName(EquivNameBase, VectorTypes, VectorQuals);

  // Lookup the vector equivalent and make sure the return type agrees.
  Function *VectorBuiltin = materializeBuiltin(EquivName, M);
  if (VectorBuiltin) {
    Type *RetTy = B.function.getReturnType();
    auto *VecRetTy = dyn_cast<FixedVectorType>(VectorBuiltin->getReturnType());
    if (!VecRetTy || (VecRetTy->getElementType() != RetTy) ||
        (VecRetTy->getNumElements() != Width)) {
      VectorBuiltin = nullptr;
    }
  }
  return VectorBuiltin;
}

Function *CLBuiltinInfo::getScalarEquivalent(Builtin const &B, Module *M) {
  // Analyze the builtin. Some functions have no scalar equivalent.
  auto const Props = B.properties;
  if (Props & eBuiltinPropertyNoVectorEquivalent) {
    return nullptr;
  }

  // Check the return type.
  auto *VecRetTy = dyn_cast<FixedVectorType>(B.function.getReturnType());
  if (!VecRetTy) {
    return nullptr;
  }

  // Builtin functions have mangled names. If it's not mangled, there will be
  // no scalar equivalent.
  NameMangler Mangler(&B.function.getContext());
  SmallVector<Type *, 4> BuiltinArgTypes, BuiltinPointeeTypes;
  SmallVector<TypeQualifiers, 4> BuiltinArgQuals;
  StringRef BuiltinName =
      Mangler.demangleName(B.function.getName(), BuiltinArgTypes,
                           BuiltinPointeeTypes, BuiltinArgQuals);
  if (BuiltinName.empty()) {
    return nullptr;
  }

  // Determine the mangled name of the scalar equivalent.
  // This means creating a list of qualified types for the arguments.
  unsigned Width = VecRetTy->getNumElements();
  SmallVector<Type *, 4> ScalarTypes;
  SmallVector<TypeQualifiers, 4> ScalarQuals;
  for (unsigned i = 0; i < BuiltinArgTypes.size(); i++) {
    Type *OldTy = BuiltinArgTypes[i];
    TypeQualifiers OldQuals = BuiltinArgQuals[i];
    if (auto *OldVecTy = dyn_cast<FixedVectorType>(OldTy)) {
      if (OldVecTy->getNumElements() != Width) {
        return nullptr;
      }
      Type *NewTy = OldVecTy->getElementType();
      TypeQualifiers NewQuals = OldQuals;
      NewQuals.pop_front();

      ScalarTypes.push_back(NewTy);
      ScalarQuals.push_back(NewQuals);
    } else if (PointerType *OldPtrTy = dyn_cast<PointerType>(OldTy)) {
      Type *const PtrRetPointeeTy =
          getPointerReturnPointeeTy(B.function, Props);
      if (PtrRetPointeeTy && PtrRetPointeeTy->isVectorTy()) {
        auto *OldPointeeTy = BuiltinPointeeTypes[i];
        (void)OldPointeeTy;
        assert(
            OldPointeeTy && OldPointeeTy == PtrRetPointeeTy &&
            multi_llvm::isOpaqueOrPointeeTypeMatches(OldPtrTy, OldPointeeTy) &&
            "Demangling inconsistency");
        auto *OldVecTy = cast<FixedVectorType>(PtrRetPointeeTy);
        Type *NewTy = PointerType::get(OldVecTy->getElementType(),
                                       OldPtrTy->getAddressSpace());
        TypeQualifiers NewQuals = OldQuals;
        TypeQualifier PtrQual = NewQuals.pop_front();
        TypeQualifier VecQual = NewQuals.pop_front();
        (void)VecQual;
        TypeQualifier EleQual = NewQuals.pop_front();
        NewQuals.push_back(PtrQual);
        NewQuals.push_back(EleQual);
        ScalarTypes.push_back(NewTy);
        ScalarQuals.push_back(NewQuals);
      } else {
        ScalarTypes.push_back(OldTy);
        ScalarQuals.push_back(OldQuals);
      }
    } else {
      if (!OldTy) {
        return nullptr;
      }
      ScalarTypes.push_back(OldTy);
      ScalarQuals.push_back(OldQuals);
    }
  }

  // Handle special builtin naming equivalents.
  std::string EquivNameBase = BuiltinName.str();
  StringRef FirstChunk;
  Lexer L(BuiltinName);
  if (L.ConsumeUntil('_', FirstChunk)) {
    bool AsBuiltin = FirstChunk.equals("as");
    bool ConvertBuiltin = FirstChunk.equals("convert");
    if (!L.Consume("_")) {
      return nullptr;
    }
    StringRef SecondChunkNoWidth;
    if (!L.ConsumeAlpha(SecondChunkNoWidth)) {
      return nullptr;
    }
    if (AsBuiltin || ConvertBuiltin) {
      // as_* and convert_* builtins have scalar equivalents, with no width
      // suffix. Remove the width suffix from the vector builtin name.
      unsigned WidthStart = L.CurrentPos();
      unsigned Width = 0;
      if (!L.ConsumeInteger(Width)) {
        return nullptr;
      }
      unsigned WidthEnd = L.CurrentPos();
      EquivNameBase.erase(WidthStart, WidthEnd - WidthStart);
    }
  }

  std::string EquivName =
      Mangler.mangleName(EquivNameBase, ScalarTypes, ScalarQuals);

  // Lookup the scalar equivalent and make sure the return type agrees.
  Function *ScalarBuiltin = materializeBuiltin(EquivName, M);
  if (!ScalarBuiltin) {
    return nullptr;
  }
  Type *RetTy = ScalarBuiltin->getReturnType();
  if (VecRetTy->getElementType() != RetTy) {
    return nullptr;
  }
  return ScalarBuiltin;
}

BuiltinSubgroupReduceKind CLBuiltinInfo::getBuiltinSubgroupReductionKind(
    Builtin const &B) const {
  switch (B.ID) {
    default:
      return eBuiltinSubgroupReduceInvalid;
    case eCLBuiltinSubgroupAll:
      return eBuiltinSubgroupAll;
    case eCLBuiltinSubgroupAny:
      return eBuiltinSubgroupAny;
    case eCLBuiltinSubgroupReduceAdd:
      return eBuiltinSubgroupReduceAdd;
    case eCLBuiltinSubgroupReduceMin:
      return eBuiltinSubgroupReduceMin;
    case eCLBuiltinSubgroupReduceMax:
      return eBuiltinSubgroupReduceMax;
      // Subgroup reductions provided by SPV_KHR_uniform_group_instructions.
    case eCLBuiltinSubgroupReduceMul:
      return eBuiltinSubgroupReduceMul;
    case eCLBuiltinSubgroupReduceAnd:
      return eBuiltinSubgroupReduceAnd;
    case eCLBuiltinSubgroupReduceOr:
      return eBuiltinSubgroupReduceOr;
    case eCLBuiltinSubgroupReduceXor:
      return eBuiltinSubgroupReduceXor;
    case eCLBuiltinSubgroupReduceLogicalAnd:
      return eBuiltinSubgroupReduceLogicalAnd;
    case eCLBuiltinSubgroupReduceLogicalOr:
      return eBuiltinSubgroupReduceLogicalOr;
    case eCLBuiltinSubgroupReduceLogicalXor:
      return eBuiltinSubgroupReduceLogicalXor;
  }
}

BuiltinSubgroupScanKind CLBuiltinInfo::getBuiltinSubgroupScanKind(
    Builtin const &B) const {
  switch (B.ID) {
    default:
      return eBuiltinSubgroupScanInvalid;
    case eCLBuiltinSubgroupScanAddInclusive:
      return eBuiltinSubgroupScanAddIncl;
    case eCLBuiltinSubgroupScanAddExclusive:
      return eBuiltinSubgroupScanAddExcl;
    case eCLBuiltinSubgroupScanMinInclusive:
      return eBuiltinSubgroupScanMinIncl;
    case eCLBuiltinSubgroupScanMinExclusive:
      return eBuiltinSubgroupScanMinExcl;
    case eCLBuiltinSubgroupScanMaxInclusive:
      return eBuiltinSubgroupScanMaxIncl;
    case eCLBuiltinSubgroupScanMaxExclusive:
      return eBuiltinSubgroupScanMaxExcl;
      // Subgroup scans provided by SPV_KHR_uniform_group_instructions.
    case eCLBuiltinSubgroupScanMulInclusive:
      return eBuiltinSubgroupScanMulIncl;
    case eCLBuiltinSubgroupScanMulExclusive:
      return eBuiltinSubgroupScanMulExcl;
    case eCLBuiltinSubgroupScanAndInclusive:
      return eBuiltinSubgroupScanAndIncl;
    case eCLBuiltinSubgroupScanAndExclusive:
      return eBuiltinSubgroupScanAndExcl;
    case eCLBuiltinSubgroupScanOrInclusive:
      return eBuiltinSubgroupScanOrIncl;
    case eCLBuiltinSubgroupScanOrExclusive:
      return eBuiltinSubgroupScanOrExcl;
    case eCLBuiltinSubgroupScanXorInclusive:
      return eBuiltinSubgroupScanXorIncl;
    case eCLBuiltinSubgroupScanXorExclusive:
      return eBuiltinSubgroupScanXorExcl;
    case eCLBuiltinSubgroupScanLogicalAndInclusive:
      return eBuiltinSubgroupScanLogicalAndIncl;
    case eCLBuiltinSubgroupScanLogicalAndExclusive:
      return eBuiltinSubgroupScanLogicalAndExcl;
    case eCLBuiltinSubgroupScanLogicalOrInclusive:
      return eBuiltinSubgroupScanLogicalOrIncl;
    case eCLBuiltinSubgroupScanLogicalOrExclusive:
      return eBuiltinSubgroupScanLogicalOrExcl;
    case eCLBuiltinSubgroupScanLogicalXorInclusive:
      return eBuiltinSubgroupScanLogicalXorIncl;
    case eCLBuiltinSubgroupScanLogicalXorExclusive:
      return eBuiltinSubgroupScanLogicalXorExcl;
  }
}

/// @brief Returns whether the parameter corresponding to given index to the
/// (assumed builtin) Function is known to possess the given qualifier.
/// @return true if the parameter is known to have the qualifier, false if not,
/// and None on error.
static multi_llvm::Optional<bool> paramHasTypeQual(const Function &F,
                                                   unsigned ParamIdx,
                                                   TypeQualifier Q) {
  // Demangle the function name to get the type qualifiers.
  SmallVector<Type *, 2> Types;
  SmallVector<TypeQualifiers, 2> Quals;
  NameMangler Mangler(&F.getContext());
  if (Mangler.demangleName(F.getName(), Types, Quals).empty()) {
    return multi_llvm::None;
  }

  if (ParamIdx >= Quals.size()) {
    return multi_llvm::None;
  }

  auto &Qual = Quals[ParamIdx];
  while (Qual.getCount()) {
    if (Qual.pop_front() == Q) {
      return true;
    }
  }
  return false;
}

Value *CLBuiltinInfo::emitBuiltinInline(Function *F, IRBuilder<> &B,
                                        ArrayRef<Value *> Args) {
  if (!F) {
    return nullptr;
  }

  // Handle 'common' builtins.
  BuiltinID BuiltinID = identifyBuiltin(*F);
  if (BuiltinID != eBuiltinInvalid && BuiltinID != eBuiltinUnknown) {
    // Note we have to handle these specially since we need to deduce whether
    // the source operand is signed or not. It is not possible to do this based
    // solely on the BuiltinID.
    switch (BuiltinID) {
        // 6.2 Explicit Conversions
      case eCLBuiltinConvertChar:
      case eCLBuiltinConvertShort:
      case eCLBuiltinConvertInt:
      case eCLBuiltinConvertLong:
      case eCLBuiltinConvertUChar:
      case eCLBuiltinConvertUShort:
      case eCLBuiltinConvertUInt:
      case eCLBuiltinConvertULong:
        return emitBuiltinInlineConvert(F, BuiltinID, B, Args);
        // 6.12.3 Integer Functions
      case eCLBuiltinAddSat:
      case eCLBuiltinSubSat: {
        multi_llvm::Optional<bool> IsParamSignedOrNone =
            paramHasTypeQual(*F, 0, eTypeQualSignedInt);
        if (!IsParamSignedOrNone.has_value()) {
          return nullptr;
        }
        bool IsSigned = *IsParamSignedOrNone;
        Intrinsic::ID IntrinsicOpc =
            BuiltinID == eCLBuiltinSubSat
                ? (IsSigned ? Intrinsic::ssub_sat : Intrinsic::usub_sat)
                : (IsSigned ? Intrinsic::sadd_sat : Intrinsic::uadd_sat);
        return emitBuiltinInlineAsLLVMBinaryIntrinsic(B, Args[0], Args[1],
                                                      IntrinsicOpc);
      }
      case eCLBuiltinVLoad: {
        NameMangler Mangler(&F->getContext());
        Lexer L(Mangler.demangleName(F->getName()));
        if (L.Consume("vload")) {
          unsigned Width = 0;
          if (L.ConsumeInteger(Width)) {
            return emitBuiltinInlineVLoad(F, Width, B, Args);
          }
        }
      } break;
      case eCLBuiltinVLoadHalf: {
        NameMangler Mangler(&F->getContext());
        auto const name = Mangler.demangleName(F->getName());
        if (name == "vload_half") {
          // TODO CA-4691 handle "vload_halfn"
          return emitBuiltinInlineVLoadHalf(F, B, Args);
        }
      } break;
      case eCLBuiltinVStore: {
        NameMangler Mangler(&F->getContext());
        Lexer L(Mangler.demangleName(F->getName()));
        if (L.Consume("vstore")) {
          unsigned Width = 0;
          if (L.ConsumeInteger(Width)) {
            return emitBuiltinInlineVStore(F, Width, B, Args);
          }
        }
      } break;
      case eCLBuiltinVStoreHalf: {
        NameMangler Mangler(&F->getContext());
        Lexer L(Mangler.demangleName(F->getName()));
        if (L.Consume("vstore_half")) {
          // TODO CA-4691 handle "vstore_halfn"
          return emitBuiltinInlineVStoreHalf(F, L.TextLeft(), B, Args);
        }
      } break;
      case eCLBuiltinSelect:
        return emitBuiltinInlineSelect(F, B, Args);
      case eCLBuiltinAs:
        return emitBuiltinInlineAs(F, B, Args);
      default:
        break;
    }
    return emitBuiltinInline(BuiltinID, B, Args);
  }

  return nullptr;
}

Value *CLBuiltinInfo::emitBuiltinInline(BuiltinID BuiltinID, IRBuilder<> &B,
                                        ArrayRef<Value *> Args) {
  switch (BuiltinID) {
    default:
      return nullptr;

    case eCLBuiltinDot:
    case eCLBuiltinCross:
    case eCLBuiltinLength:
    case eCLBuiltinDistance:
    case eCLBuiltinNormalize:
    case eCLBuiltinFastLength:
    case eCLBuiltinFastDistance:
    case eCLBuiltinFastNormalize:
      return emitBuiltinInlineGeometrics(BuiltinID, B, Args);
    // 6.12.2 Math Functions
    case eCLBuiltinFMax:
      return emitBuiltinInlineAsLLVMBinaryIntrinsic(B, Args[0], Args[1],
                                                    llvm::Intrinsic::maxnum);
    case eCLBuiltinFMin:
      return emitBuiltinInlineAsLLVMBinaryIntrinsic(B, Args[0], Args[1],
                                                    llvm::Intrinsic::minnum);
    // 6.12.6 Relational Functions
    case eCLBuiltinAll:
      return emitBuiltinInlineAll(B, Args);
    case eCLBuiltinAny:
      return emitBuiltinInlineAny(B, Args);
    case eCLBuiltinIsEqual:
    case eCLBuiltinIsNotEqual:
    case eCLBuiltinIsGreater:
    case eCLBuiltinIsGreaterEqual:
    case eCLBuiltinIsLess:
    case eCLBuiltinIsLessEqual:
    case eCLBuiltinIsLessGreater:
    case eCLBuiltinIsOrdered:
    case eCLBuiltinIsUnordered:
      return emitBuiltinInlineRelationalsWithTwoArguments(BuiltinID, B, Args);
    case eCLBuiltinIsFinite:
    case eCLBuiltinIsInf:
    case eCLBuiltinIsNan:
    case eCLBuiltinIsNormal:
    case eCLBuiltinSignBit:
      assert(Args.size() == 1 && "Invalid number of arguments");
      return emitBuiltinInlineRelationalsWithOneArgument(BuiltinID, B, Args[0]);
    // 6.12.12 Miscellaneous Vector Functions
    case eCLBuiltinShuffle:
    case eCLBuiltinShuffle2:
      return emitBuiltinInlineShuffle(BuiltinID, B, Args);

    case eCLBuiltinPrintf:
      return emitBuiltinInlinePrintf(BuiltinID, B, Args);
  }
}

Value *CLBuiltinInfo::emitBuiltinInlineGeometrics(BuiltinID BuiltinID,
                                                  IRBuilder<> &B,
                                                  ArrayRef<Value *> Args) {
  Value *Src = nullptr;
  switch (BuiltinID) {
    default:
      return nullptr;
    case eCLBuiltinDot:
      return emitBuiltinInlineDot(B, Args);
    case eCLBuiltinCross:
      return emitBuiltinInlineCross(B, Args);
    case eCLBuiltinLength:
    case eCLBuiltinFastLength:
      return emitBuiltinInlineLength(B, Args);
    case eCLBuiltinDistance:
    case eCLBuiltinFastDistance:
      if (Args.size() != 2) {
        return nullptr;
      }
      Src = B.CreateFSub(Args[0], Args[1], "distance");
      return emitBuiltinInlineLength(B, ArrayRef<Value *>(&Src, 1));
    case eCLBuiltinNormalize:
    case eCLBuiltinFastNormalize:
      return emitBuiltinInlineNormalize(B, Args);
  }
}

Value *CLBuiltinInfo::emitBuiltinInlineDot(IRBuilder<> &B,
                                           ArrayRef<Value *> Args) {
  if (Args.size() != 2) {
    return nullptr;
  }
  Value *Src0 = Args[0];
  Value *Src1 = Args[1];
  auto *SrcVecTy = dyn_cast<FixedVectorType>(Src0->getType());
  if (SrcVecTy) {
    Value *LHS0 = B.CreateExtractElement(Src0, B.getInt32(0), "lhs");
    Value *RHS0 = B.CreateExtractElement(Src1, B.getInt32(0), "rhs");
    Value *Sum = B.CreateFMul(LHS0, RHS0, "dot");
    for (unsigned i = 1; i < SrcVecTy->getNumElements(); i++) {
      Value *LHS = B.CreateExtractElement(Src0, B.getInt32(i), "lhs");
      Value *RHS = B.CreateExtractElement(Src1, B.getInt32(i), "rhs");
      Sum = B.CreateFAdd(Sum, B.CreateFMul(LHS, RHS, "dot"), "dot");
    }
    return Sum;
  } else {
    return B.CreateFMul(Src0, Src1, "dot");
  }
}

Value *CLBuiltinInfo::emitBuiltinInlineCross(IRBuilder<> &B,
                                             ArrayRef<Value *> Args) {
  if (Args.size() != 2) {
    return nullptr;
  }
  Value *Src0 = Args[0];
  Value *Src1 = Args[1];
  auto *RetTy = dyn_cast<FixedVectorType>(Src0->getType());
  if (!RetTy) {
    return nullptr;
  }
  const int SrcIndices[] = {1, 2, 2, 0, 0, 1};
  SmallVector<Value *, 4> Src0Lanes;
  SmallVector<Value *, 4> Src1Lanes;
  for (unsigned i = 0; i < 3; i++) {
    Src0Lanes.push_back(B.CreateExtractElement(Src0, B.getInt32(i)));
    Src1Lanes.push_back(B.CreateExtractElement(Src1, B.getInt32(i)));
  }

  Value *Result = UndefValue::get(RetTy);
  for (unsigned i = 0; i < 3; i++) {
    int Idx0 = SrcIndices[(i * 2) + 0];
    int Idx1 = SrcIndices[(i * 2) + 1];
    Value *Src0A = Src0Lanes[Idx0];
    Value *Src1A = Src1Lanes[Idx1];
    Value *TempA = B.CreateFMul(Src0A, Src1A);
    Value *Src0B = Src0Lanes[Idx1];
    Value *Src1B = Src1Lanes[Idx0];
    Value *TempB = B.CreateFMul(Src0B, Src1B);
    Value *Lane = B.CreateFSub(TempA, TempB);
    Result = B.CreateInsertElement(Result, Lane, B.getInt32(i));
  }
  if (RetTy->getNumElements() == 4) {
    Type *EleTy = RetTy->getElementType();
    Result = B.CreateInsertElement(Result, Constant::getNullValue(EleTy),
                                   B.getInt32(3));
  }
  return Result;
}

Value *CLBuiltinInfo::emitBuiltinInlineLength(IRBuilder<> &B,
                                              ArrayRef<Value *> Args) {
  if (Args.size() != 1) {
    return nullptr;
  }
  Value *Src0 = Args[0];
  Value *Src1 = Src0;

  NameMangler Mangler(&B.getContext());
  Type *SrcType = Src0->getType();
  auto *SrcVecType = dyn_cast<FixedVectorType>(SrcType);
  if (SrcVecType) {
    SrcType = SrcVecType->getElementType();
  }

  TypeQualifiers SrcQuals;
  SmallVector<Type *, 4> Tys;
  SmallVector<TypeQualifiers, 4> Quals;
  SrcQuals.push_back(eTypeQualNone);

  // Materialize 'sqrt', 'fabs' and 'isinf'.
  Tys.push_back(SrcType);
  Quals.push_back(SrcQuals);
  BasicBlock *BB = B.GetInsertBlock();
  if (!BB) {
    return nullptr;
  }
  Function *F = BB->getParent();
  if (!F) {
    return nullptr;
  }
  Module *M = F->getParent();
  if (!M) {
    return nullptr;
  }

  std::string FabsName = Mangler.mangleName("fabs", Tys, Quals);
  Function *Fabs = materializeBuiltin(FabsName, M);
  if (!Fabs) {
    return nullptr;
  }
  if (!SrcVecType) {
    // The "length" of a scalar is just the absolute value.
    return CreateBuiltinCall(B, Fabs, Src0, "scalar_length");
  }

  std::string SqrtName = Mangler.mangleName("sqrt", Tys, Quals);
  Function *Sqrt = materializeBuiltin(SqrtName, M);
  if (!Sqrt) {
    return nullptr;
  }

  std::string IsInfName = Mangler.mangleName("isinf", Tys, Quals);
  Function *IsInf = materializeBuiltin(IsInfName, M);
  if (!IsInf) {
    return nullptr;
  }
  Tys.clear();
  Quals.clear();

  // Materialize 'fmax'.
  Tys.push_back(SrcType);
  Quals.push_back(SrcQuals);
  Tys.push_back(SrcType);
  Quals.push_back(SrcQuals);
  std::string FmaxName = Mangler.mangleName("fmax", Tys, Quals);
  Function *Fmax = materializeBuiltin(FmaxName, M);
  if (!Fmax) {
    return nullptr;
  }

  // Emit length or distance inline.
  SmallVector<Value *, 4> Ops;
  Ops.push_back(Src0);
  Ops.push_back(Src1);
  Value *Result = emitBuiltinInline(eCLBuiltinDot, B, Ops);
  Result = CreateBuiltinCall(B, Sqrt, Result, "result");

  // Handle the case where the result is infinite.
  Value *AltResult = ConstantFP::get(SrcType, 0.0);
  if (SrcVecType) {
    for (unsigned i = 0; i < SrcVecType->getNumElements(); i++) {
      Value *SrcLane = B.CreateExtractElement(Src0, B.getInt32(i), "src_lane");
      SrcLane = CreateBuiltinCall(B, Fabs, SrcLane, "src_lane");
      AltResult =
          CreateBuiltinCall(B, Fmax, {SrcLane, AltResult}, "alt_result");
    }
  } else {
    Value *SrcLane = CreateBuiltinCall(B, Fabs, Src0, "src_lane");
    AltResult = CreateBuiltinCall(B, Fmax, {SrcLane, AltResult}, "alt_result");
  }
  Value *Cond = CreateBuiltinCall(B, IsInf, Result, "cond");
  Cond = B.CreateICmpEQ(Cond, B.getInt32(0), "cmp");
  Result = B.CreateSelect(Cond, Result, AltResult, "final_result");
  return Result;
}

Value *CLBuiltinInfo::emitBuiltinInlineNormalize(IRBuilder<> &B,
                                                 ArrayRef<Value *> Args) {
  if (Args.size() != 1) {
    return nullptr;
  }

  Value *Src0 = Args[0];

  NameMangler Mangler(&B.getContext());
  Type *SrcType = Src0->getType();
  auto *SrcVecType = dyn_cast<FixedVectorType>(SrcType);
  if (SrcVecType) {
    SrcType = SrcVecType->getElementType();
  }

  TypeQualifiers SrcQuals;
  SmallVector<Type *, 4> Tys;
  SmallVector<TypeQualifiers, 4> Quals;
  SrcQuals.push_back(eTypeQualNone);

  // Materialize 'rsqrt'.
  Tys.push_back(SrcType);
  Quals.push_back(SrcQuals);
  BasicBlock *BB = B.GetInsertBlock();
  if (!BB) {
    return nullptr;
  }
  Function *F = BB->getParent();
  if (!F) {
    return nullptr;
  }
  Module *M = F->getParent();
  if (!M) {
    return nullptr;
  }

  if (!SrcVecType) {
    // A normalized scalar is either 1.0 or -1.0, unless the input was NaN, or
    // in other words, just the sign.
    std::string SignName = Mangler.mangleName("sign", Tys, Quals);
    Function *Sign = materializeBuiltin(SignName, M);
    if (!Sign) {
      return nullptr;
    }
    return CreateBuiltinCall(B, Sign, Src0, "scalar_normalize");
  }

  std::string RSqrtName = Mangler.mangleName("rsqrt", Tys, Quals);
  Function *RSqrt = materializeBuiltin(RSqrtName, M);
  if (!RSqrt) {
    return nullptr;
  }

  // Call 'dot' on the input.
  SmallVector<Value *, 4> DotArgs;
  DotArgs.push_back(Src0);
  DotArgs.push_back(Src0);
  Value *Result = emitBuiltinInlineDot(B, DotArgs);
  Result = CreateBuiltinCall(B, RSqrt, Result, "normalize");
  if (SrcVecType) {
    Result = B.CreateVectorSplat(SrcVecType->getNumElements(), Result);
  }
  Result = B.CreateFMul(Result, Src0, "normalized");
  return Result;
}

static Value *emitAllAnyReduction(IRBuilder<> &B, ArrayRef<Value *> Args,
                                  Instruction::BinaryOps ReduceOp) {
  if (Args.size() != 1) {
    return nullptr;
  }
  Value *Arg0 = Args[0];
  IntegerType *EleTy = dyn_cast<IntegerType>(Arg0->getType()->getScalarType());
  if (!EleTy) {
    return nullptr;
  }

  // Reduce the MSB of all vector lanes.
  Value *ReducedVal = nullptr;
  auto *VecTy = dyn_cast<FixedVectorType>(Arg0->getType());
  if (VecTy) {
    ReducedVal = B.CreateExtractElement(Arg0, B.getInt32(0));
    for (unsigned i = 1; i < VecTy->getNumElements(); i++) {
      Value *Lane = B.CreateExtractElement(Arg0, B.getInt32(i));
      ReducedVal = B.CreateBinOp(ReduceOp, ReducedVal, Lane);
    }
  } else {
    ReducedVal = Arg0;
  }

  // Shift the MSB to return either 0 or 1.
  unsigned ShiftAmount = EleTy->getPrimitiveSizeInBits() - 1;
  Value *ShiftAmountVal = ConstantInt::get(EleTy, ShiftAmount);
  Value *Result = B.CreateLShr(ReducedVal, ShiftAmountVal);
  return B.CreateZExtOrTrunc(Result, B.getInt32Ty());
}

Value *CLBuiltinInfo::emitBuiltinInlineAll(IRBuilder<> &B,
                                           ArrayRef<Value *> Args) {
  return emitAllAnyReduction(B, Args, Instruction::And);
}

Value *CLBuiltinInfo::emitBuiltinInlineAny(IRBuilder<> &B,
                                           ArrayRef<Value *> Args) {
  return emitAllAnyReduction(B, Args, Instruction::Or);
}

Value *CLBuiltinInfo::emitBuiltinInlineSelect(Function *F, IRBuilder<> &B,
                                              ArrayRef<Value *> Args) {
  if (F->arg_size() != 3) {
    return nullptr;
  }
  Value *FalseVal = Args[0];
  Value *TrueVal = Args[1];
  Value *Cond = Args[2];
  Type *RetTy = F->getReturnType();
  auto *VecRetTy = dyn_cast<FixedVectorType>(RetTy);
  Type *CondEleTy = Cond->getType()->getScalarType();
  unsigned CondEleBits = CondEleTy->getPrimitiveSizeInBits();
  if (VecRetTy) {
    unsigned SimdWidth = VecRetTy->getNumElements();
    Constant *ShiftAmount = ConstantInt::get(CondEleTy, CondEleBits - 1);
    Constant *VecShiftAmount = ConstantVector::getSplat(
        ElementCount::getFixed(SimdWidth), ShiftAmount);
    Value *Mask = B.CreateAShr(Cond, VecShiftAmount);
    Value *TrueValRaw = TrueVal;
    Value *FalseValRaw = FalseVal;
    if (VecRetTy->getElementType()->isFloatingPointTy()) {
      auto *RawType = FixedVectorType::getInteger(VecRetTy);
      TrueValRaw = B.CreateBitCast(TrueVal, RawType);
      FalseValRaw = B.CreateBitCast(FalseVal, RawType);
    }
    Value *Result = B.CreateXor(TrueValRaw, FalseValRaw);
    Result = B.CreateAnd(Result, Mask);
    Result = B.CreateXor(Result, FalseValRaw);
    if (Result->getType() != VecRetTy) {
      Result = B.CreateBitCast(Result, VecRetTy);
    }
    return Result;
  } else {
    Value *Cmp = B.CreateICmpNE(Cond, Constant::getNullValue(CondEleTy));
    return B.CreateSelect(Cmp, TrueVal, FalseVal);
  }
}

/// @brief Emit the body of a builtin function as a call to a binary LLVM
/// intrinsic. If one argument is a scalar type and the other a vector type,
/// the scalar argument is splatted to the vector type.
///
/// @param[in] B Builder used to emit instructions.
/// @param[in] LHS first argument to be passed to the intrinsic.
/// @param[in] RHS second argument to be passed to the intrinsic.
/// @param[in] ID the LLVM intrinsic ID.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineAsLLVMBinaryIntrinsic(
    IRBuilder<> &B, Value *LHS, Value *RHS, llvm::Intrinsic::ID ID) {
  const Triple TT(B.GetInsertBlock()->getModule()->getTargetTriple());
  if (TT.getArch() == Triple::arm || TT.getArch() == Triple::aarch64) {
    // fmin and fmax fail CTS on arm targets.
    // This is a HACK and should be removed when CA-3595 is resolved.
    return nullptr;
  }

  const auto *LHSTy = LHS->getType();
  const auto *RHSTy = RHS->getType();
  if (LHSTy->isVectorTy() != RHSTy->isVectorTy()) {
    auto VectorEC =
        multi_llvm::getVectorElementCount(LHSTy->isVectorTy() ? LHSTy : RHSTy);
    if (!LHS->getType()->isVectorTy()) {
      LHS = B.CreateVectorSplat(VectorEC, LHS);
    }
    if (!RHS->getType()->isVectorTy()) {
      RHS = B.CreateVectorSplat(VectorEC, RHS);
    }
  }
  return B.CreateBinaryIntrinsic(ID, LHS, RHS);
}

/// @brief Emit the body of the 'as_*' builtin function.
///
/// @param[in] F Function to emit the body inline.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineAs(Function *F, llvm::IRBuilder<> &B,
                                          llvm::ArrayRef<Value *> Args) {
  if (Args.size() != 1) {
    return nullptr;
  }
  Value *Src = Args[0];
  Type *SrcTy = Src->getType();
  Type *DstTy = F->getReturnType();
  auto *SrcVecTy = dyn_cast<FixedVectorType>(SrcTy);
  auto *DstVecTy = dyn_cast<FixedVectorType>(DstTy);
  Type *SrcEleTy = SrcVecTy ? SrcVecTy->getElementType() : nullptr;
  Type *DstEleTy = DstVecTy ? DstVecTy->getElementType() : nullptr;
  unsigned SrcEleBits = SrcEleTy ? SrcEleTy->getPrimitiveSizeInBits() : 0;
  unsigned DstEleBits = DstEleTy ? DstEleTy->getPrimitiveSizeInBits() : 0;
  bool SrcDstHaveSameWidth = SrcEleTy && DstEleTy && (SrcEleBits == DstEleBits);
  bool SrcVec3 = SrcVecTy && (SrcVecTy->getNumElements() == 3);
  bool SrcVec4 = SrcVecTy && (SrcVecTy->getNumElements() == 4);
  bool DstVec3 = DstVecTy && (DstVecTy->getNumElements() == 3);
  bool DstVec4 = DstVecTy && (DstVecTy->getNumElements() == 4);
  bool LowerAsShuffle = false;
  if (SrcVec3 && !DstVec3) {
    if (!DstVec4 || !SrcDstHaveSameWidth) {
      return nullptr;
    }
    LowerAsShuffle = true;
  } else if (DstVec3 && !SrcVec3) {
    if (!SrcVec4 || !SrcDstHaveSameWidth) {
      return nullptr;
    }
    LowerAsShuffle = true;
  }

  // Lower some vec3 variants of as_* using vector shuffles.
  if (LowerAsShuffle) {
    SmallVector<Constant *, 4> Indices;
    for (unsigned i = 0; i < DstVecTy->getNumElements(); i++) {
      if (i < SrcVecTy->getNumElements()) {
        Indices.push_back(B.getInt32(i));
      } else {
        Indices.push_back(UndefValue::get(B.getInt32Ty()));
      }
    }
    Value *Mask = ConstantVector::get(Indices);
    Src = B.CreateShuffleVector(Src, UndefValue::get(SrcVecTy), Mask);
  }

  // Common case: as_* is a simple bitcast.
  return B.CreateBitCast(Src, DstTy, "as");
}

/// @brief Emit the body of the 'convert_*' builtin functions.
///
/// @param[in] F the function to emit inline.
/// @param[in] builtinID Builtin ID of the function.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineConvert(Function *F, BuiltinID builtinID,
                                               IRBuilder<> &B,
                                               ArrayRef<Value *> Args) {
  if (Args.size() != 1) {
    return nullptr;
  }
  Type *DstTy = nullptr;
  bool DstIsSigned = false;
  auto &Ctx = B.getContext();
  switch (builtinID) {
    case eCLBuiltinConvertChar:
      DstIsSigned = true;
      LLVM_FALLTHROUGH;
    case eCLBuiltinConvertUChar:
      DstTy = IntegerType::getInt8Ty(Ctx);
      break;
    case eCLBuiltinConvertShort:
      DstIsSigned = true;
      LLVM_FALLTHROUGH;
    case eCLBuiltinConvertUShort:
      DstTy = IntegerType::getInt16Ty(Ctx);
      break;
    case eCLBuiltinConvertInt:
      DstIsSigned = true;
      LLVM_FALLTHROUGH;
    case eCLBuiltinConvertUInt:
      DstTy = IntegerType::getInt32Ty(Ctx);
      break;
    case eCLBuiltinConvertLong:
      DstIsSigned = true;
      LLVM_FALLTHROUGH;
    case eCLBuiltinConvertULong:
      DstTy = IntegerType::getInt64Ty(Ctx);
      break;

    default:
      return nullptr;
  }
  if (!DstTy) {
    return nullptr;
  }

  Value *Src = Args[0];
  bool SrcIsSigned;
  if (Src->getType()->isFloatingPointTy()) {
    // All floating point types are signed
    SrcIsSigned = true;
  } else {
    auto IsParamSignedOrNone = paramHasTypeQual(*F, 0, eTypeQualSignedInt);
    if (!IsParamSignedOrNone) {
      return nullptr;
    }
    SrcIsSigned = *IsParamSignedOrNone;
  }

  auto Opcode = CastInst::getCastOpcode(Src, SrcIsSigned, DstTy, DstIsSigned);
  return B.CreateCast(Opcode, Src, DstTy, "inline_convert");
}

/// @brief Emit the body of the 'vloadN' builtin function.
///
/// @param[in] F Function to emit the body inline.
/// @param[in] Width Number of elements to load.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineVLoad(Function *F, unsigned Width,
                                             IRBuilder<> &B,
                                             ArrayRef<Value *> Args) {
  if (Width < 2) {
    return nullptr;
  }
  (void)F;

  Type *RetTy = F->getReturnType();
  assert(isa<FixedVectorType>(RetTy) && "vloadN must return a vector type");
  Type *EltTy = RetTy->getScalarType();

  Value *Ptr = Args[1];
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  if (!PtrTy) {
    return nullptr;
  }
  auto *DataTy = FixedVectorType::get(EltTy, Width);
  Value *Data = UndefValue::get(DataTy);

  // Emit the base pointer.
  Value *Offset = Args[0];
  IntegerType *OffsetTy = dyn_cast<IntegerType>(Offset->getType());
  if (!OffsetTy) {
    return nullptr;
  }
  Value *Stride = ConstantInt::get(OffsetTy, Width);
  Offset = B.CreateMul(Offset, Stride);
  Value *GEPBase = B.CreateGEP(EltTy, Ptr, Offset, "vload_base");

  if (Width == 3) {
    for (unsigned i = 0; i < Width; i++) {
      Value *Index = B.getInt32(i);
      Value *GEP = B.CreateGEP(EltTy, GEPBase, Index);
      Value *Lane = B.CreateLoad(EltTy, GEP, false, "vload");
      Data = B.CreateInsertElement(Data, Lane, Index, "vload_insert");
    }
  } else {
    PointerType *VecPtrTy = DataTy->getPointerTo(PtrTy->getAddressSpace());
    Value *VecBase = B.CreateBitCast(GEPBase, VecPtrTy, "vload_ptr");
    auto *Load = B.CreateLoad(DataTy, VecBase, false, "vload");

    unsigned Align = DataTy->getScalarSizeInBits() / 8;
    Load->setAlignment(MaybeAlign(Align).valueOrOne());
    Data = Load;
  }

  return Data;
}

/// @brief Emit the body of the 'vstoreN' builtin function.
///
/// @param[in] F Function to emit the body inline.
/// @param[in] Width Number of elements to store.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineVStore(Function *F, unsigned Width,
                                              IRBuilder<> &B,
                                              ArrayRef<Value *> Args) {
  if (Width < 2) {
    return nullptr;
  }
  (void)F;

  Value *Data = Args[0];
  auto *VecDataTy = dyn_cast<FixedVectorType>(Data->getType());
  if (!VecDataTy || (VecDataTy->getNumElements() != Width)) {
    return nullptr;
  }

  Value *Ptr = Args[2];
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  if (!PtrTy) {
    return nullptr;
  }

  // Emit the base pointer.
  Value *Offset = Args[1];
  IntegerType *OffsetTy = dyn_cast<IntegerType>(Offset->getType());
  if (!OffsetTy) {
    return nullptr;
  }
  Value *Stride = ConstantInt::get(OffsetTy, Width);
  Offset = B.CreateMul(Offset, Stride);
  Value *GEPBase =
      B.CreateGEP(VecDataTy->getElementType(), Ptr, Offset, "vstore_base");

  // Emit store(s).
  StoreInst *Store = nullptr;
  if (Width == 3) {
    for (unsigned i = 0; i < Width; i++) {
      Value *Index = B.getInt32(i);
      Value *Lane = B.CreateExtractElement(Data, Index, "vstore_extract");
      Value *GEP = B.CreateGEP(VecDataTy->getElementType(), GEPBase, Index);
      Store = B.CreateStore(Lane, GEP, false);
    }
  } else {
    PointerType *VecPtrTy = VecDataTy->getPointerTo(PtrTy->getAddressSpace());
    Value *VecBase = B.CreateBitCast(GEPBase, VecPtrTy, "vstore_ptr");
    Store = B.CreateStore(Data, VecBase, false);

    unsigned Align = VecDataTy->getScalarSizeInBits() / 8;
    Store->setAlignment(MaybeAlign(Align).valueOrOne());
  }
  return Store;
}

/// @brief Emit the body of the 'vload_half' builtin function.
///
/// @param[in] F Function to emit the body inline.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineVLoadHalf(Function *F, IRBuilder<> &B,
                                                 ArrayRef<Value *> Args) {
  if (F->getType()->isVectorTy()) {
    return nullptr;
  }

  // Cast the pointer to ushort*.
  Value *Ptr = Args[1];
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  if (!PtrTy) {
    return nullptr;
  }
  Type *U16Ty = B.getInt16Ty();
  Type *U16PtrTy = PointerType::get(U16Ty, PtrTy->getAddressSpace());
  Value *DataPtr = B.CreateBitCast(Ptr, U16PtrTy);

  // Emit the base pointer.
  Value *Offset = Args[0];
  DataPtr = B.CreateGEP(U16Ty, DataPtr, Offset, "vload_base");

  // Load a ushort.
  Value *Data = B.CreateLoad(B.getInt16Ty(), DataPtr, "vload_half");

  // Declare the conversion builtin.
  Module *M = F->getParent();
  Function *HalfToFloatFn =
      declareBuiltin(M, eCLBuiltinConvertHalfToFloat, B.getFloatTy(),
                     {B.getInt16Ty()}, {eTypeQualNone});
  if (!HalfToFloatFn) {
    return nullptr;
  }

  // Convert it to float.
  CallInst *CI = CreateBuiltinCall(B, HalfToFloatFn, {Data});
  CI->setCallingConv(F->getCallingConv());

  return CI;
}

/// @brief Emit the body of the 'vstore_half' builtin function.
///
/// @param[in] F Function to emit the body inline.
/// @param[in] Mode Rounding mode to use, e.g. '_rte'.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineVStoreHalf(Function *F, StringRef Mode,
                                                  IRBuilder<> &B,
                                                  ArrayRef<Value *> Args) {
  Value *Data = Args[0];
  if (!Data || Data->getType()->isVectorTy()) {
    return nullptr;
  }

  // Declare the conversion builtin.
  BuiltinID ConvID;

  if (Data->getType() == B.getFloatTy()) {
    ConvID = StringSwitch<BuiltinID>(Mode)
                 .Case("", eCLBuiltinConvertFloatToHalf)
                 .Case("_rte", eCLBuiltinConvertFloatToHalfRte)
                 .Case("_rtz", eCLBuiltinConvertFloatToHalfRtz)
                 .Case("_rtp", eCLBuiltinConvertFloatToHalfRtp)
                 .Case("_rtn", eCLBuiltinConvertFloatToHalfRtn)
                 .Default(eBuiltinInvalid);
  } else {
    ConvID = StringSwitch<BuiltinID>(Mode)
                 .Case("", eCLBuiltinConvertDoubleToHalf)
                 .Case("_rte", eCLBuiltinConvertDoubleToHalfRte)
                 .Case("_rtz", eCLBuiltinConvertDoubleToHalfRtz)
                 .Case("_rtp", eCLBuiltinConvertDoubleToHalfRtp)
                 .Case("_rtn", eCLBuiltinConvertDoubleToHalfRtn)
                 .Default(eBuiltinInvalid);
  }
  if (ConvID == eBuiltinInvalid) {
    return nullptr;
  }
  Module *M = F->getParent();

  // Normally, the vstore_half functions take the number to store as a float.
  // However, if the double extension is enabled, it is also possible to use
  // double instead. This means that we might have to convert either a float or
  // a double to a half.
  Function *FloatToHalfFn = declareBuiltin(M, ConvID, B.getInt16Ty(),
                                           {Data->getType()}, {eTypeQualNone});
  if (!FloatToHalfFn) {
    return nullptr;
  }

  // Convert the data from float/double to half.
  CallInst *CI = CreateBuiltinCall(B, FloatToHalfFn, {Data});
  CI->setCallingConv(F->getCallingConv());
  Data = CI;

  // Cast the pointer to ushort*.
  Value *Ptr = Args[2];
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  if (!PtrTy) {
    return nullptr;
  }
  auto U16Ty = B.getInt16Ty();
  Type *U16PtrTy = PointerType::get(U16Ty, PtrTy->getAddressSpace());
  Value *DataPtr = B.CreateBitCast(Ptr, U16PtrTy);

  // Emit the base pointer.
  Value *Offset = Args[1];
  DataPtr = B.CreateGEP(U16Ty, DataPtr, Offset, "vstore_base");

  // Store the ushort.
  return B.CreateStore(Data, DataPtr, "vstore_half");
}

/// @brief Emit the body of a relational builtin function.
///
/// This function handles relational builtins that accept two arguments, such as
/// the comparison builtins.
///
/// @param[in] BuiltinID Identifier of the builtin to emit the body inline.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineRelationalsWithTwoArguments(
    BuiltinID BuiltinID, IRBuilder<> &B, ArrayRef<Value *> Args) {
  CmpInst::Predicate Pred = CmpInst::FCMP_FALSE;
  CmpInst::Predicate Pred2 = CmpInst::FCMP_FALSE;
  switch (BuiltinID) {
    default:
      return nullptr;
    case eCLBuiltinIsEqual:
      Pred = CmpInst::FCMP_OEQ;
      break;
    case eCLBuiltinIsNotEqual:
      Pred = CmpInst::FCMP_UNE;
      break;
    case eCLBuiltinIsGreater:
      Pred = CmpInst::FCMP_OGT;
      break;
    case eCLBuiltinIsGreaterEqual:
      Pred = CmpInst::FCMP_OGE;
      break;
    case eCLBuiltinIsLess:
      Pred = CmpInst::FCMP_OLT;
      break;
    case eCLBuiltinIsLessEqual:
      Pred = CmpInst::FCMP_OLE;
      break;
    case eCLBuiltinIsLessGreater:
      Pred = CmpInst::FCMP_OLT;
      Pred2 = CmpInst::FCMP_OGT;
      break;
    case eCLBuiltinIsOrdered:
      Pred = CmpInst::FCMP_ORD;
      break;
    case eCLBuiltinIsUnordered:
      Pred = CmpInst::FCMP_UNO;
      break;
  }

  if (Args.size() != 2) {
    return nullptr;
  }
  Value *Src0 = Args[0], *Src1 = Args[1];
  Value *Cmp = B.CreateFCmp(Pred, Src0, Src1, "relational");

  Type *ResultEleTy = nullptr;
  Type *Src0Ty = Src0->getType();
  if (Src0->getType() == B.getDoubleTy()) {
    // Special case because relational(doubleN, doubleN) returns longn while
    // relational(double, double) returns int.
    if (Src0Ty->isVectorTy()) {
      ResultEleTy = B.getInt64Ty();
    } else {
      ResultEleTy = B.getInt32Ty();
    }
  } else if (Src0->getType() == B.getHalfTy()) {
    // Special case because relational(HalfTyN, HalfTyN) returns i16 while
    // relational(HalfTy, HalfTy) returns int.
    if (Src0Ty->isVectorTy()) {
      ResultEleTy = B.getInt16Ty();
    } else {
      ResultEleTy = B.getInt32Ty();
    }
  } else {
    // All the other cases can be handled here.
    ResultEleTy = B.getIntNTy(Src0->getType()->getScalarSizeInBits());
  }
  Value *Result = nullptr;
  auto *SrcVecTy = dyn_cast<FixedVectorType>(Src0->getType());
  if (SrcVecTy) {
    auto *ResultVecTy =
        FixedVectorType::get(ResultEleTy, SrcVecTy->getNumElements());
    Result = B.CreateSExt(Cmp, ResultVecTy, "relational");
  } else {
    Result = B.CreateZExt(Cmp, ResultEleTy, "relational");
  }

  if (Pred2 != CmpInst::FCMP_FALSE) {
    Value *Cmp2 = B.CreateFCmp(Pred2, Src0, Src1, "relational");
    Value *True = SrcVecTy ? Constant::getAllOnesValue(Result->getType())
                           : ConstantInt::get(Result->getType(), 1);
    Result = B.CreateSelect(Cmp2, True, Result);
  }

  return Result;
}

/// @brief Emit the body of a relational builtin function.
///
/// This function handles relational builtins that accept a single argument,
/// such as the builtins checking if the argument is infinite or not.
///
/// @param[in] BuiltinID Identifier of the builtin to emit the body inline.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Arg Argument passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineRelationalsWithOneArgument(
    BuiltinID BuiltinID, IRBuilder<> &B, Value *Arg) {
  Value *Result = nullptr;
  // The types (and misc info) that we will be using
  Type *ArgTy = Arg->getType();
  const bool isVectorTy = ArgTy->isVectorTy();
  const unsigned Width =
      isVectorTy ? multi_llvm::getVectorNumElements(ArgTy) : 0;
  Type *ArgEleTy = isVectorTy ? multi_llvm::getVectorElementType(ArgTy) : ArgTy;
  Type *SignedTy = ArgEleTy == B.getFloatTy() ? B.getInt32Ty() : B.getInt64Ty();
  Type *ReturnTy = (ArgEleTy == B.getDoubleTy() && isVectorTy) ? B.getInt64Ty()
                                                               : B.getInt32Ty();

  if (ArgEleTy != B.getFloatTy() && ArgEleTy != B.getDoubleTy()) {
    return nullptr;
  }
  // Create all the masks we are going to be using
  Constant *ExponentMask = nullptr;
  Constant *MantissaMask = nullptr;
  Constant *NonSignMask = nullptr;
  Constant *Zero = nullptr;
  if (ArgEleTy == B.getFloatTy()) {
    ExponentMask = B.getInt32(0x7F800000u);
    MantissaMask = B.getInt32(0x007FFFFFu);
    NonSignMask = B.getInt32(0x7FFFFFFFu);
    Zero = B.getInt32(0u);
  } else if (ArgEleTy == B.getDoubleTy()) {
    ExponentMask = B.getInt64(0x7FF0000000000000u);
    MantissaMask = B.getInt64(0x000FFFFFFFFFFFFFu);
    NonSignMask = B.getInt64(0x7FFFFFFFFFFFFFFFu);
    Zero = B.getInt64(0u);
  }

  // For the vector versions, we need to create vector types and values
  if (isVectorTy) {
    SignedTy = FixedVectorType::get(SignedTy, Width);
    ReturnTy = FixedVectorType::get(ReturnTy, Width);
    auto const EC = ElementCount::getFixed(Width);
    ExponentMask = ConstantVector::getSplat(EC, ExponentMask);
    MantissaMask = ConstantVector::getSplat(EC, MantissaMask);
    NonSignMask = ConstantVector::getSplat(EC, NonSignMask);
    Zero = ConstantVector::getSplat(EC, Zero);
  }

  // We will be needing access to the argument as an integer (bitcast) value
  Value *STArg = B.CreateBitCast(Arg, SignedTy);

  // Emit the IR that will calculate the result
  switch (BuiltinID) {
    default:
      llvm_unreachable("Invalid Builtin ID");
      break;
    case eCLBuiltinIsFinite:
      Result = B.CreateAnd(STArg, NonSignMask);
      Result = B.CreateICmpSLT(Result, ExponentMask);
      break;
    case eCLBuiltinIsInf:
      Result = B.CreateAnd(STArg, NonSignMask);
      Result = B.CreateICmpEQ(Result, ExponentMask);
      break;
    case eCLBuiltinIsNan: {
      Result = B.CreateAnd(STArg, NonSignMask);
      // This checks if the exponent is all ones (the same as the ExponentMask)
      // and also if the significant (the mantissa) is not zero. If the mantissa
      // is zero then it would be infinite, not NaN.
      Value *ExponentAllOnes =
          B.CreateICmpEQ(ExponentMask, B.CreateAnd(ExponentMask, Result));
      Value *MantissaNotZero =
          B.CreateICmpSGT(B.CreateAnd(MantissaMask, Result), Zero);
      Result = B.CreateAnd(ExponentAllOnes, MantissaNotZero);
      break;
    }
    case eCLBuiltinIsNormal: {
      Result = B.CreateAnd(STArg, NonSignMask);
      Value *ExponentBitsNotAllSet = B.CreateICmpSLT(Result, ExponentMask);
      Value *ExponentBitsNonZero = B.CreateICmpSGT(Result, MantissaMask);
      Result = B.CreateAnd(ExponentBitsNotAllSet, ExponentBitsNonZero);
      break;
    }
    case eCLBuiltinSignBit:
      Result = B.CreateICmpSLT(STArg, Zero);
      break;
  }

  // Convert the i1 result from the comparison instruction to the type that the
  // builtin returns
  if (isVectorTy) {
    // 0 for false, -1 (all 1s) for true
    Result = B.CreateSExt(Result, ReturnTy);
  } else {
    // 0 for false, 1 for true
    Result = B.CreateZExt(Result, ReturnTy);
  }

  return Result;
}

/// @brief Emit the body of a vector shuffle builtin function.
///
/// @param[in] BuiltinID Identifier of the builtin to emit the body inline.
/// @param[in] B Builder used to emit instructions.
/// @param[in] Args Arguments passed to the function.
///
/// @return Value returned by the builtin implementation or null on failure.
Value *CLBuiltinInfo::emitBuiltinInlineShuffle(BuiltinID BuiltinID,
                                               IRBuilder<> &B,
                                               ArrayRef<Value *> Args) {
  // Make sure we have the correct number of arguments.
  assert(((BuiltinID == eCLBuiltinShuffle && Args.size() == 2) ||
          (BuiltinID == eCLBuiltinShuffle2 && Args.size() == 3)) &&
         "Wrong number of arguments!");

  // It is not worth splitting shuffle and shuffle2 into two functions as a lot
  // of the code is the same.
  const bool isShuffle2 = (BuiltinID == eCLBuiltinShuffle2);

  // Get the mask and the mask type.
  Value *Mask = Args[isShuffle2 ? 2 : 1];
  auto MaskVecTy = cast<FixedVectorType>(Mask->getType());
  IntegerType *MaskTy = cast<IntegerType>(MaskVecTy->getElementType());
  const int MaskWidth = MaskVecTy->getNumElements();

  // TODO: Support non-constant masks (in a less efficient way)
  if (!isa<Constant>(Mask)) {
    return nullptr;
  }

  // We need to mask the mask elements, since the OpenCL standard specifies that
  // we should only take the ilogb(2N-1)+1 least significant bits from each mask
  // element into consideration, where N the number of elements in the vector
  // according to vec_step.
  auto ShuffleTy = cast<FixedVectorType>(Args[0]->getType());
  const int Width = ShuffleTy->getNumElements();
  // Vectors for size 3 are not supported by the shuffle builtin.
  assert(Width != 3 && "Invalid vector width of 3!");
  const int N = (Width == 3 ? 4 : Width);
  const int SignificantBits =
      stdcompat::ilogb(2 * N - 1) + (isShuffle2 ? 1 : 0);
  const unsigned BitMask = ~((~0u) << SignificantBits);
  Value *BitMaskV = ConstantVector::getSplat(ElementCount::getFixed(MaskWidth),
                                             ConstantInt::get(MaskTy, BitMask));
  // The builtin's mask may have different integer types, while the LLVM
  // instruction only supports i32.
  // Mask the mask.
  Value *MaskedMask = B.CreateAnd(Mask, BitMaskV, "mask");
  MaskedMask = B.CreateIntCast(
      MaskedMask, FixedVectorType::get(B.getInt32Ty(), MaskWidth), false);

  // Create the shufflevector instruction.
  Value *Arg1 = (isShuffle2 ? Args[1] : UndefValue::get(ShuffleTy));
  return B.CreateShuffleVector(Args[0], Arg1, MaskedMask, "shuffle");
}

Value *CLBuiltinInfo::emitBuiltinInlinePrintf(BuiltinID, IRBuilder<> &B,
                                              ArrayRef<Value *> Args) {
  Module &M = *(B.GetInsertBlock()->getModule());

  // Declare printf if needed.
  Function *Printf = M.getFunction("printf");
  if (!Printf) {
    PointerType *PtrTy = PointerType::getUnqual(B.getInt8Ty());
    FunctionType *PrintfTy = FunctionType::get(B.getInt32Ty(), {PtrTy}, true);
    Printf =
        Function::Create(PrintfTy, GlobalValue::ExternalLinkage, "printf", &M);
    Printf->setCallingConv(CallingConv::SPIR_FUNC);
  }

  return CreateBuiltinCall(B, Printf, Args);
}

multi_llvm::Optional<ConstantRange> CLBuiltinInfo::getBuiltinRange(
    CallInst &CI, std::array<multi_llvm::Optional<uint64_t>, 3> MaxLocalSizes,
    std::array<multi_llvm::Optional<uint64_t>, 3> MaxGlobalSizes) const {
  auto *F = CI.getCalledFunction();
  if (!F || !F->hasName() || !CI.getType()->isIntegerTy()) {
    return multi_llvm::None;
  }

  BuiltinID BuiltinID = identifyBuiltin(*F);

  auto Bits = CI.getType()->getIntegerBitWidth();
  // Assume we're indexing the global sizes array.
  std::array<multi_llvm::Optional<uint64_t>, 3> *SizesPtr = &MaxGlobalSizes;

  switch (BuiltinID) {
    default:
      return multi_llvm::None;
    case eCLBuiltinGetWorkDim:
      return ConstantRange::getNonEmpty(APInt(Bits, 1), APInt(Bits, 4));
    case eCLBuiltinGetLocalId:
    case eCLBuiltinGetLocalSize:
      // Use the local sizes array, and fall through to common handling.
      SizesPtr = &MaxLocalSizes;
      LLVM_FALLTHROUGH;
    case eCLBuiltinGetGlobalSize: {
      auto *DimIdx = CI.getOperand(0);
      if (!isa<ConstantInt>(DimIdx)) {
        return multi_llvm::None;
      }
      uint64_t DimVal = cast<ConstantInt>(DimIdx)->getZExtValue();
      if (DimVal >= SizesPtr->size() || !(*SizesPtr)[DimVal]) {
        return multi_llvm::None;
      }
      // ID builtins range from [0,size) and size builtins from [1,size]. Thus
      // offset the range by 1 at each low/high end when returning the range
      // for a size builtin.
      int const SizeAdjust = BuiltinID == eCLBuiltinGetLocalSize ||
                             BuiltinID == eCLBuiltinGetGlobalSize;
      return ConstantRange::getNonEmpty(
          APInt(Bits, SizeAdjust),
          APInt(Bits, *(*SizesPtr)[DimVal] + SizeAdjust));
    }
  }
}

// Must be kept in sync with our OpenCL headers!
enum : uint32_t {
  CLK_LOCAL_MEM_FENCE = 1,
  CLK_GLOBAL_MEM_FENCE = 2,
  // FIXME: We don't support image fences in our headers
};

// Must be kept in sync with our OpenCL headers!
enum : uint32_t {
  memory_scope_work_item = 1,
  memory_scope_sub_group = 2,
  memory_scope_work_group = 3,
  memory_scope_device = 4,
  memory_scope_all_svm_devices = 5,
  memory_scope_all_devices = 6,
};

// Must be kept in sync with our OpenCL headers!
enum : uint32_t {
  memory_order_relaxed = 0,
  memory_order_acquire = 1,
  memory_order_release = 2,
  memory_order_acq_rel = 3,
  memory_order_seq_cst = 4,
};

static multi_llvm::Optional<unsigned> parseMemFenceFlagsParam(Value *const P) {
  // Grab the 'flags' parameter.
  if (auto *const Flags = dyn_cast<ConstantInt>(P)) {
    // cl_mem_fence_flags is a bitfield and can be 0 or a combination of
    // CLK_(GLOBAL|LOCAL|IMAGE)_MEM_FENCE values ORed together.
    switch (Flags->getZExtValue()) {
      case 0:
        return multi_llvm::None;
      case CLK_LOCAL_MEM_FENCE:
        return BIMuxInfoConcept::MemSemanticsWorkGroupMemory;
      case CLK_GLOBAL_MEM_FENCE:
        return BIMuxInfoConcept::MemSemanticsCrossWorkGroupMemory;
      case CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE:
        return (BIMuxInfoConcept::MemSemanticsWorkGroupMemory |
                BIMuxInfoConcept::MemSemanticsCrossWorkGroupMemory);
    }
  }
  return multi_llvm::None;
}

static multi_llvm::Optional<unsigned> parseMemoryScopeParam(Value *const P) {
  if (auto *const Scope = dyn_cast<ConstantInt>(P)) {
    switch (Scope->getZExtValue()) {
      case memory_scope_work_item:
        return BIMuxInfoConcept::MemScopeWorkItem;
      case memory_scope_sub_group:
        return BIMuxInfoConcept::MemScopeSubGroup;
      case memory_scope_work_group:
        return BIMuxInfoConcept::MemScopeWorkGroup;
      case memory_scope_device:
        return BIMuxInfoConcept::MemScopeDevice;
      // 3.3.5. memory_scope_all_devices is an alias for
      // memory_scope_all_svm_devices.
      case memory_scope_all_devices:
      case memory_scope_all_svm_devices:
        return BIMuxInfoConcept::MemScopeCrossDevice;
    }
  }
  return multi_llvm::None;
}

static multi_llvm::Optional<unsigned> parseMemoryOrderParam(Value *const P) {
  if (auto *const Order = dyn_cast<ConstantInt>(P)) {
    switch (Order->getZExtValue()) {
      case memory_order_relaxed:
        return BIMuxInfoConcept::MemSemanticsRelaxed;
      case memory_order_acquire:
        return BIMuxInfoConcept::MemSemanticsAcquire;
      case memory_order_release:
        return BIMuxInfoConcept::MemSemanticsRelease;
      case memory_order_acq_rel:
        return BIMuxInfoConcept::MemSemanticsAcquireRelease;
      case memory_order_seq_cst:
        return BIMuxInfoConcept::MemSemanticsSequentiallyConsistent;
    }
  }
  return multi_llvm::None;
}

CallInst *CLBuiltinInfo::mapSyncBuiltinToMuxSyncBuiltin(
    CallInst &CI, BIMuxInfoConcept &BIMuxImpl) {
  auto &M = *CI.getModule();
  auto *const F = CI.getCalledFunction();
  assert(F && "No calling function?");
  auto const ID = identifyBuiltin(*F);

  auto *const I32Ty = Type::getInt32Ty(M.getContext());

  auto CtrlBarrierID = eMuxBuiltinWorkGroupBarrier;
  unsigned DefaultMemScope = BIMuxInfoConcept::MemScopeWorkGroup;
  unsigned DefaultMemOrder =
      BIMuxInfoConcept::MemSemanticsSequentiallyConsistent;

  switch (ID) {
    default:
      return nullptr;
    case eCLBuiltinSubGroupBarrier:
      CtrlBarrierID = eMuxBuiltinSubGroupBarrier;
      DefaultMemScope = BIMuxInfoConcept::MemScopeSubGroup;
      LLVM_FALLTHROUGH;
    case eCLBuiltinBarrier:
    case eCLBuiltinWorkGroupBarrier: {
      // Memory Scope which the barrier controls. Defaults to 'workgroup' or
      // 'subgroup' scope depending on the barrier, but sub_group_barrier and
      // work_group_barrier can optionally provide a scope.
      unsigned ScopeVal = DefaultMemScope;
      if ((ID == eCLBuiltinSubGroupBarrier ||
           ID == eCLBuiltinWorkGroupBarrier) &&
          F->arg_size() == 2) {
        if (auto Scope = parseMemoryScopeParam(CI.getOperand(1))) {
          ScopeVal = *Scope;
        }
      }

      unsigned SemanticsVal =
          DefaultMemOrder |
          parseMemFenceFlagsParam(CI.getOperand(0)).value_or(0);

      auto *const CtrlBarrier =
          BIMuxImpl.getOrDeclareMuxBuiltin(CtrlBarrierID, M);

      auto *const BarrierID = ConstantInt::get(I32Ty, 0);
      auto *const Scope = ConstantInt::get(I32Ty, ScopeVal);
      auto *const Semantics = ConstantInt::get(I32Ty, SemanticsVal);
      auto *const NewCI = CallInst::Create(
          CtrlBarrier, {BarrierID, Scope, Semantics}, CI.getName(), &CI);
      NewCI->setAttributes(CtrlBarrier->getAttributes());
      return NewCI;
    }
    case eCLBuiltinAtomicWorkItemFence:
      // atomic_work_item_fence has two parameters which we can parse.
      DefaultMemOrder =
          parseMemoryOrderParam(CI.getOperand(1)).value_or(DefaultMemOrder);
      DefaultMemScope =
          parseMemoryScopeParam(CI.getOperand(2)).value_or(DefaultMemScope);
      LLVM_FALLTHROUGH;
    case eCLBuiltinMemFence:
    case eCLBuiltinReadMemFence:
    case eCLBuiltinWriteMemFence: {
      // The deprecated 'fence' builtins default to memory_scope_work_group and
      // have one possible order each.
      if (ID == eCLBuiltinMemFence) {
        DefaultMemOrder = BIMuxInfoConcept::MemSemanticsAcquireRelease;
      } else if (ID == eCLBuiltinReadMemFence) {
        DefaultMemOrder = BIMuxInfoConcept::MemSemanticsAcquire;
      } else if (ID == eCLBuiltinWriteMemFence) {
        DefaultMemOrder = BIMuxInfoConcept::MemSemanticsRelease;
      }
      unsigned SemanticsVal =
          DefaultMemOrder |
          parseMemFenceFlagsParam(CI.getOperand(0)).value_or(0);
      auto *const MemBarrier =
          BIMuxImpl.getOrDeclareMuxBuiltin(eMuxBuiltinMemBarrier, M);
      auto *const Scope = ConstantInt::get(I32Ty, DefaultMemScope);
      auto *const Semantics = ConstantInt::get(I32Ty, SemanticsVal);
      auto *const NewCI =
          CallInst::Create(MemBarrier, {Scope, Semantics}, CI.getName(), &CI);
      NewCI->setAttributes(MemBarrier->getAttributes());
      return NewCI;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

Function *CLBuiltinLoader::materializeBuiltin(StringRef BuiltinName,
                                              Module *DestM,
                                              BuiltinMatFlags Flags) {
  auto *const BuiltinModule = this->getBuiltinsModule();

  // Retrieve it from the builtin module.
  if (!BuiltinModule) {
    return nullptr;
  }
  Function *SrcBuiltin = BuiltinModule->getFunction(BuiltinName);
  if (!SrcBuiltin) {
    return nullptr;
  }

  // The user only wants a declaration.
  if (!(Flags & eBuiltinMatDefinition)) {
    if (!DestM) {
      return SrcBuiltin;
    } else {
      FunctionType *FT = dyn_cast<FunctionType>(SrcBuiltin->getFunctionType());
      Function *BuiltinDecl = cast<Function>(
          DestM->getOrInsertFunction(BuiltinName, FT).getCallee());
      BuiltinDecl->copyAttributesFrom(SrcBuiltin);
      BuiltinDecl->setCallingConv(SrcBuiltin->getCallingConv());
      return BuiltinDecl;
    }
  }

  // Materialize the builtin and its callees.
  std::set<Function *> Callees;
  std::vector<Function *> Worklist;
  Worklist.push_back(SrcBuiltin);
  while (!Worklist.empty()) {
    // Materialize the first function in the work list.
    Function *Current = Worklist.front();
    Worklist.erase(Worklist.begin());
    if (!Callees.insert(Current).second) {
      continue;
    }
    if (!BuiltinModule->materialize(Current)) {
      return nullptr;
    }

    // Find any callees in the function and add them to the list.
    for (BasicBlock &BB : *Current) {
      for (Instruction &I : BB) {
        CallInst *CI = dyn_cast<CallInst>(&I);
        if (!CI) {
          continue;
        }
        Function *callee = CI->getCalledFunction();
        if (!callee) {
          continue;
        }
        Worklist.push_back(callee);
      }
    }
  }

  if (!DestM) {
    return SrcBuiltin;
  }

  // Copy builtin and callees to the target module if requested by the user.
  ValueToValueMapTy ValueMap;
  SmallVector<ReturnInst *, 4> Returns;
  // Avoid linking errors.
  GlobalValue::LinkageTypes Linkage = GlobalValue::LinkOnceAnyLinkage;

  // Declare the callees in the module if they don't already exist.
  for (Function *Callee : Callees) {
    Function *NewCallee = DestM->getFunction(Callee->getName());
    if (!NewCallee) {
      FunctionType *FT = Callee->getFunctionType();
      NewCallee = Function::Create(FT, Linkage, Callee->getName(), DestM);
    } else {
      NewCallee->setLinkage(Linkage);
    }
    Function::arg_iterator NewArgI = NewCallee->arg_begin();
    for (Argument &Arg : Callee->args()) {
      NewArgI->setName(Arg.getName());
      ValueMap[&Arg] = &*(NewArgI++);
    }
    NewCallee->copyAttributesFrom(Callee);
    ValueMap[Callee] = NewCallee;
  }

  // Clone the callees' bodies into the module.
  GlobalValueMaterializer Materializer(*DestM);
  for (Function *Callee : Callees) {
    if (Callee->isDeclaration()) {
      continue;
    }
    Function *NewCallee = cast<Function>(ValueMap[Callee]);
    assert(DestM);
    const auto CloneType = DestM == Callee->getParent()
                               ? CloneFunctionChangeType::LocalChangesOnly
                               : CloneFunctionChangeType::DifferentModule;
    multi_llvm::CloneFunctionInto(NewCallee, Callee, ValueMap, CloneType,
                                  Returns, "", nullptr, nullptr, &Materializer);
    Returns.clear();
  }

  // Clone global variable initializers.
  for (GlobalVariable *var : Materializer.variables()) {
    GlobalVariable *newVar = dyn_cast_or_null<GlobalVariable>(ValueMap[var]);
    if (!newVar) {
      return nullptr;
    }
    Constant *oldInit = var->getInitializer();
    Constant *newInit = MapValue(oldInit, ValueMap);
    newVar->setInitializer(newInit);
  }

  return cast<Function>(ValueMap[SrcBuiltin]);
}
}  // namespace utils
}  // namespace compiler
