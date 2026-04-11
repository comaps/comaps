@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================
REM CoMaps Windows Build Script
REM ============================================================
REM Usage: build_omim.bat [options] [targets...]
REM   -d          Debug build
REM   -r          Release build (skips tests)
REM   -R          RelWithDebInfo build
REM   -c          Clean build directory first
REM   -a          Standalone build
REM   -t          Designer build
REM   -j          Export compile_commands.json
REM   -l          Launch binary after build
REM   -n <jobs>   Number of parallel jobs
REM   -p <path>   Build output directory
REM
REM Defaults to Debug + RelWithDebInfo if no build type specified.
REM Run from: x64 Native Tools Command Prompt for VS 2022
REM ============================================================

REM ============================================================
REM 1. Check MSVC compiler
REM ============================================================
where cl >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERROR: MSVC compiler [cl.exe] not found.
  echo Run this script from:
  echo   x64 Native Tools Command Prompt for VS 2022
  echo.
  echo Or install Build Tools:
  echo   https://visualstudio.microsoft.com/visual-cpp-build-tools/
  echo.
  exit /b 1
)
echo Using compiler:
cl 2>&1 | findstr /C:"Version"
echo.

REM ============================================================
REM 2. Check CMake
REM ============================================================
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: cmake.exe not found in PATH
  echo Install from: https://cmake.org/download/
  exit /b 1
)
for /f "tokens=*" %%i in ('cmake --version 2^>^&1 ^| findstr /C:"cmake version"') do echo Found %%i

REM ============================================================
REM 3. Check Python
REM ============================================================
where python >nul 2>nul
if errorlevel 1 (
  echo ERROR: python not found in PATH
  echo Install: winget install -e --id Python.Python.3.13
  exit /b 1
)
for /f "tokens=*" %%i in ('python --version 2^>^&1') do echo Found %%i
REM CMake checks for protobuf ^< 4.0 at configure time and will warn if missing.
REM See INSTALL_DESKTOP.md for how to install it.

REM ============================================================
REM 4. Check Git / Bash (needed for version.sh)
REM ============================================================
where git >nul 2>nul
if errorlevel 1 (
  echo ERROR: git not found in PATH
  echo Install from: https://git-scm.com/download/win
  exit /b 1
)
echo Found Git.

REM ============================================================
REM 5. Detect build tool (Ninja or MSBuild)
REM ============================================================
where ninja >nul 2>nul
if errorlevel 1 (
  echo Ninja not found, will use MSBuild.
  set "GENERATOR=Visual Studio 17 2022"
  set "BUILD_CMD=cmake --build . --parallel"
  set "COMPILER_FLAGS="
) else (
  echo Found Ninja.
  set "GENERATOR=Ninja"
  set "BUILD_CMD=ninja"
  REM Force MSVC when using Ninja, otherwise CMake may pick up Clang from PATH.
  set "COMPILER_FLAGS=-DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl"
)

REM ============================================================
REM 6. Detect Qt6 (MSVC variant)
REM ============================================================
set "QT6_DIR="
set "QT_INSTALL_DIR=C:\Qt"

REM Search for any installed MSVC Qt6 under C:\Qt\
for /d %%V in ("%QT_INSTALL_DIR%\6.*") do (
  if exist "%%V\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" (
    set "QT6_PREFIX=%%V\msvc2022_64"
    set "QT6_DIR=%%V\msvc2022_64\lib\cmake\Qt6"
  )
  if exist "%%V\msvc2019_64\lib\cmake\Qt6\Qt6Config.cmake" (
    set "QT6_PREFIX=%%V\msvc2019_64"
    set "QT6_DIR=%%V\msvc2019_64\lib\cmake\Qt6"
  )
)

if defined QT6_DIR (
  echo Found Qt6 MSVC: !QT6_DIR!
) else (
  echo.
  echo ERROR: Qt6 MSVC variant not found under %QT_INSTALL_DIR%\.
  echo Install Qt6 with the MSVC 2019/2022 64-bit component from:
  echo   https://www.qt.io/download-qt-installer
  echo See docs\INSTALL_DESKTOP.md for full setup instructions.
  echo.
  exit /b 1
)

echo.
echo === Prerequisites OK ===
echo.

REM ============================================================
REM 7. Parse arguments
REM ============================================================
set "OPT_DEBUG="
set "OPT_RELEASE="
set "OPT_RELWITHDEBINFO="
set "OPT_CLEAN="
set "OPT_STANDALONE="
set "OPT_DESIGNER="
set "OPT_TARGET="
set "OPT_PATH="
set "OPT_COMPILE_DATABASE="
set "OPT_LAUNCH_BINARY="
set "OPT_NJOBS="
set "CMAKE_CONFIG=-DUSE_PCH=OFF"

:parse
if "%~1"=="" goto :endparse

if "%~1"=="-d" set "OPT_DEBUG=1"
if "%~1"=="-r" (
  set "OPT_RELEASE=1"
  set "CMAKE_CONFIG=!CMAKE_CONFIG! -DSKIP_TESTS=ON"
)
if "%~1"=="-R" set "OPT_RELWITHDEBINFO=1"
if "%~1"=="-c" set "OPT_CLEAN=1"
if "%~1"=="-a" set "OPT_STANDALONE=1"
if "%~1"=="-t" set "OPT_DESIGNER=1"
if "%~1"=="-j" (
  set "OPT_COMPILE_DATABASE=1"
  set "CMAKE_CONFIG=!CMAKE_CONFIG! -DCMAKE_EXPORT_COMPILE_COMMANDS=YES"
)
if "%~1"=="-l" set "OPT_LAUNCH_BINARY=1"

if "%~1"=="-n" (
  shift
  set "OPT_NJOBS=%~1"
  set "CMAKE_CONFIG=!CMAKE_CONFIG! -DNJOBS=!OPT_NJOBS!"
)

if "%~1"=="-p" (
  shift
  set "OPT_PATH=%~1"
)

shift
goto :parse
:endparse

REM Default: Debug + RelWithDebInfo
if not defined OPT_DEBUG if not defined OPT_RELEASE if not defined OPT_RELWITHDEBINFO (
  set "OPT_DEBUG=1"
  set "OPT_RELWITHDEBINFO=1"
)

if not defined OPT_NJOBS set "OPT_NJOBS=%NUMBER_OF_PROCESSORS%"

REM ============================================================
REM 8. Paths
REM ============================================================
REM Resolve source directory.
REM When called via wrapper, %~dp0 may return the caller's directory (repo root).
REM When called directly from tools\unix\, %~dp0 is the script directory.
set "OMIM_PATH=%~dp0"
if "!OMIM_PATH:~-1!"=="\" set "OMIM_PATH=!OMIM_PATH:~0,-1!"
if not exist "!OMIM_PATH!\CMakeLists.txt" (
  pushd "!OMIM_PATH!\..\.."
  set "OMIM_PATH=!CD!"
  popd
)
echo Source directory: !OMIM_PATH!

REM ============================================================
REM 9. Run configure.sh (generates drules, symbols, and other data files)
REM ============================================================
echo Running configure.sh...
pushd "!OMIM_PATH!"
bash configure.sh
if errorlevel 1 (
  popd
  echo ERROR: configure.sh failed
  exit /b 1
)
popd

REM ============================================================
REM 10. Build
REM ============================================================
if defined OPT_DEBUG call :build Debug
if errorlevel 1 exit /b 1
if defined OPT_RELEASE call :build Release
if errorlevel 1 exit /b 1
if defined OPT_RELWITHDEBINFO call :build RelWithDebInfo
if errorlevel 1 exit /b 1

echo.
echo === All builds complete ===
exit /b 0

REM ============================================================
REM Build subroutine
REM ============================================================
:build
set "CONF=%~1"
echo.
echo ========================================
echo  Building %CONF%...
echo ========================================

if defined OPT_PATH (
  set "BUILD_DIR=!OPT_PATH!\omim-build-!CONF!"
) else (
  set "BUILD_DIR=!OMIM_PATH!\..\omim-build-!CONF!"
)

if defined OPT_CLEAN if exist "!BUILD_DIR!" (
  echo Cleaning !BUILD_DIR!...
  rmdir /s /q "!BUILD_DIR!"
)

if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"
pushd "!BUILD_DIR!"

REM Auto-clean if cached compiler doesn't match (e.g. switching from Clang to MSVC).
if not exist CMakeCache.txt goto :compiler_ok
findstr /C:"=cl" CMakeCache.txt >nul 2>&1
if not errorlevel 1 goto :compiler_ok
echo Compiler mismatch in cache, full clean required...
popd
rmdir /s /q "!BUILD_DIR!"
mkdir "!BUILD_DIR!"
pushd "!BUILD_DIR!"
:compiler_ok

echo Configuring with CMake...
echo   Source: "!OMIM_PATH!"
echo   Build:  "!BUILD_DIR!"
cmake "!OMIM_PATH!" ^
  -G "!GENERATOR!" ^
  -DCMAKE_BUILD_TYPE=!CONF! ^
  -DCMAKE_PREFIX_PATH="!QT6_PREFIX!" ^
  -DCMAKE_UNITY_BUILD=OFF ^
  !COMPILER_FLAGS! ^
  !CMAKE_CONFIG!

if errorlevel 1 (
  echo.
  echo ERROR: CMake configuration failed for !CONF!
  popd
  exit /b 1
)

echo Building...
!BUILD_CMD! !OPT_TARGET!

if errorlevel 1 (
  echo.
  echo ERROR: Build failed for !CONF!
  popd
  exit /b 1
)

if defined OPT_LAUNCH_BINARY (
  for %%T in (!OPT_TARGET!) do (
    if exist "%%T.exe" "%%T.exe"
  )
)

if defined OPT_COMPILE_DATABASE (
  if exist compile_commands.json copy /y compile_commands.json "!OMIM_PATH!"
)

popd
goto :eof
