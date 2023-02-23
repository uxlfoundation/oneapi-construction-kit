# Refactor Builtin Loader:

Upgrade guidance:
* Changes to BuiltinLoader class hierarchy:
  * `compiler::utils::BuiltinLoader` is renamed to
    `compiler::utils::CLBuiltinLoader`, and is now declared in
    `modules/compiler/utils/cl_builtin_info.h`.
  * `compiler::utils::SimpleLazyBuiltinLoader` is replaced with
    `compiler::utils::SimpleCLBuiltinLoader`, and is now declared in
    `modules/compiler/utils/cl_builtin_info.h`.
  * `compiler::utils::createSimpleLazyCLBuiltinInfo` is renamed to
    `compiler::utils::createCLBuiltinInfo`, and is now declared in
    `modules/compiler/utils/cl_builtin_info.h`.
  * `compiler::utils::LazyBuiltinLoader` has been removed. Use either
    `compiler::utils::SimpleCLBuiltinLoader` or subclass
    `compiler::utils::CLBuiltinLoader` directly instead.

Non-functional changes:
* Simplified BuiltinLoader hierarchy and made it specific to CLBuiltinInfo.
  CLBuiltinInfo can now be constructed directly from a Builtins module.
