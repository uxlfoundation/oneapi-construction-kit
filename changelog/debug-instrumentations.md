Upgrade guidance:

* The `CA_DEBUG_SUPPORT_ENABLED` pre-processor define has been renamed
  `CA_ENABLE_DEBUG_SUPPORT` to match the cmake variable.
* `CA_DEBUG_PASSES_ENABLED` pre-processor define has been renamed
  `CA_ENABLE_DEBUG_SUPPORT` to match the cmake variable.
* A new cmake option - `CA_ENABLE_LLVM_OPTIONS_IN_RELEASE` - has been added to
  provide support for parsing `CA_LLVM_OPTIONS` in release-mode builds.
* All debug instrumentations have been removed. Use `CA_LLVM_OPTIONS` instead:
  * `CA_PASS_PRINT=1` -> `CA_LLVM_OPTIONS=-debug-pass-manager`
  * `CA_PASS_VERIFY=1` -> `CA_LLVM_OPTIONS=-verify-each`
  * `CA_PASS_VERIFY_DUMP=1` -> `CA_LLVM_OPTIONS=-verify-each`
  * `CA_PASS_PRE_DUMP=1` -> `CA_LLVM_OPTIONS=-print-before/-print-before-all`
  * `CA_PASS_POST_DUMP=1` -> `CA_LLVM_OPTIONS=-print-after/-print-after-all`
  * `CA_PASS_SCEV_PRE_DUMP=1` -> re-compile with
    `llvm::ScalarEvolutionPrinterPass` added to the pipeline.
  * `CA_PASS_SCEV_POST_DUMP=1` -> re-compile with
    `llvm::ScalarEvolutionPrinterPass` added to the pipeline.
