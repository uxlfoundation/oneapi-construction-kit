# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# List all available OpenCL ICD in the registry on Windows, for more info see:
# https://github.com/KhronosGroup/OpenCL-ICD-Loader#registering-an-icd-on-windows

foreach ( $icd in Get-ItemProperty -Path HKLM:\SOFTWARE\Khronos\OpenCL\Vendors ) {
  foreach ( $property in $icd.PSObject.Properties.Name ) {
    if ( !$property.StartsWith('PS') ) {
      # The path to the installable client driver is the property name.
      Write-Output "`"$property`""
    }
  }
}
