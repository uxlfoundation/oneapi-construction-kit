Upgrade guidance:

* The `compiler::utils` module has been updated to move all members under a
  single namespace `compiler::utils`. Replace references to `core::utils`,
  `::utils`, and `utils` in the context of this module, with `compiler::utils`.
* The `compiler::utils` module header paths have been moved to
  `compiler/utils`. Replace header includes of `<utils/*>` with
  `<compiler/utils/*`, except for `<include/utils/system.h>`, which is not a
  part of the `compiler::utils` module.
