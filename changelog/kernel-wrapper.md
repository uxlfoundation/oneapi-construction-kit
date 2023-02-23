Upgrade guidance:

* The `compiler::utils::AddKernelWraperPass` now takes a set of options on
  construction, wrapping up the previous `bool` parameter. The pass can now opt
  in to packing kernel pointer parameters in the `local`/`Workgroup` address
  space as pointers, rather than by `size_t` (representing the size).
