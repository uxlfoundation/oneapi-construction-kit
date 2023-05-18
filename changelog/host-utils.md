Feature additions:

* A target-specific `host-utils` library has been added to serve both `mux` and
  `compiler` modules. Any other target may add their own equivalent library in
  `modules/utils/target/<target-name>`.
* The `host-common` library has been removed; its contents have been moved to
  `host-utils`.
