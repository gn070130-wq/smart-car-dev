@echo off
setlocal
cd /d "%~dp0"

set "VS=D:\VS-community"
if not exist "%VS%\Common7\Tools\VsDevCmd.bat" (
  echo VsDevCmd not found at %VS%
  exit /b 1
)

call "%VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
set "CMAKE=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "%CMAKE%" set "CMAKE=cmake"
if not exist "%NINJA%" set "NINJA=ninja"

if not exist build mkdir build
"%CMAKE%" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
"%CMAKE%" --build build
if errorlevel 1 exit /b 1
echo.
echo OK: build\board_rt1064_stub_host.exe
build\board_rt1064_stub_host.exe
endlocal

