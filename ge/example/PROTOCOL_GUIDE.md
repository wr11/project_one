# GearEngine 协议使用指南

本指南说明如何与修复后的 GearEngine 服务器通信。

## 协议概述

GearEngine 使用**基于长度前缀的二进制协议**,确保 TCP 粘包/拆包问题得到正确处理。

### 消息格式

```
+-------------------+-------------------------+
| 4字节长度 (小端)   | 消息体 (变长)            |
+-------------------+-------------------------+
| 0x0A 0x00 0x00 0x00 | H e l l o W o r l d    |
+-------------------+-------------------------+
    ↑                      ↑
    长度=10                实际数据 (10字节)
```

- **长度字段**: 32位无符号整数,小端字节序 (Little-Endian)
- **消息体**: 实际数据,长度由前4字节指定
- **最大长度**: `CLIENT_BUFFER_SIZE - 4` = 4092 字节

## 客户端实现示例

### Python 客户端

使用提供的示例客户端:

```bash
# 安装 Python (如果还没有)
python --version  # 需要 Python 3.6+

# 运行示例客户端
cd ge/example
python protocol_example_client.py [host] [port]

# 默认连接到 127.0.0.1:8080
python protocol_example_client.py
```

**核心代码:**
```python
import socket
import struct

def send_message(sock, message):
    data = message.encode('utf-8')
    length = struct.pack('<I', len(data))  # 小端4字节长度
    sock.sendall(length + data)

def receive_message(sock):
    # 1. 读取4字节长度
    length_bytes = sock.recv(4)
    msg_length = struct.unpack('<I', length_bytes)[0]

    # 2. 读取消息体
    data = b''
    while len(data) < msg_length:
        chunk = sock.recv(msg_length - len(data))
        if not chunk:
            break
        data += chunk

    return data.decode('utf-8')
```

---

### C/C++ 客户端

```c
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>  // 或 winsock2.h (Windows)

// 发送消息
int send_message(int sock, const char* message) {
    uint32_t len = strlen(message);

    // 发送长度 (小端)
    uint32_t net_len = len;  // 假设本机是小端
    if (send(sock, &net_len, 4, 0) != 4) {
        return -1;
    }

    // 发送消息体
    if (send(sock, message, len, 0) != len) {
        return -1;
    }

    return 0;
}

// 接收消息
int receive_message(int sock, char* buffer, size_t buffer_size) {
    // 1. 读取长度
    uint32_t msg_len;
    if (recv(sock, &msg_len, 4, MSG_WAITALL) != 4) {
        return -1;
    }

    if (msg_len > buffer_size - 1) {
        return -1;  // 缓冲区太小
    }

    // 2. 读取消息
    if (recv(sock, buffer, msg_len, MSG_WAITALL) != msg_len) {
        return -1;
    }

    buffer[msg_len] = '\0';  // 添加终止符
    return msg_len;
}
```

---

### JavaScript/Node.js 客户端

```javascript
const net = require('net');

class GearEngineClient {
    constructor(host = '127.0.0.1', port = 8080) {
        this.host = host;
        this.port = port;
        this.socket = null;
        this.buffer = Buffer.alloc(0);
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection(this.port, this.host, () => {
                console.log('✅ Connected');
                resolve();
            });

            this.socket.on('data', (data) => this.onData(data));
            this.socket.on('error', (err) => reject(err));
        });
    }

    sendMessage(message) {
        const data = Buffer.from(message, 'utf-8');
        const length = Buffer.allocUnsafe(4);
        length.writeUInt32LE(data.length, 0);  // 小端写入

        this.socket.write(Buffer.concat([length, data]));
    }

    onData(chunk) {
        this.buffer = Buffer.concat([this.buffer, chunk]);

        while (this.buffer.length >= 4) {
            // 读取长度
            const msgLen = this.buffer.readUInt32LE(0);

            // 检查是否有完整消息
            if (this.buffer.length < 4 + msgLen) {
                break;  // 等待更多数据
            }

            // 提取消息
            const message = this.buffer.slice(4, 4 + msgLen).toString('utf-8');
            console.log('📨 Server:', message);

            // 移除已处理的消息
            this.buffer = this.buffer.slice(4 + msgLen);
        }
    }

    disconnect() {
        this.socket.end();
    }
}

// 使用示例
async function main() {
    const client = new GearEngineClient();
    await client.connect();

    client.sendMessage('Hello World\n');
    client.sendMessage('/help\n');

    setTimeout(() => {
        client.sendMessage('/quit\n');
        client.disconnect();
    }, 2000);
}

main().catch(console.error);
```

---

### Go 客户端

```go
package main

import (
    "encoding/binary"
    "fmt"
    "io"
    "net"
)

type GearEngineClient struct {
    conn net.Conn
}

func NewClient(host string, port int) (*GearEngineClient, error) {
    addr := fmt.Sprintf("%s:%d", host, port)
    conn, err := net.Dial("tcp", addr)
    if err != nil {
        return nil, err
    }

    return &GearEngineClient{conn: conn}, nil
}

func (c *GearEngineClient) SendMessage(message string) error {
    data := []byte(message)

    // 写入长度 (小端)
    length := make([]byte, 4)
    binary.LittleEndian.PutUint32(length, uint32(len(data)))

    // 发送长度 + 数据
    if _, err := c.conn.Write(length); err != nil {
        return err
    }
    if _, err := c.conn.Write(data); err != nil {
        return err
    }

    return nil
}

func (c *GearEngineClient) ReceiveMessage() (string, error) {
    // 读取长度
    lengthBuf := make([]byte, 4)
    if _, err := io.ReadFull(c.conn, lengthBuf); err != nil {
        return "", err
    }

    msgLen := binary.LittleEndian.Uint32(lengthBuf)

    // 读取消息
    msgBuf := make([]byte, msgLen)
    if _, err := io.ReadFull(c.conn, msgBuf); err != nil {
        return "", err
    }

    return string(msgBuf), nil
}

func (c *GearEngineClient) Close() error {
    return c.conn.Close()
}

func main() {
    client, err := NewClient("127.0.0.1", 8080)
    if err != nil {
        panic(err)
    }
    defer client.Close()

    // 发送消息
    client.SendMessage("Hello from Go!\n")

    // 接收响应
    msg, err := client.ReceiveMessage()
    if err != nil {
        panic(err)
    }
    fmt.Println("📨 Server:", msg)
}
```

---

## 服务器端 (Lua)

服务器端 Lua 脚本**无需修改**,`gear.send_data()` 和 `gear.send_bson()` 会自动添加长度前缀。

```lua
-- 示例: 回显服务器
function on_client_data(client, data)
    -- data 已经是去掉长度头的纯消息内容
    gear.log("Received: " .. data)

    -- 发送时自动添加长度头
    gear.send_data(client, "Echo: " .. data)
end
```

---

## 测试工具

### 1. 使用 netcat (简单测试)

⚠️ **注意**: netcat 不会自动添加长度前缀,需要手动构造:

```bash
# 发送 "Hello\n" (6字节)
# 长度头: 0x06 0x00 0x00 0x00 (小端)
echo -ne '\x06\x00\x00\x00Hello\n' | nc localhost 8080
```

### 2. 使用 Python 脚本 (推荐)

```bash
cd ge/example
python protocol_example_client.py

# 连接后可以:
# - 输入任意文本发送消息
# - 输入 /test 进行压力测试
# - 输入 /quit 断开连接
```

### 3. 压力测试

```python
#!/usr/bin/env python3
import socket
import struct
import time

def stress_test(host='127.0.0.1', port=8080, count=1000):
    """发送 count 条消息,测试服务器性能"""
    sock = socket.socket()
    sock.connect((host, port))

    start = time.time()

    for i in range(count):
        msg = f"Stress test message {i}\n".encode()
        length = struct.pack('<I', len(msg))
        sock.sendall(length + msg)

        if i % 100 == 0:
            print(f"Sent {i}/{count}")

    elapsed = time.time() - start
    print(f"✅ Sent {count} messages in {elapsed:.2f}s")
    print(f"   Throughput: {count/elapsed:.0f} msg/s")

    sock.close()

if __name__ == '__main__':
    stress_test()
```

---

## 常见问题

### Q1: 为什么需要长度前缀?

**A:** TCP 是流协议,不保证消息边界。例如:
- 发送: `"Hello"` + `"World"`
- 接收可能是: `"HelloWor"` + `"ld"` (粘包)
- 或者: `"Hello"` + `"Wo"` + `"rld"` (拆包)

长度前缀确保接收方知道每条消息的确切边界。

### Q2: 能否使用分隔符代替长度前缀?

**A:** 可以,但有局限性:
- 分隔符 (如 `\n`) 简单,但消息体不能包含分隔符
- 需要转义处理 (复杂且低效)
- 二进制数据难以处理

长度前缀方案更通用、高效。

### Q3: 为什么是小端字节序?

**A:**
- x86/x64 架构使用小端 (最常见)
- ARM 通常也是小端
- 保持一致性,避免字节序转换开销

如果客户端是大端系统,需要转换:
```c
// 大端转小端
uint32_t len_le = htole32(msg_len);
```

### Q4: 最大消息长度是多少?

**A:** 当前限制为 4092 字节 (`CLIENT_BUFFER_SIZE - MESSAGE_HEADER_SIZE`)

**增大限制**:
```c
// 修改 src/server.h
#define CLIENT_BUFFER_SIZE 65536  // 改为 64KB

// 重新编译
cd build
cmake --build .
```

### Q5: 如何调试协议问题?

**A:** 使用 Wireshark 或 tcpdump 抓包:

```bash
# 抓取本地回环流量
sudo tcpdump -i lo -X port 8080

# 或使用 Wireshark 过滤: tcp.port == 8080
```

查看数据包内容,验证长度前缀是否正确。

---

## 迁移检查清单

从旧版本 (无长度前缀) 迁移到新版本:

- [ ] 更新所有客户端代码,添加长度前缀发送逻辑
- [ ] 更新所有客户端代码,添加长度前缀接收逻辑
- [ ] 测试单条消息发送/接收
- [ ] 测试多条消息连续发送 (粘包场景)
- [ ] 测试大消息 (接近 4KB)
- [ ] 压力测试 (1000+ 消息)
- [ ] 验证错误处理 (断开连接、超时等)

---

## 相关文档

- [FIXES.md](../FIXES.md) - 详细修复说明
- [chat_server.lua](example_server/chat_server.lua) - 服务器示例
- [protocol_example_client.py](protocol_example_client.py) - Python客户端示例

---

**文档版本**: 1.0
**最后更新**: 2026-01-19
