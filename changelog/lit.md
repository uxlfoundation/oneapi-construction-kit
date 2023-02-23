Upgrade guidance:

* `check-passes-host-lit` has been renamed to `check-host-compiler-lit`.

Feature additions:

* New LIT check targets have been introduced:
  * `check-host-lit` runs all mux/compiler lit tests for the `host` target
  * `check-riscv-lit` runs all mux/compiler lit tests for the `riscv` target
  * `check-mux-lit` runs all mux lit tests for all targets.
  * `check-compiler-lit` runs all compiler tests for all targets.
