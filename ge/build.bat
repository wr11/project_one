@echo off
REM GearEngine 构建脚本
REM 自动检测并使用可用的编译器

echo Checking for available compilers...

REM 检查 MSYS2 MinGW64
if exist "C:\msys64\mingw64\bin\gcc.exe" (
    echo Found MSYS2 MinGW64 GCC
    set PATH=C:\msys64\mingw64\bin;%PATH%
    if not exist build mkdir build
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe
    if %ERRORLEVEL% EQU 0 (
        echo Building...
        cmake --build .
    )
    exit /b %ERRORLEVEL%
)

REM 检查系统 PATH 中的 MinGW
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found MinGW GCC in PATH
    if not exist build mkdir build
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
    if %ERRORLEVEL% EQU 0 (
        echo Building...
        cmake --build .
    )
    exit /b %ERRORLEVEL%
)

REM 检查 Visual Studio
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found Visual Studio
    if not exist build mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %ERRORLEVEL% NEQ 0 (
        cmake .. -G "Visual Studio 16 2019" -A x64
    )
    if %ERRORLEVEL% EQU 0 (
        echo Building...
        cmake --build . --config Release
    )
    exit /b %ERRORLEVEL%
)

REM 检查 Clang
where clang >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found Clang
    if not exist build mkdir build
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=clang
    if %ERRORLEVEL% EQU 0 (
        echo Building...
        cmake --build .
    )
    exit /b %ERRORLEVEL%
)

echo ERROR: No C compiler found!
echo Please install one of the following:
echo   - MinGW-w64 (recommended)
echo   - Visual Studio
echo   - Clang
echo.
echo See CMAKE_COMPILER_FIX.md for details.
exit /b 1
