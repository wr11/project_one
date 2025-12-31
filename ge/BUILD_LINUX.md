# Linux 构建指南

本指南介绍如何在 Linux 系统上构建和运行 GearEngine 聊天服务器。

## 系统要求

- Linux 发行版（Ubuntu, Debian, Fedora, CentOS 等）
- GCC 编译器（支持 C11）
- CMake 3.15 或更高版本
- Make 构建工具

## 安装依赖

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake git
```

### Fedora/CentOS/RHEL

```bash
sudo dnf install gcc cmake make git
# 或对于较旧的版本
sudo yum install gcc cmake make git
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake git
```

## 构建项目

### 方法 1: 使用构建脚本（推荐）

```bash
cd gearengine
chmod +x build-linux.sh
./build-linux.sh
```

### 方法 2: 手动构建

```bash
cd gearengine
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 运行服务器

```bash
# 使用默认端口 8080
./build/bin/gearengine 8080 server/chat_server.lua

# 使用自定义端口
./build/bin/gearengine 8888 server/chat_server.lua
```

## 运行客户端

```bash
# 连接到默认服务器 (localhost:8080)
./build/bin/gearchat_client

# 连接到指定服务器和端口
./build/bin/gearchat_client localhost 8080

# 使用自定义 Lua 脚本
./build/bin/gearchat_client localhost 8080 client/chat_client.lua
```

## 故障排除

### 编译错误：找不到 pthread

如果遇到链接错误，确保安装了 pthread 库：

```bash
# Ubuntu/Debian
sudo apt install libpthread-stubs0-dev

# Fedora/CentOS
sudo dnf install glibc-devel
```

### TTY 初始化失败

如果客户端无法读取标准输入，确保在终端中运行（而不是通过管道或重定向）：

```bash
# 正确的方式
./build/bin/gearchat_client

# 错误的方式（不要这样做）
echo "test" | ./build/bin/gearchat_client
```

### 权限问题

如果遇到端口绑定权限问题（例如绑定到 1024 以下的端口），可以使用：

```bash
# 使用 sudo（不推荐用于开发）
sudo ./build/bin/gearengine 80 server/chat_server.lua

# 或使用 1024 以上的端口（推荐）
./build/bin/gearengine 8080 server/chat_server.lua
```

## 测试

1. 启动服务器：
   ```bash
   ./build/bin/gearengine 8080 server/chat_server.lua
   ```

2. 在另一个终端启动客户端：
   ```bash
   ./build/bin/gearchat_client localhost 8080
   ```

3. 发送消息测试聊天功能

## 性能优化

对于生产环境，可以使用 Release 模式构建：

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 跨平台兼容性

代码已经过测试，支持：
- ✅ Linux (Ubuntu, Debian, Fedora, CentOS, Arch)
- ✅ Windows (MSYS2/MinGW)
- ✅ macOS (理论上支持，需要测试)

所有平台使用相同的代码库，通过条件编译处理平台差异。
