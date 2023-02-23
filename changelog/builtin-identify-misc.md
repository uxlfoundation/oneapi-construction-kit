# BuiltinInfo interface tidy up:

Non-functional changes:
* CLBuiltinInfo now assigns Builtin IDs for `VLoad`/`VStore` (including `_half`
  variants), and also `select` and `as_` builtins, simplifying code and easing
  maintenance.
* The BuiltinInfo interface no longer has `emitBuiltinInline` that accepts a
  BuiltinID. Use the version that accepts a Function pointer instead.
* The Builtininfo interface no longer has `materializeBuiltin` function, since
  this is only used internally by specific implementations.

