Versioning Scheme
=================

Versioning of the oneAPI Construction Kit follows the `semantic versioning
<http://semver.org/>`__ scheme; increments of the major version signify
incompatible API changes, increments of the minor version denote the addition
of new functionality which is backward compatible, and increments of the patch
version mean backward compatible bug fixes have been applied.

.. note::

  The notion of what constitutes a public API with regards to the versioning
  scheme applies variously to different components of the oneAPI Construction
  Kit and is yet to be fully fleshed out.

  Components which have well-defined public APIs include:

  - Any end user facing API: :doc:`/source/cl`, :doc:`/source/vk`, etc.
  - :doc:`mux-runtime-spec`
  - Aspects of the :doc:`mux-compiler-spec` including the core ``compiler``
    classes, mux builtin functions, function attributes and module metadata.
  - :doc:`/modules/metadata`
  - The :doc:`/modules/loader` Module, :doc:`/modules/cargo`, and :doc:`/modules/debug`.

  Components which do not have well-defined public APIs include:

  - Aspects of the :doc:`mux-compiler-spec` such as contracts on specific passes, 
  - The :doc:`/modules/host`, :doc:`/modules/riscv`, :doc:`/modules/spirv-ll`,
    :doc:`/modules/vecz` modules

  These will be defined in time but until then, versioning will be made on a
  best-effort basis.


Deprecation Policy
------------------

The specifications and public APIs constituting the oneAPI Construction Kit are
subject to the following deprecation policy:

- A deprecated API or feature **shall** continue to be supported until the next
  major release version.

