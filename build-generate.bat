:: Generates CMake Cache
:: Deletes any previous cache and build files
:: If the environment variable VCPKG_PATH is set, vcpkg's toolchain will be used

@echo off
set BASE_PATH=%~dp0
set BUILD_PATH=%BASE_PATH%build\

IF NOT EXIST %BUILD_PATH% GOTO MissingBuildDir
echo "Deleting Build Path..."
rmdir /S /Q %BUILD_PATH%
:MissingBuildDir
IF NOT EXIST %BUILD_PATH% mkdir %BUILD_PATH%

pushd %BUILD_PATH%

IF NOT DEFINED VCPKG_PATH goto NoVcpkg
echo Using VcPkg toolchain located at %VCPKG_PATH% 
cmake -G "Visual Studio 15 2017" .. "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake"
goto FinishCMake

:NoVcpkg
echo VcPkg not found. Continuing without. 
cmake -G "Visual Studio 15 2017" ..

:FinishCMake
popd

echo[
echo CMake Cache Generation Done!
echo ----------------------------

