# Refactor BuiltinID:

Upgrade guidance:
* `BuiltinID` enums specific to OpenCL and DXIL have been hidden from the API,
  and most functions accepting or returning BuiltinIDs now use the new `Builtin`
  and `BuiltinCall` structs to hold all the required information. Changes:
  * `BuiltinInfo::analyzeBuiltin()` now returns a `Builtin` struct containing
    both the builtin properties and the builtin ID. Use of language-specific IDs
    is discouraged outside of the implementations themselves. Mux builtin IDs
    are still exposed.
  * `BuiltinInfo::identifyBuiltin()` has been removed from the API. Use
    `BuiltinInfo::analyzeBuiltin()` instead and obtain the ID from the `Builtin`
    struct returned.
  * `BuiltinInfo::isBuiltinUniform()` has been removed from the BuiltinInfo API
    (although it is still present on the `BILangInfoConcept`). Use
    `BuiltinInfo::analyzeBuiltinCall()` to get the uniformity instead.
  * The following property queries have been removed and replaced by flags in
    the `BuiltinProperties` enum:
    * `BuiltinInfo::isBuiltinAtomic()` replaced by `eBuiltinPropertyAtomic`
    * `BuiltinInfo::isRematerializableBuiltinID()` replaced by
      `eBuiltinPropertyRematerializable`
  * Functions formerly accepting a `BuiltinID` now accept a `Builtin const &`.
  * The static functions `BuiltinInfo::getInvalidBuiltin()` and
    `BuiltinInfo::getUnknownBuiltin()` have been removed. Implementations can use
    the corresponding enums directly. In addition, the `Builtin` struct has
    `isValid()` and `isUnknown()` member functions, for convenience.

Non-functional changes:
* Refactor of BuiltinInfo API to remove exposure of internal BuiltinIDs and
  create a universal way to pass builtin information between BuiltinInfo
  functions instead of having to use IDs in some places and function pointers
  in others.
