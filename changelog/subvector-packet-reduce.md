# Improve subgroup reductions over subvectors:

Feature additions:
* Improved efficiency of reductions over subvector packets by reducing to a
  single vector first and then reducing that to scalar, rather than the other
  way round.
