# Vecz Refactor Vectorization Unit Builtin handling:

Non-functional changes:
* Vecz: Removed handling of builtin functions from the Vectorization Unit. The
  Vectorization Context is now directly responsible for returning everything
  required to vectorize a builtin.
