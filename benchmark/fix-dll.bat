@echo off
REM Quick fix script to copy DLL files for stress test client

cd /d "%~dp0"

echo Copying DLL files for stress test client...

REM Find stress_client.exe location
set STRESS_DIR=..\ge\build\bin
if not exist "%STRESS_DIR%\stress_client.exe" (
    set STRESS_DIR=ge\build\bin
)

if not exist "%STRESS_DIR%\stress_client.exe" (
    echo Error: stress_client.exe not found
    exit /b 1
)

REM Copy libuv.dll
set LIBUV_SRC=..\ge\build\libuv\libuv.dll
if not exist "%LIBUV_SRC%" (
    set LIBUV_SRC=ge\build\libuv\libuv.dll
)

if exist "%LIBUV_SRC%" (
    copy /Y "%LIBUV_SRC%" "%STRESS_DIR%\" >nul
    echo [OK] Copied libuv.dll to %STRESS_DIR%
) else (
    echo [ERROR] libuv.dll not found in build directory
    exit /b 1
)

echo.
echo Done! DLL files are now in: %STRESS_DIR%
