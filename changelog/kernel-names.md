Upgrade guidance:

* The compiler no longer necessarily generates kernels which retain their
  original names. The actual generated kernel name is conveyed through ELF
  metadata along with the original name, so the runtime can map between kernels
  and their generated forms.
* The `mux-orig-fn` function attribute now truly conveys the original function
  name and should not change throughout the compiler. Its previous use as the
  base name component which is used when generating new kernel wrappers has
  been renamed as `mux-base-fn-name`.
