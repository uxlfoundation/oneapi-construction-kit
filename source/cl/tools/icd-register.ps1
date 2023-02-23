# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Register an OpenCL ICD on Windows, for more info see:
# https://github.com/KhronosGroup/OpenCL-ICD-Loader#registering-an-icd-on-windows

# Specify the required OpenCLICD parameter.
[CmdletBinding()]
Param([Parameter(Mandatory=$true)] [String]$OpenCLICD)

# Enter the HKEY_LOCAL_MACHINE location, so we can modify it.
Push-Location
Set-Location HKLM:

# Ensure that .\SOFTWARE\Khronos\OpenCL\Vendors actually exists in
# the HKEY_LOCAL_MACHINE location.
if (!(Test-Path .\SOFTWARE\Khronos)) {
  New-Item -Path .\SOFTWARE -Name Khronos
}
if (!(Test-Path .\SOFTWARE\Khronos\OpenCL)) {
  New-Item -Path .\SOFTWARE\Khronos\ -Name OpenCL
}
if (!(Test-Path .\SOFTWARE\Khronos\OpenCL\Vendors)) {
  New-Item -Path .\SOFTWARE\Khronos\OpenCL\ -Name Vendors
}

# Register the given OpenCL ICD.
New-ItemProperty -Path .\SOFTWARE\Khronos\OpenCL\Vendors `
  -Name $OpenCLICD -Value 0 -PropertyType DWORD

# Leave the HKEY_LOCAL_MACHINE location.
Pop-Location
