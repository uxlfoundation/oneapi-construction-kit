Bug fixes:

* The `AlignModuleStructsPass` has been fixed so that it pads struct types more
  closely matching the user-facing SPIR ABI. This also means it no longer pads
  in cases where it previously unnecessarily did.
