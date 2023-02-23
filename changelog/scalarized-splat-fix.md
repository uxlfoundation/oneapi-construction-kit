# Scalarizer output quality:

Bug fixes:
* When scalarizing a vector splat which is used by a non-scalarizable
  instruction, the scalarizer no longer inserts multiple insert element
  instructions to reconstruct the value, but restores the vector splat instead.
