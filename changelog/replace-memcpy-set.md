Feature additions:

* A new utils pass `compiler::utils::ReplaceMemcpySetIntrinsicsPass` has been
  created which will replace calls to intrinsics `llvm.memcpy.*`, `llvm.memset.*`
  and `llvm.memmove.*` with calls to a generated equivalent loop. This pass has
  now been added to the `riscv` target and conditionally to the cookie cutter
  generated target, based off the `json` "feature" element `replace_mem`.
  