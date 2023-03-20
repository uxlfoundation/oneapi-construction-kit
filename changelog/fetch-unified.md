Upgrade guidance:
* The unified runtime submodule has been removed and is now fetched if
  CA_ENABLE_API includes `ur`. The default for CA_ENABLE_API is now `cl;vk`
  rather than all APIs as unified runtime does not fully work across all targets.
