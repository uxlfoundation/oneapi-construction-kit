# BuiltinInfo interface tidy up:

Upgrade guidance:
* Customers will need to take note of changes to the BuiltinInfo interface, if
  they use it.

Non-functional changes:
* BuiltinInfo interface changes:
  * `BuiltinInfo::identifyBuiltin()` now takes a `llvm:Function` const
    reference instead of a StringRef of the name.
  * `BuiltinInfo::analyzeBuiltin()` now takes a `llvm::Function` const
    reference instead of a const pointer.
  * `BuiltinInfo::isArgumentVectorized()` has been removed.
  * The above apply equally to CLBuiltinInfo and DXILBuiltinInfo.
