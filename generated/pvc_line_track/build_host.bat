@echo off
setlocal
cd /d "%~dp0"

set "VCVARS=D:\VS-community\VC\Auxiliary\Build\vcvars64.bat"
set "CMAKE=D:\VS-community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

where gcc >nul 2>&1
if %ERRORLEVEL%==0 (
  echo [build] using gcc
  gcc -std=c11 -Wall -Wextra -DPVC_TRACK_HOST_DEMO=1 -I. ^
    track_pvc.c element_fsm.c control_steer.c app_track_main.c hal_stub.c ^
    -o pvc_track_host.exe -lm
  if errorlevel 1 exit /b 1
  pvc_track_host.exe
  echo [ok] gcc build and run finished
  exit /b 0
)

if exist "%VCVARS%" if exist "%CMAKE%" (
  echo [build] using VS + CMake
  call "%VCVARS%" >nul
  "%CMAKE%" -B build -G Ninja -DPVC_TRACK_HOST_DEMO=ON
  if errorlevel 1 exit /b 1
  "%CMAKE%" --build build --config Release
  if errorlevel 1 exit /b 1
  if exist build\pvc_track_host.exe (
    build\pvc_track_host.exe
  ) else if exist build\Release\pvc_track_host.exe (
    build\Release\pvc_track_host.exe
  )
  echo [ok] VS build and run finished
  exit /b 0
)

echo [fail] need gcc or VS CMake. Checked: %CMAKE%
exit /b 1
