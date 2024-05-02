
# compiler passes

This provides an easy way of building the compiler passes without extra OCK mux
related code. It currently only includes the compiler-pipeline, multi_llvm and
vecz directories, but this will be reviewed in the future.

It can be built by including from another repo, or directly here with:

```
  cmake -GNinja -DCA_LLVM_INSTALL_DIR=<path_to_llvm_install> <ock>/compiler_passes -B<build_dir>
  cd <build_dir>
  ninja
```
