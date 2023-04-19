// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Helper functions for working with sub_group and work_group functions.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_GROUP_COLLECTIVE_HELPERS_H_INCLUDED
#define COMPILER_UTILS_GROUP_COLLECTIVE_HELPERS_H_INCLUDED

#include <llvm/ADT/Optional.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/optional_helper.h>

namespace llvm {
class Constant;
class Function;
class Type;
}  // namespace llvm

namespace compiler {
namespace utils {
/// @brief Utility function for retrieving the neutral value of a
/// reduction/scan operation. A neutral value is one that does not affect the
/// result of a given operation, e.g., adding 0 or multiplying by 1.
///
/// @param[in] Kind The kind of scan/reduction operation
/// @param[in] Ty The type of the returned neutral value. Must match the type
/// assumed by @a Kind, e.g., a floating-point type for floating-point
/// operations.
///
/// @return The neutral value, or nullptr if unhandled.
llvm::Constant *getNeutralVal(llvm::RecurKind Kind, llvm::Type *Ty);

/// @brief Utility function for retrieving the identity value of a
/// reduction/scan operation. The identity value is one that is expected to be
/// found in the first element of an exclusive scan. It is equal to the neutral
/// value (see @ref getNeutralVal) in all cases except in floating-point
/// min/max, where -INF/+INF is the expected identity and in floating-point
/// addition, where 0.0 (not -0.0 which is the neutral value) is the expected
/// identity.
///
/// @param[in] Kind The kind of scan/reduction operation
/// @param[in] Ty The type of the returned neutral value. Must match the type
/// assumed by @a Kind, e.g., a floating-point type for floating-point
/// operations.
///
/// @return The neutral value, or nullptr if unhandled.
llvm::Constant *getIdentityVal(llvm::RecurKind Kind, llvm::Type *Ty);

/// @brief Represents a work-group or sub-group collective operation.
struct GroupCollective {
  /// @brief The different operation types a group collective can represent.
  enum class Op {
    None,
    All,
    Any,
    Reduction,
    ScanInclusive,
    ScanExclusive,
    Broadcast
  };

  /// @brief The possible scopes of a group collective.
  enum class Scope { None, WorkGroup, SubGroup };

  /// @brief The operation type of the group collective.
  Op op = Op::None;
  /// @brief The scope of the group collective operation.
  Scope scope = Scope::None;
  /// @brief The llvm recurrence operation this can be mapped to. For broadcasts
  /// this will be None.
  llvm::RecurKind recurKind = llvm::RecurKind::None;
  /// @brief The llvm function body for this group collective instance.
  llvm::Function *func = nullptr;
  /// @brief The type the group operation is applied to. Will always be the
  /// type of the first argument of `func`.
  llvm::Type *type = nullptr;
  /// @brief True if the operation is logical, rather than bitwise.
  bool isLogical = false;
  /// @brief Returns true for Any/All type collective operations.
  bool isAnyAll() const { return op == Op::Any || op == Op::All; }
};

/// @brief Helper function to parse a group collective operation.
///
/// TODO: This function is similar to isSubgroupScan defined in
/// `vectorization_context.cpp`, we should consider merging the two.
///
/// @param[in] f Function to parse.
///
/// @return Optional value which may be populated with a GroupCollective
/// instance. If `f` is a sub-group function or work-group collective a value
/// will be returned otherwise return value will be empty.
multi_llvm::Optional<GroupCollective> isGroupCollective(llvm::Function *f);
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_GROUP_COLLECTIVE_HELPERS_H_INCLUDED
