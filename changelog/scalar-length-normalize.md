# Scalar Length and Normalize code generation:

Bug fixes:
* CL Builtins `length` and `fast_length` are now emitted as `fabs` for scalar
  types; `normalize` and `fast_normalize` are emitted as `sign`.

