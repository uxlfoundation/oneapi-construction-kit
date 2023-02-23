:: Copyright (C) Codeplay Software Limited. All Rights Reserved.
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
