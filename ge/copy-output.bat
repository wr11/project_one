@echo off
REM Copy executables and dependencies to server/client directories

cd /d "%~dp0"

REM Check if build directory exists
if not exist "build\bin" (
    echo Error: build\bin directory not found. Please build the project first.
    exit /b 1
)

REM Create directories if they don't exist
if not exist "..\server" mkdir "..\server"
if not exist "..\client" mkdir "..\client"

echo Copying executables...

REM Copy server executable (ge is the short name for GearEngine)
if exist "..\server\ge.exe" (
    echo   [OK] Server executable already in ..\server\ (ge.exe)
) else if exist "build\bin\ge.exe" (
    copy /Y "build\bin\ge.exe" "..\server\" >nul
    echo   [OK] Copied ge.exe to ..\server\
) else (
    echo   [ERROR] Server executable not found
)

REM Copy client executable
if exist "build\bin\gearchat_client.exe" (
    copy /Y "build\bin\gearchat_client.exe" "..\client\" >nul
    echo   [OK] Copied gearchat_client.exe to ..\client\
) else (
    echo   [ERROR] Client executable not found
)

REM Copy DLL files
echo Copying DLL files...
set DLL_COPIED=0

REM Copy libuv.dll from build\libuv
if exist "build\libuv\libuv.dll" (
    copy /Y "build\libuv\libuv.dll" "..\server\" >nul
    copy /Y "build\libuv\libuv.dll" "..\client\" >nul
    echo   [OK] Copied libuv.dll to ..\server\ and ..\client\
    set DLL_COPIED=1
)

REM Copy libbson DLL from build\libbson (if exists)
if exist "build\libbson\lib1.dll" (
    copy /Y "build\libbson\lib1.dll" "..\server\" >nul
    copy /Y "build\libbson\lib1.dll" "..\client\" >nul
    echo   [OK] Copied libbson DLL to ..\server\ and ..\client\
    set DLL_COPIED=1
)

REM Also check build\bin for any DLL files
for %%f in (build\bin\*.dll) do (
    copy /Y "%%f" "..\server\" >nul
    copy /Y "%%f" "..\client\" >nul
    set DLL_COPIED=1
)

if %DLL_COPIED%==0 (
    echo   [WARN] No DLL files found (may be using static linking)
)

echo.
echo Done! Executables are now in:
echo   - ..\server\
echo   - ..\client\
