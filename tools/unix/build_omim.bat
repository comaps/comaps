@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================
REM CoMaps Windows Build Wrapper
REM
REM Validates the MSVC environment, auto-detects Qt6, then
REM delegates to tools/unix/build_omim.sh via Git Bash.
REM All build flags/options are handled by the shell script.
REM
REM Usage: build_omim.bat [options] [targets...]
REM   See build_omim.sh -h for full option list.
REM
REM Run from: x64 Native Tools Command Prompt for VS 2022
REM ============================================================

REM 1. Require MSVC (cl.exe must be in PATH)
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

REM 2. Require Git Bash
where bash >nul 2>nul
if errorlevel 1 (
  echo ERROR: bash not found in PATH.
  echo Install Git for Windows: https://git-scm.com/download/win
  exit /b 1
)

REM 3. Auto-detect Qt6 MSVC variant under C:\Qt\
set "QT6_PREFIX="
for /d %%V in ("C:\Qt\6.*") do (
  if exist "%%V\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" set "QT6_PREFIX=%%V\msvc2022_64"
  if exist "%%V\msvc2019_64\lib\cmake\Qt6\Qt6Config.cmake" set "QT6_PREFIX=%%V\msvc2019_64"
)
if not defined QT6_PREFIX (
  echo.
  echo ERROR: Qt6 MSVC variant not found under C:\Qt\.
  echo Install Qt6 with the MSVC 2019/2022 64-bit component from:
  echo   https://www.qt.io/download-qt-installer
  echo See docs\INSTALL_DESKTOP.md for full setup instructions.
  echo.
  exit /b 1
)
echo Found Qt6 MSVC: !QT6_PREFIX!

REM 4. Resolve repo root (works whether invoked from repo root or tools\unix\)
set "OMIM_PATH=%~dp0"
if "!OMIM_PATH:~-1!"=="\" set "OMIM_PATH=!OMIM_PATH:~0,-1!"
if not exist "!OMIM_PATH!\CMakeLists.txt" (
  pushd "!OMIM_PATH!\..\.."
  set "OMIM_PATH=!CD!"
  popd
)

REM 5. Inject Windows-specific CMake flags via CMAKE_CONFIG.
REM    build_omim.sh prepends its own -U flags to whatever CMAKE_CONFIG contains,
REM    so these are included in the final cmake invocation automatically.
set "CMAKE_CONFIG=-DCMAKE_PREFIX_PATH=!QT6_PREFIX! -DCMAKE_UNITY_BUILD=OFF -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DUSE_PCH=OFF"

REM 6. Hand off to the shared build script (from repo root so ./configure.sh resolves).
cd /d "!OMIM_PATH!"
bash tools/unix/build_omim.sh %*
exit /b %ERRORLEVEL%
