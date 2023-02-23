Upgrade guidance:
* The refsi M1 compiler target has been split off from the main refsi target. The
 risc-v compiler target remains, but is only compatible with the 'G' target,
 which is now the default. The 'M' target now exists under
 examples/refsi/refsi_m1. Both compiler targets are now derived from a new
 compiler riscv utils library. The M1 target can still be built by using
 -DCA_EXTERNAL_MUX_COMPILER_DIRS=<CA_DDK>/examples/refsi/refsi_m1/compiler/refsi_m1
 and -DCA_MUX_COMPILERS_TO_ENABLE="refsi_m1"
