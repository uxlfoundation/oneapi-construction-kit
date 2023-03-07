# Subgroup support rework:

Feature additions:
* created a mechanism for implementing degenerate subgroups selectively on some
  kernels and not others. It will also clone kernels to produce two versions
  (using degenerate and non-degenerate subgroups) when the local size is not
  known at compile time.
