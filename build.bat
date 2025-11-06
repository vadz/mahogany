@echo off
REM #############################################################################
REM build.bat - Sample build script for Mahogany using CMake on Windows
REM #############################################################################
REM This script demonstrates how to build Mahogany with various configurations
REM on Windows using Visual Studio or other generators.

setlocal

REM Configuration variables
set BUILD_TYPE=Release
set BUILD_SHARED=OFF
set ENABLE_PYTHON=ON
set ENABLE_SSL=ON
set BUILD_TESTS=ON
set GENERATOR=Visual Studio 17 2022
set PLATFORM=x64

REM Build directory
set BUILD_DIR=build

echo === Mahogany CMake Build Script (Windows) ===
echo Build type: %BUILD_TYPE%
echo Generator: %GENERATOR%
echo Platform: %PLATFORM%
echo Shared libs: %BUILD_SHARED%
echo Python support: %ENABLE_PYTHON%
echo SSL support: %ENABLE_SSL%
echo Build tests: %BUILD_TESTS%
echo.

REM Clean build directory if requested
if "%1"=="clean" (
    echo Cleaning build directory...
    rmdir /s /q %BUILD_DIR% 2>nul
    echo Clean complete.
    goto :eof
)

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure with CMake
echo Configuring with CMake...
cmake .. ^
    -G "%GENERATOR%" ^
    -A %PLATFORM% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_SHARED_LIBS=%BUILD_SHARED% ^
    -DENABLE_PYTHON=%ENABLE_PYTHON% ^
    -DENABLE_SSL=%ENABLE_SSL% ^
    -DBUILD_TESTS=%BUILD_TESTS%

if %ERRORLEVEL% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Run tests if enabled
if "%BUILD_TESTS%"=="ON" (
    echo Running tests...
    ctest -C %BUILD_TYPE% --output-on-failure
)

echo.
echo === Build Complete ===
echo To install: cmake --install . --config %BUILD_TYPE%
echo To open in Visual Studio: start Mahogany.sln

pause