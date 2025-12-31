# GearEngine 构建指南

## 前置要求

- CMake 3.15 或更高版本
- C 编译器（支持 C11 标准）
  - Windows: Visual Studio 2015+ 或 MinGW
  - Linux: GCC 4.8+ 或 Clang 3.3+
  - macOS: Xcode Command Line Tools

## Windows 构建

### 使用 Visual Studio

1. 打开 PowerShell 或命令提示符，进入项目目录：
```powershell
cd gearengine
mkdir build
cd build
```

2. 配置 CMake（使用 Visual Studio 2019 示例）：
```powershell
cmake .. -G "Visual Studio 16 2019" -A x64
```

3. 编译：
```powershell
cmake --build . --config Release
```

4. 可执行文件位于：`build\bin\Release\gearengine.exe`

### 使用 MinGW

```powershell
cd gearengine
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## Linux 构建

```bash
cd gearengine
mkdir build && cd build
cmake ..
make -j$(nproc)
```

可执行文件位于：`build/bin/gearengine`

## macOS 构建

```bash
cd gearengine
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## 构建选项

### libuv 选项

- `LIBUV_BUILD_SHARED`: 构建共享库（默认：ON）
- `LIBUV_BUILD_TESTS`: 构建测试（默认：OFF）

### libbson 选项

注意：libbson 可能需要一些额外的配置。如果遇到构建错误，可以尝试：

1. 设置版本信息：
```bash
cmake .. -DPROJECT_VERSION=1.0.0
```

2. 如果 libbson 构建失败，可能需要先单独构建 libbson：
```bash
cd libbson
mkdir build && cd build
cmake .. -DENABLE_STATIC=ON -DENABLE_SHARED=ON
cmake --build .
```

## 常见问题

### libbson 构建错误

如果遇到 `mongo_setting` 或 `mongo-c-driver` 相关的错误，可能需要：

1. 检查 libbson 的 CMakeLists.txt 是否完整
2. 确保所有必要的源文件都存在
3. 尝试使用预编译的 libbson 库

### Lua 链接错误

确保 Lua 源码目录中包含所有必要的 `.c` 文件。如果某些文件缺失，可以从官方 Lua 源码获取。

### Windows 链接错误

如果遇到链接错误，确保链接了必要的系统库：
- `ws2_32` (Windows Sockets)
- `psapi` (Process Status API)
- `iphlpapi` (IP Helper API)
- `userenv` (User Environment)

## 调试构建

启用调试信息：
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## 安装（可选）

```bash
cmake --install . --prefix /usr/local
```
