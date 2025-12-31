@echo off
REM Stress test runner script for Windows

cd /d "%~dp0"

REM Try multiple possible paths
set STRESS_CLIENT=..\ge\build\bin\stress_client.exe
set STRESS_DIR=..\ge\build\bin
if not exist "%STRESS_CLIENT%" (
    set STRESS_CLIENT=ge\build\bin\stress_client.exe
    set STRESS_DIR=ge\build\bin
)
if not exist "%STRESS_CLIENT%" (
    echo Error: stress_client.exe not found. Please build the project first.
    echo.
    echo To build on Windows MSYS2:
    echo   1. Open MSYS2 terminal
    echo   2. cd ge
    echo   3. ./build-msys2.sh
    echo.
    echo Or manually build:
    echo   cd ge\build
    echo   cmake ..
    echo   cmake --build .
    echo.
    exit /b 1
)

REM Copy libuv.dll to bin directory if needed
set LIBUV_DLL=..\ge\build\libuv\libuv.dll
if not exist "%LIBUV_DLL%" (
    set LIBUV_DLL=ge\build\libuv\libuv.dll
)
if exist "%LIBUV_DLL%" (
    if not exist "%STRESS_DIR%\libuv.dll" (
        copy /Y "%LIBUV_DLL%" "%STRESS_DIR%\" >nul
        echo [OK] Copied libuv.dll to %STRESS_DIR%
    )
) else (
    echo [WARN] libuv.dll not found in build directory
)

echo === GearEngine Stress Test Runner ===
echo.

REM Test 1: Basic connection test
echo Test 1: Basic Connection Test (100 clients, 10 messages each)
echo ------------------------------------------------------------
"%STRESS_CLIENT%" 100 10 100 localhost 8080
echo.

REM Test 2: High concurrency
echo Test 2: High Concurrency Test (500 clients, 5 messages each)
echo ------------------------------------------------------------
timeout /t 2 /nobreak >nul
"%STRESS_CLIENT%" 500 5 200 localhost 8080
echo.

REM Test 3: Message throughput
echo Test 3: Message Throughput Test (100 clients, 100 messages, fast)
echo -------------------------------------------------------------------
timeout /t 2 /nobreak >nul
"%STRESS_CLIENT%" 100 100 10 localhost 8080
echo.

echo All tests completed!
