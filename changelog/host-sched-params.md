Upgrade guidance:

* The `host` target now configures its scheduling parameters through a derived
  `compiler::utils::BIMuxInfoConcept`. It configures three scheduling
  parameters (found in the host target documentation). The
  `host::AddEntryHookPass` no longer alters the kernel ABI and purely performs
  work-group scheduling.
