@echo off
REM Lua 5.5.0 Visual Studio 编译脚本
REM 需要在 Visual Studio Developer Command Prompt 中运行

echo ========================================
echo Lua 5.5.0 Visual Studio 编译脚本
echo ========================================
echo.

cd src

REM 编译核心库和目标文件
echo [1/3] 编译核心库...
cl /MD /O2 /W3 /c /DLUA_BUILD_AS_DLL *.c /Fd:lua.pdb
if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    cd ..
    exit /b 1
)

REM 创建 DLL
echo [2/3] 创建动态链接库...
link /DLL /OUT:lua55.dll *.obj /DEF:lua.def 2>nul
if %ERRORLEVEL% neq 0 (
    echo 注意：未找到 lua.def，尝试不使用 DEF 文件...
    link /DLL /OUT:lua55.dll *.obj
    if %ERRORLEVEL% neq 0 (
        echo DLL 创建失败！
        cd ..
        exit /b 1
    )
)

REM 创建 lua.exe
echo [3/3] 创建 lua.exe...
link /OUT:lua.exe lua.obj lua55.lib
if %ERRORLEVEL% neq 0 (
    echo lua.exe 创建失败！
    cd ..
    exit /b 1
)

REM 创建 luac.exe
echo [4/4] 创建 luac.exe...
link /OUT:luac.exe luac.obj lua55.lib
if %ERRORLEVEL% neq 0 (
    echo luac.exe 创建失败！
    cd ..
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo ========================================
echo 生成的文件：
echo   - lua.exe (Lua 解释器)
echo   - luac.exe (Lua 编译器)
echo   - lua55.dll (Lua 动态库)
echo   - lua55.lib (导入库)
echo.
echo 文件位置: src\
cd ..
pause
