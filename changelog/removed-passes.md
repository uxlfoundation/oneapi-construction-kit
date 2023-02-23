Upgrade guidance:

* The `utils::AddWorkItemInfoStructPass`, `utils::AddWorkGroupInfoStructPass`,
  `utils::AddWorkItemFunctionsIfRequiredPass`,
  `utils::ReplaceLocalWorkItemIdFunctionsPass` and
  `utils::ReplaceNonLocalWorkItemFunctionsPass` have been removed. Use the
  `utils::AddSchedulingParametersPass` and
  `utils::DefineMuxBuiltinsPass` in their place.
