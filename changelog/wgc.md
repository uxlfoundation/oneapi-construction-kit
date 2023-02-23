Bug fixes:

* Linear work-item schedules, produced by certain work-group collective
  builtins, no longer generate invalid IR when vectorized.
* Local-address-space accumulator global variables are now generated with
  `internal` linkage to fix potential unresolved symbols.
