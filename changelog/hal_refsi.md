Upgrade guidance:

* `hal_refsi` now rounds up local buffer arguments to 128 bytes.
* `hal_refsi` now aligns pass-by-value kernel arguments to the next power of in
  `WI` mode: not just in `WG` mode.
