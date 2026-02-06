@echo off
setlocal EnableExtensions

REM winget install -e --id Python.Python.3.13

REM ============================================================
REM MSVC sanity check
REM ============================================================

where cl >nul 2>nul
if errorlevel 1 goto :NO_MSVC

echo Using compiler:
cl 2>&1 | findstr /C:"Version"

REM ============================================================
REM Force CLI-only build (no Qt GUI)
REM ============================================================

set CMAKE_CONFIG=-DSKIP_QT_GUI=ON -DBUILD_DESIGNER=OFF -DBUILD_STANDALONE=OFF

REM ============================================================
REM Auto-detect Qt6 (optional)
REM ============================================================

set QT6_DIR=

if exist C:\Qt (
  for /d %%V in (C:\Qt\6.*) do (
    if exist %%V\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake (
      set QT6_DIR=%%V\msvc2022_64\lib\cmake\Qt6
    )
    if exist %%V\msvc2019_64\lib\cmake\Qt6\Qt6Config.cmake (
      set QT6_DIR=%%V\msvc2019_64\lib\cmake\Qt6
    )
  )
)

if not "%QT6_DIR%"=="" (
  echo Found Qt6: %QT6_DIR%
  set CMAKE_CONFIG=%CMAKE_CONFIG% -DQt6_DIR="%QT6_DIR%"
) else (
  echo Qt6 not found

  echo Installing Qt6 automatically...

  echo Checking for aqtinstall...
  python -c "import aqtinstall" >nul 2>&1

  if errorlevel 1 (
    echo Installing aqtinstall via python -m pip...
     python -m pip install aqtinstall
     python -c "import aqtinstall; print(aqtinstall.__file__)"
      if errorlevel 1 (
          echo ERROR: Failed to install aqtinstall
          exit /b 1
      )
  )

  set QT_INSTALL_DIR=C:\Qt
  set QT_VERSION=6.6.1
  set QT_MSCV_VARIANT=msvc2022_64

  set QT6_DIR=%QT_INSTALL_DIR%\%QT_VERSION%\%QT_MSCV_VARIANT%

  if not exist "%QT6_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
      echo Qt6 not found, installing automatically via aqtinstall...
      python -m aqtinstall install-qt windows desktop %QT_VERSION% %QT_MSCV_VARIANT% --outputdir %QT_INSTALL_DIR%
      if errorlevel 1 (
          echo ERROR: Failed to install Qt6 via aqtinstall
          exit /b 1
      )
  ) else (
      echo Found Qt6 at %QT6_DIR%
  )

  REM ============================================================
  REM 7️⃣ Configure build flags
  REM ============================================================

set CMAKE_CONFIG=-DQt6_DIR="%QT6_DIR%" -DBUILD_DESIGNER=ON -DBUILD_STANDALONE=ON

)

REM ============================================================
REM CMake / Ninja
REM ============================================================

where cmake >nul 2>nul || goto :NO_CMAKE

where ninja >nul 2>nul
if errorlevel 1 (
  echo Ninja not found, using CMake build
  set CMAKE_GENERATOR=Visual Studio 17 2022
  set BUILD_CMD=cmake --build . --parallel
) else (
  echo Using Ninja
  set CMAKE_GENERATOR=Ninja
  set BUILD_CMD=ninja
)

goto :EOF

:NO_MSVC
echo.
echo ERROR: MSVC compiler (cl.exe) not found.
echo Run from:
echo   x64 Native Tools Command Prompt for VS
echo.
echo Or install Build Tools:
echo   https://visualstudio.microsoft.com/visual-cpp-build-tools/
echo.
exit /b 1

:NO_CMAKE
echo.
echo ERROR: cmake.exe not found in PATH
echo.
exit /b 1


REM =========================
REM Defaults
REM =========================
set OPT_DEBUG=
set OPT_RELEASE=
set OPT_RELWITHDEBINFO=
set OPT_CLEAN=
set OPT_STANDALONE=
set OPT_DESIGNER=
set OPT_TARGET=
set OPT_PATH=
set OPT_COMPILE_DATABASE=
set OPT_LAUNCH_BINARY=
set OPT_NJOBS=

set CMAKE_CONFIG=%CMAKE_CONFIG% -DBUILD_DESIGNER=OFF -DBUILD_STANDALONE=OFF

REM =========================
REM Parse arguments
REM =========================
:parse
if "%~1"=="" goto endparse

if "%~1"=="-d" set OPT_DEBUG=1
if "%~1"=="-r" (
  set OPT_RELEASE=1
  set CMAKE_CONFIG=!CMAKE_CONFIG! -DSKIP_TESTS=1
)
if "%~1"=="-R" set OPT_RELWITHDEBINFO=1
if "%~1"=="-c" set OPT_CLEAN=1
if "%~1"=="-x" set CMAKE_CONFIG=!CMAKE_CONFIG! -DUSE_PCH=YES
if "%~1"=="-a" set OPT_STANDALONE=1
if "%~1"=="-t" set OPT_DESIGNER=1
if "%~1"=="-j" (
  set OPT_COMPILE_DATABASE=1
  set CMAKE_CONFIG=!CMAKE_CONFIG! -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
)
if "%~1"=="-l" set OPT_LAUNCH_BINARY=1

if "%~1"=="-n" (
  shift
  set OPT_NJOBS=%~1
  set CMAKE_CONFIG=!CMAKE_CONFIG! -DNJOBS=%OPT_NJOBS%
)

if "%~1"=="-p" (
  shift
  set OPT_PATH=%~1
)

REM Anything else = target
if not "%~1:~0,1%"=="-" (
  set OPT_TARGET=!OPT_TARGET! %~1
)

shift
goto parse
:endparse

REM =========================
REM Defaults
REM =========================
if not defined OPT_DEBUG if not defined OPT_RELEASE if not defined OPT_RELWITHDEBINFO (
  set OPT_DEBUG=1
  set OPT_RELWITHDEBINFO=1
)

REM =========================
REM Paths
REM =========================
set OMIM_PATH=%~dp0\..\..
for %%I in ("%OMIM_PATH%") do set OMIM_PATH=%%~fI

REM =========================
REM Find tools
REM =========================
where cmake >nul 2>nul || (
  echo ERROR: cmake not found
  exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
  echo Ninja not found, falling back to MSBuild
  set GENERATOR=Visual Studio 17 2022
  set BUILD_CMD=cmake --build . --parallel
) else (
  set GENERATOR=Ninja
  set BUILD_CMD=ninja
)

REM =========================
REM CPU count
REM =========================
if not defined OPT_NJOBS (
  set OPT_NJOBS=%NUMBER_OF_PROCESSORS%
)

REM =========================
REM Build function
REM =========================
call :build Debug
call :build Release
call :build RelWithDebInfo
exit /b 0

REM =========================
REM Build implementation
REM =========================
:build
set CONF=%1

if "%CONF%"=="Debug" if not defined OPT_DEBUG goto :eof
if "%CONF%"=="Release" if not defined OPT_RELEASE goto :eof
if "%CONF%"=="RelWithDebInfo" if not defined OPT_RELWITHDEBINFO goto :eof

if defined OPT_PATH (
  set BUILD_DIR=%OPT_PATH%\omim-build-%CONF%
) else (
  set BUILD_DIR=%OMIM_PATH%\..\omim-build-%CONF%
)

if defined OPT_CLEAN if exist "%BUILD_DIR%" (
  rmdir /s /q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"

set QT_FOUND=

where qmake >nul 2>nul && (
  for %%I in (qmake.exe) do set QMAKE_PATH=%%~$PATH:I
)

for %%D in (
  C:\Qt
  C:\Qt\6.*
) do (
  if exist "%%D" (
    for /d %%V in ("%%D\6.*") do (
      for /d %%T in ("%%V\msvc*") do (
        if exist "%%T\lib\cmake\Qt6\Qt6Config.cmake" (
          set QT6_DIR=%%T\lib\cmake\Qt6
          set QT_FOUND=1
        )
      )
    )
  )
)

if defined QT_FOUND (
  echo Found Qt6 at %QT6_DIR%
  set CMAKE_CONFIG=%CMAKE_CONFIG% -DQt6_DIR="%QT6_DIR%"
) else (
  echo Qt6 not found, disabling Qt GUI
  set CMAKE_CONFIG=%CMAKE_CONFIG% -DSKIP_QT_GUI=ON
)

cmake "%OMIM_PATH%" ^
  -G "%GENERATOR%" ^
  -DCMAKE_BUILD_TYPE=%CONF% ^
  -DBUILD_DESIGNER=%OPT_DESIGNER% ^
  -DBUILD_STANDALONE=%OPT_STANDALONE% ^
  %CMAKE_CONFIG%

%BUILD_CMD% %OPT_TARGET%

if defined OPT_LAUNCH_BINARY (
  for %%T in (%OPT_TARGET%) do (
    if exist "%%T.exe" "%%T.exe"
  )
)

if defined OPT_COMPILE_DATABASE (
  if exist compile_commands.json copy /y compile_commands.json "%OMIM_PATH%"
)
