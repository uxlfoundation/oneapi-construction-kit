:: Copyright (C) Codeplay Software Limited
::
:: Licensed under the Apache License, Version 2.0 (the "License") with LLVM
:: Exceptions; you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
:: WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
:: License for the specific language governing permissions and limitations
:: under the License.
::
:: SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
::
:: Script for initializing the Windows MSVC toolchain using vcvarsall.bat

@ECHO OFF

IF "%1" == "/?" (
  ECHO Utility script for running vcvarsall.bat to initialize MSVC toolchain.
  ECHO Usage: setup-vcvars.bat [arch]
  ECHO   arch - Architecture passed to vcvarsall.bat, defaults to x64.
  EXIT /B 0
)

SET arch=x64
IF NOT "%1" == "" (
  SET arch=%1
)

IF DEFINED VCINSTALLDIR (
 ECHO vcvarsall.bat has already been run
 EXIT /B 1
)

SET vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
IF EXIST %vswhere% (
  FOR /F "delims==" %%A IN ('%vswhere% -property InstallationPath -sort') DO (
    "%%A"\VC\Auxiliary\Build\vcvarsall.bat %arch%
    GOTO :EOF
  )
)

IF DEFINED VS140COMNTOOLS (
  "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" %arch%
  GOTO :EOF
)

ECHO Error: Couldn't discover location of vcvarsall.bat
EXIT /B 1
