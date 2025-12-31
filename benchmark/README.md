# GearEngine 压力测试工具

这是一个用于测试 GearEngine 聊天服务器性能的压力测试工具。

## 功能特性

- 支持创建多个并发连接
- 自动发送消息
- 统计性能指标：
  - 连接成功率
  - 消息吞吐量（每秒消息数）
  - 连接数
  - 错误统计
  - 响应延迟

## 构建

压力测试工具会在构建主项目时自动构建：

```bash
cd ge
./build-msys2.sh  # Windows MSYS2
# 或
./build-linux.sh  # Linux
```

可执行文件位于：`ge/build/bin/stress_client.exe` (Windows) 或 `ge/build/bin/stress_client` (Linux)

## 使用方法

### 基本用法

```bash
# 默认：100个客户端，每个发送10条消息，间隔100ms
./stress_client

# 自定义参数
./stress_client [客户端数] [每客户端消息数] [消息间隔ms] [服务器地址] [端口]
```

### 参数说明

1. **客户端数** - 要创建的并发连接数（默认：100）
2. **每客户端消息数** - 每个客户端发送的消息数量（默认：10）
3. **消息间隔** - 发送消息的间隔时间，单位毫秒（默认：100）
4. **服务器地址** - 服务器IP或域名（默认：localhost）
5. **端口** - 服务器端口（默认：8080）

### 示例

```bash
# 测试100个客户端，每个发送20条消息，间隔50ms
./stress_client 100 20 50

# 测试500个客户端，每个发送5条消息，间隔200ms，连接到192.168.1.100:8080
./stress_client 500 5 200 192.168.1.100 8080

# 高并发测试：1000个客户端，快速发送消息
./stress_client 1000 50 10 localhost 8080
```

## 测试场景

### 1. 连接压力测试

测试服务器能同时处理多少个连接：

```bash
# 逐步增加连接数
./stress_client 100 1 1000    # 100个连接，每个只发1条消息
./stress_client 500 1 1000    # 500个连接
./stress_client 1000 1 1000   # 1000个连接
```

### 2. 消息吞吐量测试

测试服务器每秒能处理多少消息：

```bash
# 100个客户端，每个发送100条消息，间隔10ms（高频率）
./stress_client 100 100 10

# 500个客户端，每个发送50条消息，间隔20ms
./stress_client 500 50 20
```

### 3. 长时间稳定性测试

测试服务器长时间运行的稳定性：

```bash
# 100个客户端，每个发送1000条消息，间隔100ms
./stress_client 100 1000 100
```

## 输出说明

测试过程中会每5秒打印一次统计信息：

```
=== Stress Test Statistics ===
Total connections attempted: 100
Successful connections: 98
Failed connections: 2
Active connections: 95
Messages sent: 850
Messages received: 1200
Errors: 5
Elapsed time: 10.50 seconds
Messages per second: 80.95
Connections per second: 9.33
==============================
```

### 指标说明

- **Total connections attempted** - 尝试连接的总数
- **Successful connections** - 成功建立的连接数
- **Failed connections** - 失败的连接数
- **Active connections** - 当前活跃的连接数
- **Messages sent** - 已发送的消息总数
- **Messages received** - 已接收的消息总数（包括服务器广播）
- **Errors** - 发生的错误数
- **Messages per second** - 每秒消息吞吐量
- **Connections per second** - 每秒连接数

## 性能调优建议

根据测试结果，可以调整服务器配置：

1. **连接数不足** - 检查服务器最大连接数限制
2. **消息丢失** - 检查消息队列和缓冲区大小
3. **高延迟** - 优化消息处理逻辑
4. **错误率高** - 检查网络和资源限制

## 注意事项

1. 确保服务器已启动并运行
2. 根据服务器性能调整客户端数和消息频率
3. 过高的并发可能导致系统资源耗尽
4. 建议在测试环境中运行，避免影响生产环境

## 故障排除

### 连接失败

- 检查服务器是否运行
- 检查端口是否正确
- 检查防火墙设置

### 消息丢失

- 降低消息发送频率
- 减少并发连接数
- 检查服务器日志

### 程序崩溃

- 检查系统资源（内存、文件描述符）
- 减少并发连接数
- 检查是否有内存泄漏
