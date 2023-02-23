Upgrade guidance:

* `utils::populateSimpleWorkItemInfoLookupFunction`,
  `utils::populateSimpleWorkGroupInfoLookupFunction` and
  `utils::populateSimpleWorkItemInfoSetterFunction` have been removed and
  replaced with the more generic `utils::populateStructSetterFunction` and
  `utils::populateStructGetterFunction`. This is so targets can easily reuse
  these functions to define builtins that may not necessarily use the
  `MuxWorkItemInfoStruct` or `MuxWorkGroupInfoStruct` types.
* The dependency on `utils::AddWorkItemFunctionsIfRequiredPass` from
  `utils::HandleBarriersPass` has been severed. The `HandleBarriersPass` now
  unconditionally calls the helper builtins `__mux_set_local_id` and
  `__mux_set_sub_group_id`. They will be optimized out by later passes if
  unused.

Feature additions:

* `utils::BuiltinInfo` can be used to get-or-declare mux work-item builtin
  functions.
