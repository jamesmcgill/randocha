:: Deletes any previous cache and build files created by BOTH cmake and cl build scripts

@echo off
set BASE_PATH=%~dp0
set CMAKE_BUILD_PATH=%BASE_PATH%build\
set CL_BUILD_PATH=%BASE_PATH%clbuild\


IF NOT EXIST %CMAKE_BUILD_PATH% GOTO NoCmakeDir
echo "Deleting CMake Build Path..."
rmdir /S /Q %CMAKE_BUILD_PATH%
:NoCmakeDir

IF NOT EXIST %CL_BUILD_PATH% GOTO NoCLDir
echo "Deleting CL Build Path..."
rmdir /S /Q %CL_BUILD_PATH%
:NoCLDir
