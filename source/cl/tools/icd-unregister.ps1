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
