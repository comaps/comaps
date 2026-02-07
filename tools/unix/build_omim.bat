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
REM 3. Check Python and install required packages
REM ============================================================
where python >nul 2>nul
if errorlevel 1 (
  echo ERROR: python not found in PATH
  echo Install: winget install -e --id Python.Python.3.13
  exit /b 1
)
for /f "tokens=*" %%i in ('python --version 2^>^&1') do echo Found %%i

REM protobuf < 4.0 is required by the CMake build
python -c "import google.protobuf; v=google.protobuf.__version__; exit(0 if tuple(int(x) for x in v.split('.')) < (4,0,0) else 1)" >nul 2>&1
if errorlevel 1 (
  echo Installing Python protobuf ...
  python -m pip install "protobuf<4.0" --quiet
  if errorlevel 1 (
    echo ERROR: Failed to install protobuf
    exit /b 1
  )
  echo Installed protobuf.
) else (
  echo Found protobuf.
)

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
REM 6. Detect or install Qt6 (MSVC variant)
REM ============================================================
set "QT6_DIR="
set "QT_VERSION=6.6.0"
set "QT_INSTALL_DIR=C:\Qt"
set "QT_MSVC_ARCH=msvc2019_64"
set "QT_AQT_ARCH=win64_msvc2019_64"

REM Search for existing MSVC Qt6
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
  echo Qt6 MSVC variant not found. Installing via aqtinstall...

  python -c "import aqt" >nul 2>&1
  if errorlevel 1 (
    echo Installing aqtinstall...
    python -m pip install aqtinstall --quiet
    if errorlevel 1 (
      echo ERROR: Failed to install aqtinstall
      exit /b 1
    )
  )

  set "QT6_TARGET_DIR=!QT_INSTALL_DIR!\!QT_VERSION!\!QT_MSVC_ARCH!"

  if not exist "!QT6_TARGET_DIR!\lib\cmake\Qt6\Qt6Config.cmake" (
    echo Installing Qt !QT_VERSION! [!QT_AQT_ARCH!]...
    python -m aqt install-qt windows desktop !QT_VERSION! !QT_AQT_ARCH! --outputdir "!QT_INSTALL_DIR!"
    if errorlevel 1 (
      echo ERROR: Failed to install Qt6 via aqtinstall
      exit /b 1
    )
  )

  set "QT6_PREFIX=!QT6_TARGET_DIR!"
  set "QT6_DIR=!QT6_TARGET_DIR!\lib\cmake\Qt6"
  echo Installed Qt6: !QT6_DIR!
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
set "OMIM_PATH=%~dp0\..\.."
for %%I in ("%OMIM_PATH%") do set "OMIM_PATH=%%~fI"

REM ============================================================
REM 9. Build
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
if exist CMakeCache.txt (
  findstr /R /C:"CMAKE_C_COMPILER:.*=.*[/\\]cl\.exe" CMakeCache.txt >nul 2>&1
  if errorlevel 1 (
    echo Compiler mismatch in cache, full clean required...
    popd
    rmdir /s /q "!BUILD_DIR!"
    mkdir "!BUILD_DIR!"
    pushd "!BUILD_DIR!"
  )
)

echo Configuring with CMake...
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
