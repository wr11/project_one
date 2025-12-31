@echo off
REM Lua 5.5.0 Windows 编译脚本
REM 支持 MinGW/MSYS2 和 Visual Studio 两种编译方式

echo ========================================
echo Lua 5.5.0 Windows 编译脚本
echo ========================================
echo.

REM 检查 MinGW/MSYS2
where gcc >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo [检测到] MinGW/MSYS2 GCC 编译器
    echo.
    echo 开始使用 MinGW 编译...
    cd src
    make mingw
    if %ERRORLEVEL% == 0 (
        echo.
        echo ========================================
        echo 编译成功！
        echo ========================================
        echo 生成的文件：
        echo   - lua.exe (Lua 解释器)
        echo   - luac.exe (Lua 编译器)
        echo   - lua55.dll (Lua 动态库)
        echo.
        echo 文件位置: src\
        cd ..
        exit /b 0
    ) else (
        echo.
        echo 编译失败，请检查错误信息
        cd ..
        exit /b 1
    )
)

REM 检查 Visual Studio
where cl >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo [检测到] Visual Studio 编译器
    echo.
    echo 注意：Visual Studio 编译需要手动设置环境
    echo 请使用 "Developer Command Prompt for VS" 运行此脚本
    echo 或者运行 build_vs.bat
    echo.
    exit /b 1
)

echo [错误] 未找到任何 C 编译器！
echo.
echo 请安装以下工具之一：
echo   1. MinGW-w64 或 MSYS2 (推荐)
echo     下载地址: https://www.mingw-w64.org/downloads/
echo     或 https://www.msys2.org/
echo.
echo   2. Visual Studio (包含 MSVC 编译器)
echo     下载地址: https://visualstudio.microsoft.com/
echo.
echo   3. 使用 WSL (Windows Subsystem for Linux)
echo     在 WSL 中运行: make linux
echo.
pause
exit /b 1
