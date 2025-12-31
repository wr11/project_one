# GearEngine - 基于 libuv、Lua 和 libbson 的服务器引擎

GearEngine 是一个高性能的异步服务器引擎，整合了以下三个强大的库：

- **libuv**: 提供跨平台的异步 I/O 和事件循环
- **Lua**: 提供灵活的脚本语言环境
- **libbson**: 提供高效的 BSON 数据序列化/反序列化

## 项目结构

```
gearengine/
├── CMakeLists.txt          # 主构建文件
├── libuv/                  # libuv 源码
├── libbson/                # libbson 源码
├── lua/                    # Lua 源码
├── src/                    # 服务器引擎源码
│   ├── CMakeLists.txt
│   ├── main.c             # 主程序入口
│   ├── server.h           # 服务器头文件
│   ├── server.c           # 服务器核心实现
│   ├── lua_bindings.c     # Lua 绑定
│   └── bson_handler.c     # BSON 处理
└── scripts/               # Lua 脚本目录
    └── example.lua        # 示例脚本
```

## 构建说明

### Windows (使用 CMake)

#### 方法 1：使用自动构建脚本（推荐）

**PowerShell:**
```powershell
cd gearengine
.\build.ps1
```

**CMD:**
```cmd
cd gearengine
build.bat
```

脚本会自动检测可用的编译器（MinGW、Visual Studio 或 Clang）并配置构建。

#### 方法 2：使用 MSYS2（推荐）

如果你安装了 MSYS2：

1. **打开 MSYS2 MinGW64 终端**（不是 MSYS2 MSYS 终端）

2. 进入项目目录：
```bash
cd /c/Users/huaweiwei/Desktop/my_proj/gearengine
```

3. 运行构建脚本：
```bash
chmod +x build-msys2.sh
./build-msys2.sh
```

或者手动构建：
```bash
mkdir -p build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

**详细说明请查看 `BUILD_MSYS2.md`**

#### 方法 3：手动配置

**使用 MinGW:**
```bash
cd gearengine
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
cmake --build .
```

**使用 Visual Studio:**
```bash
cd gearengine
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**如果遇到编译器未找到的错误，请查看 `CMAKE_COMPILER_FIX.md` 获取详细解决方案。**

### Linux/macOS

```bash
cd gearengine
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 基本用法

```bash
# 使用默认端口 8080
./gearengine

# 指定端口
./gearengine 8888

# 指定端口和 Lua 脚本
./gearengine 8888 scripts/example.lua
```

## Lua API

服务器引擎提供了以下 Lua API：

### gear.send_data(client, data)
向客户端发送字符串数据。

**参数：**
- `client`: 客户端对象（由回调函数提供）
- `data`: 要发送的字符串数据

**返回：** 成功返回 `true`，失败返回 `false` 和错误信息

### gear.send_bson(client, bson_data)
向客户端发送 BSON 数据。

**参数：**
- `client`: 客户端对象
- `bson_data`: BSON 二进制数据（字符串）

### gear.close_client(client)
关闭客户端连接。

**参数：**
- `client`: 客户端对象

### gear.get_client_count()
获取当前连接的客户端数量。

**返回：** 客户端数量（整数）

### gear.log(message)
输出日志消息。

**参数：**
- `message`: 日志消息字符串

## Lua 回调函数

你可以在 Lua 脚本中定义以下回调函数：

### on_server_start()
服务器启动时调用。

### on_client_connect(client)
新客户端连接时调用。

**参数：**
- `client`: 客户端对象

### on_client_data(client, data)
收到客户端数据时调用。

**参数：**
- `client`: 客户端对象
- `data`: 接收到的数据（字符串）

### on_client_disconnect(client)
客户端断开连接时调用。

**参数：**
- `client`: 客户端对象

### on_server_stop()
服务器停止时调用。

## 示例脚本

查看 `scripts/example.lua` 了解完整的使用示例。

## 架构说明

### 网络层（libuv）
- 使用 libuv 的事件循环处理所有网络 I/O
- 支持异步 TCP 连接和数据传输
- 自动处理连接管理和资源清理

### 脚本层（Lua）
- 通过 Lua 脚本定义服务器行为
- 提供灵活的 API 供脚本调用
- 支持热更新（重新加载脚本）

### 数据层（libbson）
- 使用 BSON 格式进行高效的数据序列化
- 支持复杂数据结构的传输
- 提供数据验证功能

## 开发计划

- [ ] 添加更多 Lua API（文件操作、定时器等）
- [ ] 支持 UDP 协议
- [ ] 添加 SSL/TLS 支持
- [ ] 性能优化和监控
- [ ] 更完善的错误处理
- [ ] 配置文件和日志系统

## 许可证

本项目使用的库：
- libuv: MIT License
- Lua: MIT License
- libbson: Apache License 2.0

## 贡献

欢迎提交 Issue 和 Pull Request！
