Upgrade guidance:

* Support for supplying the `-cl-std` argument to `clc` in offline UnitCL
  kernels has been moved from `CLC OPTIONS:` into a new `CL_STD:` requirement.
  This allows it to be conveniently passed to the compiler invocation when
  compiling SPIR-V kernels.
