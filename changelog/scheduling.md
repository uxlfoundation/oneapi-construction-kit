Feature additions:

* Two new passes - `utils::AddSchedulingParametersPass` and
  `utils::DefineMuxBuiltinsPass` - have been added to replace the old pass
  framework of adding scheduling paramaters and replacing work-item builtins:
  * `core::utils::AddWorkItemFunctionsIfRequiredPass`
  * `core::utils::AddWorkItemInfoStructPass`
  * `core::utils::ReplaceLocalWorkItemIdFuncsPass`
  * `core::utils::AddWorkGroupInfoStructPass`
  * `core::utils::ReplaceNonLocalWorkItemFuncsPass`

  The two replacement passes utilize `utils::BuiltinInfo` APIs concerning
  scheduling parameters (see `utils::BuiltinInfo::SchedParamInfo` and
  associated methods) and for defining mux work-item builtins (see
  `utils::BuiltinInfo::defineMuxBuiltin`)
