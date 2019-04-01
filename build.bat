@echo off
::##############################################################################
:: Incrementally build and run tests
:: Expects build-generate.bat to be run beforehand
::##############################################################################

:: Configuration variables. You may edit these.
set BUILD_TARGET="Release"


::##############################################################################
:: Shouldn't need to edit beyond this point
::##############################################################################
set BASE_PATH=%~dp0
set BUILD_PATH=%BASE_PATH%build\

IF EXIST %BUILD_PATH%CMakeCache.txt GOTO Build
echo Missing CMakeCache.txt. Running build-generate.bat
call %BASE_PATH%build-generate.bat

:Build
pushd %BUILD_PATH%
cmake --build . --config %BUILD_TARGET%
popd

echo[
echo Done!
echo -----
:End
