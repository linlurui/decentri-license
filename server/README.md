# DecentriLicense WAN Registry Server

DecentriLicense广域网注册中心服务器，提供设备注册、心跳检测和许可证协调服务，是智能降级架构中的WAN层组件。

## 功能特性

- ✅ **设备注册** - 设备上线注册到WAN中心
- ✅ **心跳检测** - 设备在线状态监控
- ✅ **许可证查询** - 查询许可证持有者信息
- ✅ **令牌转移** - 协调跨设备令牌转移
- ✅ **高并发** - 支持1000+并发请求
- ✅ **负载均衡** - 自动流量控制和限流

## 架构位置

```
DecentriLicense 智能降级架构
├── WAN层 (Registry Server) ← 当前组件
├── LAN层 (P2P dl-core)
└── Offline层 (本地验证)
```

## API 接口

### 健康检查
```
GET /api/health
```
返回服务器健康状态。

### 统计信息
```
GET /api/stats
```
返回注册中心统计数据。

### 设备注册
```
POST /api/devices/register
Content-Type: application/json

{
  "device_id": "unique-device-id",
  "license_code": "LICENSE-CODE",
  "public_ip": "203.0.113.1",
  "tcp_port": 23325
}
```

### 心跳上报
```
POST /api/devices/heartbeat
Content-Type: application/json

{
  "device_id": "unique-device-id"
}
```

### 许可证查询
```
GET /api/licenses/{license_code}/holder
```
查询许可证的当前持有设备信息。

### 令牌转移
```
POST /api/tokens/transfer
Content-Type: application/json

{
  "token_id": "token-uuid",
  "token_data": "json-serialized-token",  // 可选，用于验证
  "from_device": "device-a",
  "to_device": "device-b",
  "license_code": "LICENSE-CODE"
}
```

**验证逻辑**:
- 检查源设备是否持有许可证
- 验证目标设备存在且活跃
- 可选的令牌数据完整性检查

## 编译运行

### 编译
```bash
cd server
go build -o dl-server cmd/main.go
```

### 运行
```bash
# 默认端口 3883
./dl-server

# 指定端口
./dl-server -port 8080

# 指定工作协程数
./dl-server -workers 8
```

### 生产部署
```bash
# 使用优化版本
./decentrilicense-server-optimized

# 或使用dl-server
./dl-server -port 3883 -workers 16
```

## 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-port` | 3883 | HTTP服务器端口 |
| `-workers` | CPU核心数 | 工作协程数量 |

## 性能特性

- **并发处理**: 1000 req/s 基本负载，2000 req/s 峰值
- **内存优化**: 对象池复用，减少GC压力
- **安全防护**: 速率限制、请求超时、安全头
- **监控支持**: 健康检查、统计信息

## 数据模型

### Device (设备)
```go
type Device struct {
    DeviceID    string    // 设备唯一标识
    LicenseCode string    // 关联的许可证代码
    PublicIP    string    // 公网IP地址
    TCPPort     int       // P2P通信端口
    LastSeen    time.Time // 最后在线时间
}
```

### TokenTransfer (令牌转移)
```go
type TokenTransfer struct {
    TokenID     string    // 令牌ID
    FromDevice  string    // 源设备ID
    ToDevice    string    // 目标设备ID
    LicenseCode string    // 许可证代码
    Timestamp   time.Time // 转移时间戳
}
```

## 集成方式

### Go SDK 配置
```go
config := decenlicense.Config{
    LicenseCode:       "YOUR-LICENSE",
    PreferredMode:     decenlicense.ConnectionModeWANRegistry,
    UDPPort:          13325,
    TCPPort:          23325,
    RegistryServerURL: "https://your-registry-server.com",
}
```

### 降级策略
1. **WAN模式**: 优先连接注册中心
2. **LAN模式**: 注册中心不可用时自动降级到局域网P2P
3. **Offline模式**: 网络完全不可用时使用离线验证

## 监控和运维

### 健康检查
```bash
curl http://localhost:3883/api/health
```

### 统计查询
```bash
curl http://localhost:3883/api/stats
```

### 日志监控
服务器会输出详细的请求日志，支持：
- 设备注册/心跳事件
- 令牌转移记录
- 错误和异常情况

## 安全考虑

- **速率限制**: 防止DDoS攻击
- **输入验证**: JSON Schema验证
- **超时控制**: 防止慢速攻击
- **安全头**: XSS防护、内容嗅探防护

## 部署建议

### 生产环境
- 使用反向代理 (Nginx/Caddy)
- 启用TLS/SSL证书
- 配置防火墙规则
- 设置监控告警

### 扩展部署
- 使用负载均衡器
- 部署多个实例
- 配置数据库持久化（如果需要）

## 故障排除

### 常见问题

**Q: 设备无法注册**
A: 检查网络连接和API端点URL

**Q: 心跳超时**
A: 检查防火墙设置和网络稳定性

**Q: 性能下降**
A: 调整 `-workers` 参数或升级服务器配置

## 开发说明

### 项目结构
```
server/
├── cmd/main.go           # 服务器主程序
├── internal/
│   ├── handler/api.go    # HTTP API处理器
│   ├── service/registry.go # 注册中心服务
│   └── model/device.go   # 数据模型
└── README.md             # 本文档
```

### 扩展开发
- 在 `handler/api.go` 中添加新API端点
- 在 `service/registry.go` 中实现业务逻辑
- 在 `model/device.go` 中定义数据结构

---

**DecentriLicense WAN服务器** - 提供可靠的广域网设备协调服务，支持大规模分布式部署。
