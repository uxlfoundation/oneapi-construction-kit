Bug fixes:

* `sub_group_scan_exclusive_*` builtins are now correctly scalably-vectorized
  for `half` and `double` types and no longer crash the compiler backend.
