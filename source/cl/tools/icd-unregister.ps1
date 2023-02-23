# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Unregister an OpenCL ICD on Windows, for more info see:
# https://github.com/KhronosGroup/OpenCL-ICD-Loader#registering-an-icd-on-windows

# Specify the required OpenCLICD parameter.
[CmdletBinding()]
Param([Parameter(Mandatory=$true)] [String]$OpenCLICD)

# Enter the HKEY_LOCAL_MACHINE location, so we can modify it.
Push-Location
Set-Location HKLM:

# Unregister the given OpenCL ICD.
Remove-ItemProperty .\SOFTWARE\Khronos\OpenCL\Vendors -Name $OpenCLICD

# Leave the HKEY_LOCAL_MACHINE location.
Pop-Location
