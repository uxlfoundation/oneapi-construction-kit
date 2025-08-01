Bumping the ComputeMux Specification Version
============================================

1. Make the required changes to ``modules/mux/tools/api/mux.xml`` or the
   compiler interface in ``modules/compiler/include/compiler/*.h``.
2. Update ``${FUNCTION_PREFIX}_MAJOR_VERSION``,
   ``${FUNCTION_PREFIX}_MINOR_VERSION``, and
   ``${FUNCTION_PREFIX}_PATCH_VERSION`` in ``modules/mux/tools/api/mux.xml`` as
   appropriate.

   a. Note that as long as the version number is in the 0.X.Y stage it is very
      unlikely that you will be updating the major number.
3. Configure CMake, making sure that all ComputeMux targets are enabled. This
   includes ``host`` which is enabled by default, and the ``riscv`` target.
4. Build the ``mux-api-generate`` target e.g., ``ninja mux-api-generate``.
5. Update ``doc/modules/mux/changes.rst``. The previous step will have
   generated an empty entry for the latest version.
6. Update any other relevant documentation (e.g.,
   ``doc/specifications/mux-runtime-spec.rst`` and
   ``doc/specifications/mux-compiler-spec.rst``).
7. Create a merge request with all changes. Make sure to include the ComputeMux
   Runtime API XML, ``mux`` and all target headers, ``compiler``, and
   ``doc/modules/mux/changes.rst``.
