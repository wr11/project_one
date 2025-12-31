# Lua 5.5.0 Windows 编译安装指南

本指南将帮助您在 Windows 环境下编译和安装 Lua 5.5.0。

## 方法一：使用 MinGW/MSYS2（推荐）

### 1. 安装 MinGW-w64 或 MSYS2

#### 选项 A：安装 MSYS2（推荐）
1. 访问 https://www.msys2.org/
2. 下载并安装 MSYS2
3. 打开 MSYS2 终端，运行：
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
   ```

#### 选项 B：安装 MinGW-w64
1. 访问 https://www.mingw-w64.org/downloads/
2. 下载 MinGW-w64 安装器
3. 安装时选择：
   - Architecture: x86_64
   - Threads: posix
   - Exception: seh

### 2. 配置环境变量

将 MinGW/MSYS2 的 `bin` 目录添加到系统 PATH：
- MSYS2: `C:\msys64\mingw64\bin`
- MinGW-w64: `C:\mingw64\bin`（根据实际安装路径）

### 3. 编译

在 PowerShell 或命令提示符中运行：

```bash
cd src
make mingw
```

或者使用提供的批处理脚本：

```bash
build.bat
```

### 4. 编译结果

编译成功后，在 `src` 目录下会生成：
- `lua.exe` - Lua 解释器
- `luac.exe` - Lua 编译器
- `lua55.dll` - Lua 动态链接库

## 方法二：使用 Visual Studio

### 1. 安装 Visual Studio

1. 下载 Visual Studio Community（免费）：https://visualstudio.microsoft.com/
2. 安装时选择 "使用 C++ 的桌面开发" 工作负载

### 2. 编译

1. 打开 "Developer Command Prompt for VS" 或 "x64 Native Tools Command Prompt"
2. 导航到 Lua 源码目录
3. 运行：

```bash
build_vs.bat
```

或者手动编译（需要创建 `lua.def` 文件）：

```bash
cd src
cl /MD /O2 /W3 /c /DLUA_BUILD_AS_DLL *.c
link /DLL /OUT:lua55.dll *.obj
link /OUT:lua.exe lua.obj lua55.lib
link /OUT:luac.exe luac.obj lua55.lib
```

## 方法三：使用 WSL（Windows Subsystem for Linux）

如果您安装了 WSL，可以在 Linux 环境中编译：

```bash
wsl
cd /mnt/c/Users/huaweiwei/Desktop/lua-5.5.0
make linux
```

## 安装到系统

编译完成后，可以选择将 Lua 安装到系统：

### MinGW 方式：
```bash
make install INSTALL_TOP=C:/lua
```

### 手动安装：
1. 将 `lua.exe` 和 `luac.exe` 复制到系统 PATH 目录（如 `C:\Windows\System32` 或创建 `C:\lua\bin`）
2. 将 `lua55.dll` 复制到相同目录或系统目录
3. 将头文件（`lua.h`, `luaconf.h`, `lualib.h`, `lauxlib.h`）复制到 `C:\lua\include`
4. 将库文件复制到 `C:\lua\lib`

## 测试安装

编译完成后，测试 Lua 是否正常工作：

```bash
cd src
lua.exe -v
```

应该输出：`Lua 5.5.0`

## 常见问题

### 问题1：找不到 make 命令
- **MinGW**: 确保安装了 `mingw32-make`，或使用 `mingw32-make` 代替 `make`
- **MSYS2**: 使用 `pacman -S make` 安装

### 问题2：编译错误：找不到某些头文件
- 确保所有源文件都在 `src` 目录中
- 检查 `luaconf.h` 配置是否正确

### 问题3：运行时找不到 DLL
- 将 `lua55.dll` 复制到与 `lua.exe` 相同的目录
- 或将其添加到系统 PATH

## 快速开始

如果您已经安装了 MinGW/MSYS2，只需运行：

```bash
build.bat
```

编译完成后，`src` 目录中的 `lua.exe` 就可以使用了！
