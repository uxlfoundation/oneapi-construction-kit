Upgrade guidance:

* The `CoreWorkItemInfo`, `CoreWorkGroupInfo`, and `Core_schedule_info_s`
  structure types have been renamed, switching "Core" to "Mux".
* The `CorePackedArgs` structure type has been renamed. It is now named as
  `MuxPackedArgs.` followed by the name of the kernel whose parameters it
  wraps. This guarantees a unique naming more often, as previously two kernels
  with different signatures would both compete for the same structure name,
  leading to `CorePackedArgs` and `CorePackedArgs.0`.
