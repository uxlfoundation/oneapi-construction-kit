# Scalable vectorization of shuffle vectors:

Feature additions:
* Vecz is now able to vectorize shuffle vector instructions (without scalarizing
  them first) when a scalable vectorization factor is requested.
