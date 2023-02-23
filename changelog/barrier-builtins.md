Upgrade guidance:

* The helper function `compiler::utils::IsBarrierName` has been removed. Use
  `compiler::utils::BuiltinInfo::isMuxBarrierID` instead.
* `compiler::utils::BarrierRegions` now queries the `BuiltinInfo` for what were
  known as _movable work-item calls_. The
  `BuiltinInfo::isRematerializableBuiltinID` API now handles this, allowing
  targets to customize behaviour.
