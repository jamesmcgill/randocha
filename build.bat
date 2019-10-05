@echo off
::##############################################################################
:: Incrementally build and run tests
::##############################################################################

:: Configuration variables. You may edit these.
set BUILD_TARGET="Release"
set EXE_NAME="distribution_viz"



::##############################################################################
:: Shouldn't need to edit beyond this point
::##############################################################################
set BASE_PATH=%~dp0
set BUILD_PATH=%BASE_PATH%build\

:: Generate the project cache (if required)
if exist %BUILD_PATH%CMakeCache.txt goto SKIP_CMAKE
echo.
echo Missing CMakeCache.txt. Running build-generate.bat
call %BASE_PATH%build-generate.bat
:SKIP_CMAKE

:: Build and run our code
pushd %BUILD_PATH%
echo.
call cmake --build . --config %BUILD_TARGET% && (
.\%BUILD_TARGET%\%EXE_NAME%.exe
)
popd

echo.
echo Done!
echo -----
