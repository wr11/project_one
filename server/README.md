# Chat Server

聊天服务器目录。

## 文件说明

- `chat_server.lua` - 聊天服务器 Lua 脚本
- `ge.exe` (Windows) 或 `ge` (Linux) - 服务器可执行文件（构建后生成）
  - `ge` 是 GearEngine（齿轮引擎）的简称
- `libuv.dll` (Windows) - libuv 动态库（构建后自动复制）

## 使用方法

### Windows (MSYS2)

```bash
cd server
./ge.exe 8080 chat_server.lua
```

或者如果使用默认脚本：

```bash
cd server
./ge.exe 8080
```

### Linux

```bash
cd server
./ge 8080 chat_server.lua
```

或者如果使用默认脚本：

```bash
cd server
./ge 8080
```

## 故障排除

### 找不到 libuv.dll

如果提示找不到 `libuv.dll`，请运行：

```bash
cd gearengine
./fix-dll.bat  # Windows
# 或
./fix-dll.sh   # Linux/MSYS2
```

或者手动复制：

```bash
# Windows
copy gearengine\build\libuv\libuv.dll server\
copy gearengine\build\libuv\libuv.dll client\

# Linux/MSYS2
cp gearengine/build/libuv/libuv.dll server/
cp gearengine/build/libuv/libuv.dll client/
```

## 关于 ge

`ge` 是 GearEngine（齿轮引擎）的简称，表示一个基于 libuv、Lua 和 libbson 的服务器引擎：
- **ge** = **G**ear**E**ngine
- 使用 libuv 提供异步网络驱动
- 使用 Lua 提供脚本语言环境
- 使用 libbson 提供网络协议数据的解包和压包功能

## 功能特性

- 多客户端同时连接
- 消息广播
- 客户端名称管理
- 命令处理系统
- 用户列表查询
