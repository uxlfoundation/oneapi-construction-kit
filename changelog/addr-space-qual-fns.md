# Generic Address Space Support:

Upgrade guidance:
* Targets may need to provide their own version of the
  `compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass` if different
  address spaces are supported in hardware, if they wish to support the feature.

Feature additions:
* Initial basic support for Generic Address Space in OpenCL 3.0.
