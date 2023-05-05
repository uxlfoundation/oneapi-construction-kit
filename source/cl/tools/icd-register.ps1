# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
