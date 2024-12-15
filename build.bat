@echo off
setlocal

set DEBUG_BUILD_DIR=build\debug
set RELEASE_BUILD_DIR=build\release


echo Argument passed: %~1

:: Check for argument
if "%~1"=="" (
    echo Usage: %0 [debug^|release]
    exit /b 1
)

set BUILD_TYPE=%~1

:: Determine the build directory and CMake build type
if /I "%BUILD_TYPE%"=="debug" (
    set BUILD_DIR=%DEBUG_BUILD_DIR%
    set CMAKE_BUILD_TYPE=Debug
) else if /I "%BUILD_TYPE%"=="release" (
    set BUILD_DIR=%RELEASE_BUILD_DIR%
    set CMAKE_BUILD_TYPE=Release
) else (
    echo Usage: %0 [debug^|release]
    exit /b 1
)

:: Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: Change to the build directory
cd /d "%BUILD_DIR%" || exit /b 1

echo Configuring CMake project for %CMAKE_BUILD_TYPE% mode...
cmake -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DRENDERER_BACKEND=OpenGL ..\..

echo Building the project in %CMAKE_BUILD_TYPE% mode...
cmake --build . --config %CMAKE_BUILD_TYPE%

echo Build complete. Output stored in %BUILD_DIR%.

endlocal

