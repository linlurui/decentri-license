# DecentriLicense C++ Core SDK
# DecentriLicense C++ 核心库

## Dependencies / 依赖项

### Required Libraries / 必需库

1. **OpenSSL** (1.1.1 or later) / **OpenSSL** (1.1.1 或更高版本)
2. **libsodium** (1.0.18 or later) / **libsodium** (1.0.18 或更高版本)
3. **ASIO** (standalone, not Boost.ASIO) / **ASIO** (独立版本，非Boost.ASIO)

### Installation Instructions / 安装说明

#### Ubuntu/Debian / Ubuntu/Debian
```bash
# Install OpenSSL and development tools
# 安装OpenSSL和开发工具
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config

# Install OpenSSL
# 安装OpenSSL
sudo apt-get install libssl-dev

# Install libsodium
# 安装libsodium
sudo apt-get install libsodium-dev

# Install ASIO (standalone)
# 安装ASIO (独立版本)
sudo apt-get install libasio-dev
```

#### macOS / macOS
```bash
# Install Homebrew if not already installed
# 如果尚未安装Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
# 安装依赖项
brew install cmake openssl libsodium asio pkg-config
```

#### Windows / Windows
```bash
# Install vcpkg package manager
# 安装vcpkg包管理器
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
# 安装依赖项
.\vcpkg install openssl libsodium asio
```

## Building / 构建

```bash
mkdir build
cd build
cmake ..
make
```

## Supported Algorithms / 支持的算法

- RSA (OpenSSL) / RSA (OpenSSL)
- Ed25519 (libsodium) - Fully implemented / Ed25519 (libsodium) - 已完全实现
- SM2 (Planned for future release, use Go implementation for now) / SM2 (计划在未来版本中发布，目前请使用Go实现)

## Architecture / 架构

### Core Components / 核心组件

#### DecentriLicenseClient / DecentriLicenseClient
Main client class providing high-level API for license management.
主要的客户端类，提供许可证管理的高级API。

#### TokenManager / TokenManager
Handles token creation, validation, and state management.
处理令牌创建、验证和状态管理。

#### NetworkManager / NetworkManager
Manages P2P communication and device discovery.
管理P2P通信和设备发现。

#### ElectionManager / ElectionManager
Implements distributed coordinator election using Bully algorithm.
使用Bully算法实现分布式协调器选举。

### Key Features / 关键特性

#### Smart Degradation / 智能降级
- WAN Registry → LAN P2P → Offline fallback
- 广域网注册中心 → 局域网P2P → 离线回退

#### State Chain / 状态链
- Immutable audit trail of license usage
- 许可证使用的不可变审计跟踪
- Cryptographic linking between states
- 状态间的密码学链接

#### P2P Coordination / P2P协调
- UDP discovery (port 13325) / UDP发现 (端口13325)
- TCP coordination (port 23325) / TCP协调 (端口23325)
- Automatic conflict resolution / 自动冲突解决

## API Overview / API概览

### Client Initialization / 客户端初始化
```cpp
#include "decentrilicense/decentrilicense_client.hpp"

DecentriLicenseClient client(ClientConfig{});
client.initialize();
```

### Token Activation / 令牌激活
```cpp
std::string token_json = /* encrypted token */;
bool success = client.activate_with_token(token_json);
```

### License Verification / 许可证验证
```cpp
bool valid = client.verify_token_with_environment_check(current_token);
```

## Security / 安全特性

### Cryptographic Operations / 密码学操作
- RSA signature verification using PKCS#1 v1.5 padding
- 使用PKCS#1 v1.5填充的RSA签名验证
- SHA256 hashing for data integrity
- SHA256哈希用于数据完整性
- Secure random number generation
- 安全随机数生成

### Anti-Tampering / 防篡改
- State chain prevents rollback attacks
- 状态链防止回滚攻击
- Timestamp validation prevents replay attacks
- 时间戳验证防止重放攻击
- Hardware fingerprinting for device binding
- 硬件指纹用于设备绑定

## Performance / 性能

### Benchmarks / 基准测试
- Token verification: < 10ms / 令牌验证：<10ms
- Network discovery: < 100ms (LAN) / 网络发现：<100ms (局域网)
- Memory usage: < 50MB typical / 内存使用：<50MB 典型值
- Concurrent connections: 1000+ / 并发连接：1000+

### Optimizations / 优化
- Object pooling for reduced GC pressure / 对象池以减少GC压力
- Asynchronous I/O operations / 异步I/O操作
- Zero-copy message handling / 零拷贝消息处理
- Connection multiplexing / 连接复用

## Error Handling / 错误处理

### Common Error Codes / 常见错误代码
- `INVALID_TOKEN`: Token format or signature invalid / 令牌格式或签名无效
- `NETWORK_ERROR`: P2P communication failure / P2P通信失败
- `LICENSE_CONFLICT`: Multiple devices claiming same license / 多设备声明同一许可证
- `VERIFICATION_FAILED`: Cryptographic verification error / 密码学验证错误

### Logging / 日志记录
All components use standard C++ logging with configurable levels:
所有组件使用标准C++日志，可配置级别：
- ERROR: Critical errors / 严重错误
- WARN: Warning conditions / 警告条件
- INFO: General information / 一般信息
- DEBUG: Detailed debugging / 详细调试

## Testing / 测试

### Unit Tests / 单元测试
```bash
cd build
make test
```

### Integration Tests / 集成测试
```bash
# Run full system integration test
./test_integration
```

### Validation Suite / 验证套件
```bash
# Run comprehensive validation
./validate_all
```

## Deployment / 部署

### Production Configuration / 生产配置
- Enable TLS for network communications / 为网络通信启用TLS
- Configure rate limiting / 配置速率限制
- Set up monitoring and alerting / 设置监控和告警
- Use optimized build flags / 使用优化的构建标志

### Container Deployment / 容器部署
```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y \
    libssl-dev \
    libsodium-dev \
    libasio-dev

COPY build/libdecentrilicense.so /usr/lib/
COPY include/ /usr/include/decentrilicense/
```

## Contributing / 贡献

### Code Style / 代码风格
- Follow Google C++ Style Guide / 遵循Google C++风格指南
- Use smart pointers for memory management / 使用智能指针进行内存管理
- Document all public APIs / 为所有公共API编写文档
- Write unit tests for new features / 为新功能编写单元测试

### Development Setup / 开发设置
```bash
# Clone repository / 克隆仓库
git clone https://github.com/your-org/decentrilicense.git
cd decentrilicense/dl-core

# Install dependencies / 安装依赖项
./install_deps.sh

# Build in debug mode / 以调试模式构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests / 运行测试
make test
```

## License / 许可证

This project is licensed under the MIT License.
本项目采用MIT许可证。