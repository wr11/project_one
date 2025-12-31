# Lua 5.5.0 Windows PowerShell 编译脚本
# 支持 MinGW/MSYS2 和 Visual Studio 两种编译方式

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Lua 5.5.0 Windows 编译脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查 MinGW/MSYS2
$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
if ($gccPath) {
    Write-Host "[检测到] MinGW/MSYS2 GCC 编译器: $($gccPath.Source)" -ForegroundColor Green
    Write-Host ""
    
    # 检查 make 命令
    $makeCmd = Get-Command make -ErrorAction SilentlyContinue
    if (-not $makeCmd) {
        $mingwMake = Get-Command mingw32-make -ErrorAction SilentlyContinue
        if ($mingwMake) {
            Write-Host "[使用] mingw32-make" -ForegroundColor Yellow
            $makeCmd = "mingw32-make"
        } else {
            Write-Host "[错误] 未找到 make 或 mingw32-make 命令！" -ForegroundColor Red
            Write-Host ""
            Write-Host "请安装 make 工具：" -ForegroundColor Yellow
            Write-Host "  - MSYS2: pacman -S make" -ForegroundColor White
            Write-Host "  - MinGW: 确保安装了 make 工具" -ForegroundColor White
            exit 1
        }
    } else {
        $makeCmd = "make"
    }
    
    Write-Host "开始使用 MinGW 编译..." -ForegroundColor Yellow
    Push-Location src
    
    try {
        & $makeCmd mingw
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "========================================" -ForegroundColor Green
            Write-Host "编译成功！" -ForegroundColor Green
            Write-Host "========================================" -ForegroundColor Green
            Write-Host ""
            Write-Host "生成的文件：" -ForegroundColor Cyan
            Write-Host "  - lua.exe (Lua 解释器)" -ForegroundColor White
            Write-Host "  - luac.exe (Lua 编译器)" -ForegroundColor White
            Write-Host "  - lua55.dll (Lua 动态库)" -ForegroundColor White
            Write-Host ""
            Write-Host "文件位置: src\" -ForegroundColor Yellow
            
            # 测试编译结果
            if (Test-Path "lua.exe") {
                Write-Host ""
                Write-Host "测试 Lua 版本：" -ForegroundColor Cyan
                & .\lua.exe -v
            }
        } else {
            Write-Host ""
            Write-Host "编译失败，请检查错误信息" -ForegroundColor Red
            exit 1
        }
    } finally {
        Pop-Location
    }
    exit 0
}

# 检查 Visual Studio
$clPath = Get-Command cl -ErrorAction SilentlyContinue
if ($clPath) {
    Write-Host "[检测到] Visual Studio 编译器: $($clPath.Source)" -ForegroundColor Green
    Write-Host ""
    Write-Host "注意：Visual Studio 编译需要使用 build_vs.bat" -ForegroundColor Yellow
    Write-Host "或在 Developer Command Prompt 中运行此脚本" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# 未找到编译器
Write-Host "[错误] 未找到任何 C 编译器！" -ForegroundColor Red
Write-Host ""
Write-Host "请安装以下工具之一：" -ForegroundColor Yellow
Write-Host ""
Write-Host "1. MinGW-w64 或 MSYS2 (推荐)" -ForegroundColor Cyan
Write-Host "   下载地址: https://www.mingw-w64.org/downloads/" -ForegroundColor White
Write-Host "   或 https://www.msys2.org/" -ForegroundColor White
Write-Host ""
Write-Host "2. Visual Studio (包含 MSVC 编译器)" -ForegroundColor Cyan
Write-Host "   下载地址: https://visualstudio.microsoft.com/" -ForegroundColor White
Write-Host ""
Write-Host "3. 使用 WSL (Windows Subsystem for Linux)" -ForegroundColor Cyan
Write-Host "   在 WSL 中运行: make linux" -ForegroundColor White
Write-Host ""
Read-Host "按 Enter 键退出"
