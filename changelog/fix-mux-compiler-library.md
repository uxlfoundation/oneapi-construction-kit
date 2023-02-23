Bug fixes:
  * Fixed the use of CA_MUX_COMPILERS_TO_ENABLE which could be used to choose
    which compiler targets were enabled. CA_MUX_COMPILERS_TO_ENABLE can be used
    to decide which compiler directories to be added. If
    CA_MUX_COMPILERS_TO_ENABLE was passed in, the name of the library to be
    added was taken from this variable rather than the actual name of the
    library which would probably differ from the directory name. The logic is
    changed so that the library name to be added is taken from
    MUX_COMPILER_LIBRARIES, which was added to when the compiler target was
    added using `add_mux_compiler_target`. This allows
    CA_MUX_COMPILERS_TO_ENABLE to be used effectively to limit to one
    compiler library or another.

    CA_MUX_TARGETS_TO_ENABLE has been changed to use the same logic i.e. it only
    enables the directory inclusion.
