# Lua 5.5.0 Windows 快速编译指南

## 🚀 快速开始

### 最简单的方法：安装 MSYS2

1. **下载并安装 MSYS2**
   - 访问：https://www.msys2.org/
   - 下载并运行安装程序
   - 安装完成后，打开 **MSYS2 UCRT64** 终端（不是 MSYS）

2. **安装编译工具**
   ```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
   ```

3. **编译 Lua**
   ```bash
   cd /c/Users/huaweiwei/Desktop/lua-5.5.0/src
   make mingw
   ```

4. **测试**
   ```bash
   ./lua.exe -v
   ```

## 📋 其他编译方法

### 方法一：使用批处理脚本（如果已安装 MinGW）

在 PowerShell 或命令提示符中运行：

```powershell
.\build.bat
```

或 PowerShell 脚本：

```powershell
.\build.ps1
```

### 方法二：使用 Visual Studio

1. 安装 Visual Studio Community（免费）
2. 打开 "x64 Native Tools Command Prompt for VS"
3. 运行：
   ```bash
   build_vs.bat
   ```

### 方法三：手动编译（MinGW）

如果已安装 MinGW，在 `src` 目录中运行：

```bash
make mingw
```

## 📦 编译输出

编译成功后，在 `src` 目录下会生成：

- `lua.exe` - Lua 解释器（运行 Lua 脚本）
- `luac.exe` - Lua 编译器（编译 Lua 脚本）
- `lua55.dll` - Lua 动态链接库

## 🔧 安装到系统（可选）

### 方式一：添加到 PATH

1. 创建目录：`C:\lua\bin`
2. 将 `lua.exe`、`luac.exe` 和 `lua55.dll` 复制到 `C:\lua\bin`
3. 将 `C:\lua\bin` 添加到系统 PATH 环境变量

### 方式二：使用 make install

在 MSYS2 终端中：

```bash
cd /c/Users/huaweiwei/Desktop/lua-5.5.0
make install INSTALL_TOP=/c/lua
```

## ✅ 验证安装

```bash
lua -v
```

应该输出：`Lua 5.5.0`

## 🆘 常见问题

### Q: 找不到 make 命令
**A:** 
- MSYS2: 运行 `pacman -S make`
- MinGW: 使用 `mingw32-make` 代替 `make`

### Q: 找不到 gcc 命令
**A:** 
- 确保已安装 MinGW/MSYS2
- 检查 PATH 环境变量是否包含编译器路径
- MSYS2: `C:\msys64\ucrt64\bin` 或 `C:\msys64\mingw64\bin`

### Q: 运行时找不到 lua55.dll
**A:** 
- 将 `lua55.dll` 复制到与 `lua.exe` 相同的目录
- 或将其添加到系统 PATH

### Q: 编译错误
**A:** 
- 确保所有源文件都在 `src` 目录中
- 检查编译器版本是否支持 C99 标准
- 查看详细错误信息

## 📚 更多信息

详细编译说明请查看 `BUILD_WINDOWS.md`

## 💡 推荐配置

**最佳实践：**
1. 使用 MSYS2（最简单、最稳定）
2. 使用 UCRT64 环境（Windows 11/10 推荐）
3. 编译后测试：`./lua.exe -e "print('Hello, Lua!')"`
