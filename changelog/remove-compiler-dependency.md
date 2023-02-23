Bug fixes:
* The `riscv` and generated targets have been updated to store the
  `VectorizeInfoMetadata` directly, removing the dependency on the compiler,
  allowing CA_RUNTIME_COMPILER_ENABLED to be used on risc-v and other targets.
