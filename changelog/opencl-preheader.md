Upgrade guidance:
* Offline OpenCL kernels compiled with `clc` are now serialized with the metadata API 
  and stored using a different binary format. Thus, kernels compiled with a previous 
  version of `clc` will no longer work. You will need to recompile all your offline 
  kernels for them to be accepted by ComputeAorta.
