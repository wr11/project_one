# GearEngine 快速开始指南

## 项目概述

GearEngine 是一个整合了 libuv（网络驱动）、Lua（脚本语言）和 libbson（数据序列化）的服务器引擎。

## 快速开始

### 1. 构建项目

```bash
cd gearengine
mkdir build && cd build
cmake ..
cmake --build .
```

### 2. 运行服务器

```bash
# 使用默认端口 8080
./bin/gearengine

# 指定端口和脚本
./bin/gearengine 8888 ../scripts/example.lua
```

### 3. 测试连接

使用 telnet 或 nc 连接到服务器：

```bash
telnet localhost 8080
# 或
nc localhost 8080
```

输入一些文本，服务器会回显你的消息。

## 编写你的第一个脚本

创建 `scripts/my_server.lua`：

```lua
function on_server_start()
    gear.log("我的服务器启动了！")
end

function on_client_connect(client)
    gear.log("新客户端连接")
    gear.send_data(client, "你好！欢迎连接到服务器。\n")
end

function on_client_data(client, data)
    gear.log("收到: " .. data)
    gear.send_data(client, "你说了: " .. data)
end

function on_client_disconnect(client)
    gear.log("客户端断开")
end
```

运行：
```bash
./bin/gearengine 8080 scripts/my_server.lua
```

## API 参考

### gear.send_data(client, data)
发送字符串数据到客户端。

### gear.send_bson(client, bson_data)
发送 BSON 二进制数据到客户端。

### gear.close_client(client)
关闭客户端连接。

### gear.get_client_count()
获取当前连接的客户端数量。

### gear.log(message)
输出日志消息。

## 回调函数

- `on_server_start()` - 服务器启动时调用
- `on_client_connect(client)` - 新客户端连接时调用
- `on_client_data(client, data)` - 收到客户端数据时调用
- `on_client_disconnect(client)` - 客户端断开时调用
- `on_server_stop()` - 服务器停止时调用

## 下一步

- 查看 `scripts/example.lua` 了解完整示例
- 阅读 `README.md` 了解详细文档
- 查看 `BUILD.md` 了解构建选项
