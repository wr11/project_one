# Chat Client

聊天客户端目录。

## 文件说明

- `chat_client.lua` - 客户端 Lua 脚本
- `gearchat_client.exe` (Windows) 或 `gearchat_client` (Linux) - 客户端可执行文件（构建后生成）
- `libuv.dll` (Windows) - libuv 动态库（构建后自动复制）

## 使用方法

### Windows (MSYS2)

```bash
cd client
./gearchat_client.exe localhost 8080 chat_client.lua
```

或者如果使用默认脚本：

```bash
cd client
./gearchat_client.exe localhost 8080
```

### Linux

```bash
cd client
./gearchat_client localhost 8080 chat_client.lua
```

或者如果使用默认脚本：

```bash
cd client
./gearchat_client localhost 8080
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

## 客户端命令

- `/help` - 显示帮助
- `/name <名字>` - 更改名字
- `/quit` - 断开连接
- `/exit` - 退出客户端
