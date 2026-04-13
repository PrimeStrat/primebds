@echo off
setlocal enabledelayedexpansion

:: PrimeBDS Build Script for Windows
:: Requires: CMake 3.15+, C++20 compiler (Clang, MSVC, or MinGW)

set "BUILD_DIR=build\windows"
set "BUILD_TYPE=Release"
set "COMPILER_FLAGS="

:: Parse arguments
:parse_args
if "%~1"=="" goto done_args
if /i "%~1"=="--debug"   (set "BUILD_TYPE=Debug"   & shift & goto parse_args)
if /i "%~1"=="--release" (set "BUILD_TYPE=Release"  & shift & goto parse_args)
if /i "%~1"=="--clean"   (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    shift & goto parse_args
)
if /i "%~1"=="--help" (
    echo Usage: build.bat [options]
    echo   --debug     Build in Debug mode
    echo   --release   Build in Release mode ^(default^)
    echo   --clean     Remove build directory before building
    echo   --help      Show this help
    exit /b 0
)
echo Unknown option: %~1
exit /b 1
:done_args

:: Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake 3.15+ and add it to PATH.
    exit /b 1
)

:: Detect C++ compiler and set up MSVC environment
set "COMPILER_FOUND="
set "VCVARS_FOUND="

:: 1) Try to set up MSVC environment via vcvarsall.bat
::    Check VS 2026 (v18), 2022, 2019, 2017 across editions
for %%V in (18 2022 2019 2017) do (
    for %%E in (Community Professional Enterprise BuildTools) do (
        if not defined VCVARS_FOUND (
            if exist "C:\Program Files\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
                echo Setting up MSVC environment from VS %%V %%E...
                call "C:\Program Files\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
                set "VCVARS_FOUND=1"
            )
            if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
                echo Setting up MSVC environment from VS %%V %%E...
                call "C:\Program Files (x86)\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
                set "VCVARS_FOUND=1"
            )
        )
    )
)

:: 2) Check if cl.exe is now on PATH (MSVC native compiler)
where cl >nul 2>&1
if not errorlevel 1 (
    set "COMPILER_FOUND=MSVC"
    goto :compiler_done
)

:: 3) Check for Clang (works with MSVC libs set up above)
where clang++ >nul 2>&1
if not errorlevel 1 (
    set "COMPILER_FOUND=Clang"
    set "COMPILER_FLAGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
    goto :compiler_done
)
if exist "C:\Program Files\LLVM\bin\clang++.exe" (
    set "PATH=C:\Program Files\LLVM\bin;%PATH%"
    set "COMPILER_FOUND=Clang"
    set "COMPILER_FLAGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
    goto :compiler_done
)

:: 4) Check for MinGW g++
where g++ >nul 2>&1
if not errorlevel 1 (
    set "COMPILER_FOUND=MinGW"
    goto :compiler_done
)

:compiler_done
if not defined COMPILER_FOUND (
    echo [ERROR] No C++ compiler found. Install one of:
    echo   - Visual Studio with "Desktop development with C++" workload
    echo   - LLVM/Clang: winget install LLVM.LLVM
    echo   - MinGW-w64
    exit /b 1
)
echo Compiler: %COMPILER_FOUND%
if defined VCVARS_FOUND echo MSVC environment: loaded

:: Pick generator - prefer Ninja if available, fall back to NMake
set "GENERATOR=NMake Makefiles"
where ninja >nul 2>&1
if not errorlevel 1 set "GENERATOR=Ninja"

echo ============================================
echo  PrimeBDS Build - Windows (%BUILD_TYPE%)
echo  Generator: %GENERATOR%
echo ============================================

:: Configure
echo [1/2] Configuring...
cmake -S . -B "%BUILD_DIR%" -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON %COMPILER_FLAGS%
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

:: Build
echo [2/2] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel -- -k 0
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo ============================================
echo  Build succeeded!
echo  Output: %BUILD_DIR%\%BUILD_TYPE%\output\
echo ============================================
