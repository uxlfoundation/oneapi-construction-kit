Upgrade guidance:

* The `HandleBarriersPass` no longer sets `combined_vecz_scalar`,
  `vecz_scalar_loop_induction`, or `vecz_wrapper_loop` metadata.
* The unused `compiler::utils::mutateScalarPeelInductionStart` function has
  been removed.
* The vectorizer no longer sets `vecz_vector_predication` metadata on
  vector-predicated kernels. This information is available through the
  orig<->vecz link metadata.
