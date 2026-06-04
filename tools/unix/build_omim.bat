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

REM 2. Require Git Bash (explicitly — WSL bash produces a Linux build)
set "GIT_BASH="
for %%C in (
  "C:\Program Files\Git\bin\bash.exe"
  "C:\Program Files\Git\usr\bin\bash.exe"
) do if exist %%C if not defined GIT_BASH set "GIT_BASH=%%~C"

if not defined GIT_BASH (
  REM Fall back to PATH, but reject WSL bash (System32\bash.exe)
  for /f "delims=" %%P in ('where bash 2^>nul') do (
    if /i not "%%P"=="%SystemRoot%\System32\bash.exe" (
      if /i not "%%P"=="%LOCALAPPDATA%\Microsoft\WindowsApps\bash.exe" (
        if not defined GIT_BASH set "GIT_BASH=%%P"
      )
    )
  )
)

if not defined GIT_BASH (
  echo.
  echo ERROR: Git Bash not found. WSL bash cannot be used ^(it produces a Linux build^).
  echo Install Git for Windows: https://git-scm.com/download/win
  echo.
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

REM 5. Windows-specific environment for configure.sh
REM    - PYTHONUTF8: prevents cp1252 decode errors when Python reads UTF-8 MapCSS/resource files
REM    - SKIP_GENERATE_SERBIAN_LATIN_STRINGS: requires uconv (ICU), not available on Windows
REM    - optipng: warn if missing (symbols generation will be skipped by configure.sh automatically)
set "PYTHONUTF8=1"
set "SKIP_GENERATE_SERBIAN_LATIN_STRINGS=1"
where optipng >nul 2>nul || (
  echo WARNING: optipng not found — symbol sprites will not be regenerated.
  echo          Install via: choco install optipng
  echo          Or run: winget install optipng
)

REM 7. Inject Windows-specific CMake flags via CMAKE_CONFIG.
REM    build_omim.sh prepends its own -U flags to whatever CMAKE_CONFIG contains,
REM    so these are included in the final cmake invocation automatically.
set "CMAKE_CONFIG=-DCMAKE_PREFIX_PATH=!QT6_PREFIX! -DCMAKE_UNITY_BUILD=OFF -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DUSE_PCH=OFF"

REM 8. Hand off to the shared build script (from repo root so ./configure.sh resolves).
cd /d "!OMIM_PATH!"
"!GIT_BASH!" tools/unix/build_omim.sh %*
exit /b %ERRORLEVEL%
