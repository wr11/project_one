# 使用 MSYS2 构建 GearEngine

## 前置要求

确保 MSYS2 已安装并配置好 MinGW-w64 工具链。

## 安装必要的工具

在 MSYS2 终端中运行：

```bash
# 更新包数据库
pacman -Syu

# 安装编译工具链
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S git
```

## 构建步骤

### 方法 1：在 MSYS2 MinGW64 终端中构建（推荐）

1. 打开 **MSYS2 MinGW64** 终端（不是 MSYS2 MSYS 终端）

2. 进入项目目录：
```bash
cd /c/Users/huaweiwei/Desktop/my_proj/gearengine
```

3. 创建构建目录：
```bash
mkdir -p build
cd build
```

4. 配置 CMake：
```bash
cmake .. -G "MinGW Makefiles"
```

5. 编译：
```bash
cmake --build .
```

### 方法 2：在 Windows PowerShell/CMD 中使用 MSYS2 工具链

如果你想在 Windows 命令行中使用 MSYS2 的编译器：

1. 将 MSYS2 MinGW64 bin 目录添加到 PATH：
   - 路径通常是：`C:\msys64\mingw64\bin`

2. 在 PowerShell 中临时添加 PATH：
```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
cd gearengine
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe
cmake --build .
```

3. 或者在 CMD 中：
```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
cd gearengine
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe
cmake --build .
```

## 验证安装

在 MSYS2 MinGW64 终端中运行：

```bash
gcc --version
cmake --version
make --version
```

应该能看到版本信息。

## 常见问题

### 问题：找不到 cmake

如果 `cmake --version` 失败，需要安装：
```bash
pacman -S mingw-w64-x86_64-cmake
```

### 问题：PATH 未设置

确保在 **MSYS2 MinGW64** 终端中运行，而不是 MSYS2 MSYS 终端。

### 问题：权限错误

如果在 Windows 文件系统上遇到权限问题，尝试在 MSYS2 终端中使用 `/c/` 路径格式。

## 快速构建脚本

创建 `build-msys2.sh`：

```bash
#!/bin/bash
set -e

echo "Building GearEngine with MSYS2..."

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

cmake .. -G "MinGW Makefiles"

cmake --build .

echo "Build complete! Executable is in build/bin/"
```

使用方法：
```bash
chmod +x build-msys2.sh
./build-msys2.sh
```
