Upgrade guidance:

* The `metadata` library's `VectorizeInfoMetadata` now represents the 'work
  width' as a single named structure -- `FixedOrScalableQuantity<uint64_t>`,
  rather than two disjoint values for the known and scalable parts.
