# GearEngine 构建脚本 (PowerShell)
# 自动检测并使用可用的编译器

Write-Host "Checking for available compilers..." -ForegroundColor Cyan

# 检查 MSYS2 MinGW64
$msys2Gcc = "C:\msys64\mingw64\bin\gcc.exe"
if (Test-Path $msys2Gcc) {
    Write-Host "Found MSYS2 MinGW64 GCC" -ForegroundColor Green
    $env:Path = "C:\msys64\mingw64\bin;" + $env:Path
    if (-not (Test-Path build)) {
        New-Item -ItemType Directory -Path build | Out-Null
    }
    Set-Location build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=$msys2Gcc
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Building..." -ForegroundColor Green
        cmake --build .
    }
    exit $LASTEXITCODE
}

# 检查系统 PATH 中的 MinGW
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if ($gcc) {
    Write-Host "Found MinGW GCC in PATH" -ForegroundColor Green
    if (-not (Test-Path build)) {
        New-Item -ItemType Directory -Path build | Out-Null
    }
    Set-Location build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Building..." -ForegroundColor Green
        cmake --build .
    }
    exit $LASTEXITCODE
}

# 检查 Visual Studio
$cl = Get-Command cl -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "Found Visual Studio" -ForegroundColor Green
    if (-not (Test-Path build)) {
        New-Item -ItemType Directory -Path build | Out-Null
    }
    Set-Location build
    cmake .. -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        cmake .. -G "Visual Studio 16 2019" -A x64
    }
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Building..." -ForegroundColor Green
        cmake --build . --config Release
    }
    exit $LASTEXITCODE
}

# 检查 Clang
$clang = Get-Command clang -ErrorAction SilentlyContinue
if ($clang) {
    Write-Host "Found Clang" -ForegroundColor Green
    if (-not (Test-Path build)) {
        New-Item -ItemType Directory -Path build | Out-Null
    }
    Set-Location build
    cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=clang
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Building..." -ForegroundColor Green
        cmake --build .
    }
    exit $LASTEXITCODE
}

Write-Host "ERROR: No C compiler found!" -ForegroundColor Red
Write-Host "Please install one of the following:" -ForegroundColor Yellow
Write-Host "  - MinGW-w64 (recommended)"
Write-Host "  - Visual Studio"
Write-Host "  - Clang"
Write-Host ""
Write-Host "See CMAKE_COMPILER_FIX.md for details." -ForegroundColor Yellow
exit 1
