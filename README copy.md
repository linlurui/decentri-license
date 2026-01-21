# DecentriLicense

<p align="center">
  <b>去中心化软件授权SDK</b><br>
  <i>Decentralized Software License Management System</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License">
  <img src="https://img.shields.io/badge/C++-17/20-00599C.svg" alt="C++">
  <img src="https://img.shields.io/badge/Go-1.21-00ADD8.svg" alt="Go">
</p>

## 📖 简介 | Introduction

DecentriLicense 是一个创新的混合架构软件授权管理开发框架，主张去中心化软件授权。它通过本地P2P网络实现安全的离线授权与协作，并可选配轻量级云端协调服务，在保障用户隐私与离线可用的同时，为软件提供商提供集中式的许可证管理与洞察。这解决了传统方案在断网即失效、隐私顾虑和部署复杂上的核心痛点。

DecentriLicense is an innovative hybrid-architecture software license management system. It enables secure offline authorization and collaboration through a local P2P network, with an optional lightweight cloud coordination service. This unique design ensures user privacy and offline operation while providing software vendors with centralized license management and insights—effectively solving the core drawbacks of traditional solutions, such as network dependency, privacy concerns, and deployment complexity.

我们从设计哲学出发，采用清晰的职责分离设计：轻量级SDK处理局域网内的直接通信，而一个独立、由Go编写的高并发服务端则负责管理广域网下的状态协调，确保架构的弹性与扩展性。

Our system adopts a clean separation of concerns: a lightweight SDK handles direct communication within the LAN, while a dedicated, high-concurrency server written in Golang manages state coordination across the WAN, ensuring architectural resilience and scalability.

### 核心特性 | Core Features

- **🌐 去中心化架构** | Decentralized Architecture
  - 局域网内P2P自动发现和协调
  - 无需中央服务器即可运行
  
- **💻 跨平台支持** | Cross-Platform Support
  - C++ SDK支持Windows、macOS、Linux
  - 基于现代C++17/20标准
  
- **🔐 安全可靠** | Secure & Reliable
  - 多算法数字签名支持（RSA、Ed25519、SM2国密标准）
  - AES-256-GCM加密（密钥由产品公钥 PEM 的 SHA256 全 32 字节派生，输出 Base64URL(ciphertext)|Base64URL(nonce)）
  - OpenSSL加密库支持
  
- **⚡ 高性能网络** | High-Performance Networking
  - ASIO异步I/O
  - UDP广播发现 + TCP可靠传输
  - 简化的Bully选举算法

- **🔧 灵活的密钥管理** | Flexible Key Management
  - 支持自动生成密钥对
  - 支持从文件加载现有密钥对
  - 提供设置向导功能
  
- **🤝 智能冲突协调**：当设备联网时，自动通过高效的P2P选举算法或广域网协调服务解决许可证冲突，确保同一时间只有一台设备可使用。
- **💾 离线状态链（核心创新）**：每个许可证都是一个可追溯、不可篡改的微型状态链，支持安全的离线状态更新与审计，实现真正的**离线溯源码**功能。
- **🔐 兼容安全与性能的算法栈**：支持现代密码学标准，默认使用高性能的Ed25519，同时兼容传统的RSA以及符合中国国密的SM2算法。
- **📦 对开发者友好**：提供多语言SDK，易于集成到现有软件中。

### 🪢 离线状态链：超越静态授权
DecentriLicense 的 Token 不是一个静态代码，而是一个可安全更新的**微型状态链**。这为软件授权带来了前所未有的灵活性：

[创世状态] → [状态1: 充值100点] → [状态2: 使用30点] → [当前状态: 剩余70点]

↑ ↑ ↑

根签名 状态签名 状态签名


- **不可篡改**：每个新状态都包含前一个状态的哈希，形成密码学保证的链条。
- **离线更新**：状态的更新（如记录使用量）完全在本地完成，无需网络。
- **业务承载**：状态可携带自定义数据（JSON格式），实现复杂的计费、溯源或功能解锁逻辑。

## 🏗️ 系统架构 | Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   应用程序 Application                    │
└───────────────────────────────┬─────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────┐
│              C++ SDK (DecentriLicense)                  │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐   │
│  │ Network Mgr │  │ Election Mgr │  │  Token Mgr    │   │
│  │  (P2P)      │  │  (Bully)     │  │  (License)    │   │
│  └─────────────┘  └──────────────┘  └───────────────┘   │
│                    ┌──────────────┐                     │
│                    │ Crypto Utils │                     │
│                    │  (OpenSSL)   │                     │
│                    └──────────────┘                     │
└────────────────┬───────────────────────┬────────────────┘
                 │                       │
                 │ UDP Broadcast         │ TCP P2P
                 │ (Discovery)           │ (Token Transfer)
                 ▼                       ▼
        局域网 LAN P2P             ╔══════════════════╗
                                  ║ Go Registry      ║
                                  ║ Server (Optional)║
                                  ║ WAN Coordination ║
                                  ╚══════════════════╝
```

## 核心架构

系统由四个核心部分构成，目录结构清晰：
1.  **`dl-core` (C++ 核心 SDK)**
    - **性质**：开源
    - **功能**：提供核心的P2P网络通信、选举算法、令牌验证与状态链逻辑。使用 **OpenSSL** 实现加密。

2.  **`sdks` (各种开发语言 SDK)**
    - **性质**：开源
    - **功能**：为各种开发语言生态提供的绑定或实现，便于项目集成验证逻辑。

3.  **`dl-issuer` (签发服务)**
    - **性质**：闭源 / 商业
    - **功能**：安全的创世令牌签发工具。包含绝密的根私钥，用于生成和签名初始许可证。

4.  **`server` (协调服务中心)**
    - **性质**：闭源服务 / 可自托管
    - **功能**：轻量级的"号码簿"服务，用于在广域网环境下协调不同设备，防止许可证滥用。


### 📁 项目目录结构 | Project Layout

decentrilicense/
├── server/ # 协调服务中心 (Go) - 商业组件
├── sdks/ # 多语言软件开发工具包 (开源)
│ ├── c/ # C 语言绑定
│ ├── cpp/ # C++ 标头和示例
│ ├── go/ # Go 语言绑定
│ ├── java/ # Java 语言绑定
│ ├── nodejs/ # Node.js 语言绑定
│ ├── python/ # Python 语言绑定
│ ├── rust/ # Rust 语言绑定
│ └── csharp/ # C# 语言绑定
├── dl-issuer/ # 创世令牌签发工具 (Go) - 商业组件
├── dl-core/ # C++ 核心库实现
└── tests/ # 跨组件集成测试


**关键说明**：
- `dl-core` 是系统的基石，所有去中心化逻辑在此实现。
- `sdks/` 目录包含各语言的绑定和示例代码。
- `dl-issuer` 是商业签发工具，包含根私钥用于生成创世令牌。
- *商业组件* 为可选的高级功能和服务，建立在开源核心之上。

## 详细需求清单

### 1. 授权模型与令牌 (Token)
- **令牌结构**：令牌包含唯一ID、许可证代码、设备绑定信息、状态链字段、加密签名等。
- **状态链（离线溯源码）**：
    - 每个令牌都是一个状态链的头部，支持离线状态迁移。
    - 每次状态更新（如记录使用量）都生成新的令牌，并包含前一个状态的哈希，形成不可篡改的链式记录。
    - 链上可承载自定义业务数据 (`state_payload`)，实现复杂的离线记账与溯源。
- **多层签名**：
    - **根签名**：由发行方的根私钥签发，建立对许可证公钥的信任。
    - **状态签名**：由本次许可证的私钥对状态链数据签发，保证状态更新的合法性。
    - **令牌签名**：保证整个令牌数据的完整性。

### 2. 网络与激活协议
- **局域网P2P发现**：基于UDP广播，自动发现同一网络内使用相同许可证的设备。
- **冲突解决选举**：采用改进的Bully或Raft-like算法，公平、随机地选出一台设备作为"领导者"持有令牌。
- **令牌安全传递**：领导者可将令牌通过加密通道安全传递给网络内请求使用的其他设备。
- **广域网协调**：设备可向协调服务中心注册心跳，中心协助解决跨局域网的冲突。

### 3. 加密与安全
- **密码学算法**：
    - 默认及推荐：**Ed25519** (高性能，签名短)
    - 兼容支持：**RSA-2048/4096** (最大兼容性)
    - 合规支持：**国密SM2** (满足国内合规要求)
- **密钥体系**：
    - **根密钥对**：由发行方离线生成，私钥绝密保管，公钥硬编码至SDK。
    - **许可证密钥对**：每次签发时临时生成，私钥不落盘，用后即焚。
- **防篡改**：所有关键数据均有数字签名保护。
- **防重放**：令牌包含时间戳、状态索引等，防止旧令牌被重复使用。

### 4. 跨平台与部署
- **SDK支持**：核心C++ SDK (`sdks/cpp`) 需支持 Windows、macOS、Linux。
- **多语言SDK**：提供Go等语言的SDK (`sdks/go`)，方便不同技术栈集成。
- **签发工具**：签发服务 (`dl-issuer`) 为跨平台命令行工具 (Go实现)。
- **协调服务**：协调中心 (`server`) 为无状态HTTP服务，可容器化部署。

### 5. 商业模式 (可选)
- **开源核心**：`sdks/` 目录下的SDK以MIT协议开源，供开发者自由集成。
- **商业服务**：提供以下付费服务：
    - **签发服务 (`dl-issuer`)**：软件发行方使用，用于生成许可证。
    - **协调中心服务 (`server`)**：SaaS模式，提供许可证冲突协调、设备管理、使用量分析仪表盘。
    - **企业版**：支持私有化部署、定制算法、专属支持。
- **计费模式**：按活跃许可证数量或终端用户量订阅计费。

## 技术栈
- **`sdks/cpp` (核心)**：C++17/20, CMake, ASIO (网络), **OpenSSL** (加密，支持RSA/Ed25519/SM2)
- **`sdks/go` (绑定)**：Go (Golang), CGO或纯Go实现
- **`dl-issuer` & `server`**：Go (Golang), 标准库 `crypto`
- **持久化**：本地文件系统 (状态链日志)，可选数据库 (协调中心)

## 目标用户
1.  **独立软件开发商 (ISV)**：销售专业桌面工具、创意软件、开发工具。
2.  **企业软件提供商**：需要复杂授权逻辑（按模块、按用户、按量）的B2B软件。
3.  **游戏开发者**：需要控制试玩时长、防止账号共享的独立游戏。
4.  **需要离线溯源码的场景**：物流、资产追踪、离线设备数据采集等需要离线环境下记录不可篡改状态链的领域。
5.  **任何需要可靠、灵活、离线可用授权系统的开发者**。

## 项目状态
本项目正在积极开发中。我们遵循"开源核心，商业增强"的原则，致力于构建一个透明、可信且可持续发展的授权解决方案。

## 🚀 快速开始 | Quick Start

### 前置条件 | Prerequisites

**C++ SDK 编译环境 | C++ SDK Build Environment:**
- CMake 3.15+
- C++17 兼容编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- OpenSSL 1.1.0+
- ASIO (standalone or Boost.Asio)

**Go 服务端 | Go Server:**
- Go 1.23

### 安装依赖 | Install Dependencies

**macOS:**
```bash
brew install cmake openssl asio
```

**Ubuntu/Debian:**
```bash
sudo apt-get install cmake libssl-dev libasio-dev
```

**Windows:**
- 安装 Visual Studio 2019+
- 使用 vcpkg: `vcpkg install openssl asio`

### 编译 C++ SDK | Build C++ SDK

```bash
# 1. 配置项目 | Configure project
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. 编译 | Build
cmake --build build --config Release

# 3. 运行演示程序 | Run demo
./build/bin/demo

# 4. 运行设置向导演示程序 | Run setup wizard demo
./build/bin/setup_wizard_demo
```

### 运行 Go 服务端 | Run Go Server

```bash
cd server
go build -o decentrilicense-server ./cmd
./decentrilicense-server -port 3883
```

## 📝 API 使用示例 | API Usage Example

``cpp
#include <decentrilicense/decentrilicense_client.hpp>

using namespace decentrilicense;

int main() {
    // 创建客户端 | Create client
    DecentriLicenseClient client;
    
    // 配置 | Configure
    ClientConfig config;
    config.license_code = "YOUR-LICENSE-KEY";
    config.udp_port = 8888;
    config.tcp_port = 8889;
    
    // 初始化 | Initialize
    if (!client.Initialize(config)) {
        return 1;
    }
    
    // 激活许可 | Activate license
    auto future = client.Activate();
    auto result = future.get();
    
    if (result.success) {
        // 获取令牌 | Get token
        auto token = client.GetCurrentToken();
        if (token.has_value()) {
            std::cout << "Licensed!" << std::endl;
        }
    }
    
    // 关闭 | Shutdown
    client.Shutdown();
    
    return 0;
}
``

## 🔧 设置向导功能 | Setup Wizard Feature

DecentriLicense SDK 现在支持灵活的密钥管理选项，用户可以选择自动生成密钥对或使用现有的密钥文件。

### 自动生成密钥（默认行为）

```cpp
ClientConfig config;
config.license_code = "YOUR-LICENSE-KEY";
config.generate_keys_automatically = true;  // 默认值

DecentriLicenseClient client;
client.Initialize(config);
```

### 使用现有密钥文件

```cpp
ClientConfig config;
config.license_code = "YOUR-LICENSE-CODE";
config.generate_keys_automatically = false;
config.private_key_file = "/path/to/private_key.pem";
config.public_key_file = "/path/to/public_key.pem";

DecentriLicenseClient client;
client.Initialize(config);
```

### 状态链操作示例 | State Chain Operations

```cpp
// 假设已获得初始 Token
auto current_token = client.GetCurrentToken().value();

// 1. 准备新的业务数据（例如，扣除50点额度）
nlohmann::json new_payload = {
    {"action", "usage_deduct"},
    {"units", 50},
    {"remaining", 450} // 假设原有500点
};

// 2. 执行状态迁移（需要许可证私钥）
// 注：私钥应由安全的密钥管理器提供，此处为示例
std::string license_priv_key = load_private_key_safely();
auto new_token = token_manager.MigrateState(current_token, 
                                            new_payload.dump(), 
                                            license_priv_key);

// 3. 验证新状态的合法性
if(token_manager.VerifyToken(new_token)) {
    // 验证通过，可替换旧Token并持久化
    client.UpdateCurrentToken(new_token);
    std::cout << "状态已更新，剩余额度：" << new_payload["remaining"] << std::endl;
}
```

## 🌐 Go 服务端 API | Go Server API

### 健康检查 | Health Check
```bash
curl http://localhost:3883/api/health
```

### 注册设备 | Register Device
```bash
curl -X POST http://localhost:3883/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id":"abc123","license_code":"LICENSE-KEY","tcp_port":8889}'
```

### 查询许可证持有者 | Query License Holder
```bash
curl http://localhost:3883/api/licenses/LICENSE-KEY/holder
```

## 技术特点
### LICENSE-KEY (Token) 机制与安全
### LICENSE-KEY (Token) Mechanism and Security

在 DecentriLicense 系统中，许可证密钥被称为 **Token**，它包含了授权状态的核心信息，并通过加密机制确保其安全性与防伪性。
In the DecentriLicense system, the license key is referred to as a **Token**. It contains the core information of the authorization state and ensures its security and anti-counterfeiting through cryptographic mechanisms.

---

### Token 数据结构 | Token Data Structure
一个 Token 包含以下字段：
A Token consists of the following fields:
```json
{
  "token_id": "a18fee2ec0eb2503aa2180581350486f",
  "license_code": "ccait",
  "holder_device_id": "dev-63e0ac2331680000",
  "issue_time": 1765212485,
  "expire_time": 1767804485,
  "app_id": "my-app",
  "environment_hash": "",

  // --- 状态链核心字段（新增）---
  "state_index": 0,
  "prev_state_hash": "",
  "state_payload": "{}",
  "state_signature": "[对状态数据的标准长度签名]",

  // --- 清晰的算法标识（新增）---
  "key_algorithm": "RSA",

  "license_public_key": "-----BEGIN PUBLIC KEY-----\nMIIBIj...\n-----END PUBLIC KEY-----\n",
  "root_signature": "[对公钥和算法的标准长度签名]",

  // --- 最终令牌签名（必须符合标准长度）---
  "signature": "[对整个令牌数据的标准长度签名]"
}
```

---

### 生成 Token | Token Generation

*   **协调者角色**: 在本地P2P网络中，只有被选举为 **协调者** 的设备才有权生成新的有效Token。
    **Coordinator Role**: In the local P2P network, only the device elected as the **Coordinator** is authorized to generate new valid Tokens.
*   **生成过程**: 通过调用 `TokenManager::generate_token()` 方法创建并初始化一个新的Token实例。
    **Generation Process**: A new Token instance is created and initialized by calling the `TokenManager::generate_token()` method.
*   **签名保护**: 生成时，系统使用协调者的RSA私钥对Token的核心数据进行数字签名，并将签名存入`signature`字段。这是Token合法性的根本保证。
    **Signature Protection**: During generation, the system uses the Coordinator's RSA private key to create a digital signature over the Token's core data, which is stored in the `signature` field. This is the fundamental guarantee of the Token's legitimacy.

---

### 校验 Token | Token Verification

任何设备在接收到一个Token后，都必须进行严格校验：
Any device must perform strict verification upon receiving a Token:

1.  **签名验证**: 通过 `TokenManager::verify_token()` 方法，使用对应的RSA公钥验证 `signature` 的有效性。此步骤确保Token未被篡改且确实由合法的协调者颁发。
    **Signature Verification**: The `TokenManager::verify_token()` method is used to validate the `signature` using the corresponding RSA public key. This step ensures the Token has not been tampered with and was indeed issued by a legitimate Coordinator.
2.  **有效性检查**: 检查当前时间是否在 `issue_time` 和 `expire_time` 之间，并确认Token的 `holder_device_id` 是否与当前上下文匹配（防止重复使用）。
    **Validity Check**: Verifies that the current time is between `issue_time` and `expire_time`, and confirms that the Token's `holder_device_id` matches the current context (to prevent reuse).
3.  **完整性验证**: 确认Token的所有字段均符合逻辑与业务规则（例如，`license_code` 格式正确）。
    **Integrity Verification**: Confirms that all fields of the Token conform to logical and business rules (e.g., the `license_code` is in the correct format).

---

### 加密安全机制 | Cryptographic Security Mechanisms

系统采用多层加密措施来构建完整的安全防线：

The system employs multiple layers of cryptography to build a comprehensive security defense:

*   **RSA 签名**: 采用RSA非对称加密算法对Token进行数字签名。这是防伪的核心，私钥签名、公钥验证的模式确保了令牌的不可伪造性。

    **RSA Signature**: The RSA asymmetric encryption algorithm is used for digital signing of Tokens. This is the core of anti-counterfeiting; the model of signing with a private key and verifying with a public key ensures the token's unforgeability.
  
* **SHA-256 哈希**: 广泛用于生成设备ID、计算数据摘要以及作为其他加密过程的基础。其抗碰撞性保证了标识和摘要的唯一性与可靠性。

    **SHA-256 Hash**: Widely used for generating device IDs, computing data digests, and serving as the foundation for other cryptographic processes. Its collision resistance guarantees the uniqueness and reliability of identifiers and digests.

* **AES 加密**: 在对网络传输的敏感数据（如Token传输协议载荷）进行加密时使用，确保通信过程中的机密性，防止窃听。

   **AES Encryption**: Used to encrypt sensitive data during network transmission (e.g., Token transfer protocol payloads), ensuring confidentiality during communication and preventing eavesdropping.

---

## 🔧 核心组件说明 | Core Components

### NetworkManager (网络管理器)
- UDP 广播发现局域网设备
- TCP 点对点可靠通信
- 异步 I/O 处理

### ElectionManager (选举管理器)
- 简化的 Bully 选举算法
- 设备 ID 优先级判定
- 协调者选举与维护

### TokenManager (令牌管理器)
- 令牌生成与签名（支持RSA、Ed25519、SM2算法）
- 令牌验证与转移
- 过期时间管理
- 智能验证结果缓存

### CryptoUtils (加密工具)
- RSA/Ed25519/SM2 密钥对生成
- 数字签名与验证
- AES-GCM 加密/解密
- SHA-256/SM3 哈希

## 🌐 多算法支持 | Multi-Algorithm Support

DecentriLicense now supports multiple cryptographic algorithms to meet various security and compliance requirements:

### Supported Algorithms

| Algorithm | Description | Use Case |
|-----------|-------------|----------|
| RSA | Industry-standard asymmetric cryptography | Maximum compatibility with international environments |
| Ed25519 | Modern high-speed elliptic curve cryptography | Best performance and security for modern applications |
| SM2 | Chinese National Standard for asymmetric cryptography | Compliance with Chinese regulations for government and financial sectors |

### Algorithm Selection Guide

| 场景 | 推荐算法 | 理由 |
| :--- | :--- | :--- |
| 追求极致性能与现代化 | Ed25519 | 速度最快，签名短，设计安全 |
| 最大兼容性（国际传统环境） | RSA | 行业标准，无处不在 |
| 中国合规性需求 | SM2 | 满足国密标准，金融政务必备 |


### 总结 | Summary
因此，DecentriLicense 的主客户端不仅能够生成和校验许可证令牌，更通过一套完整的、分层式的安全机制，从根本上保障了许可证在整个生命周期内的有效性、完整性与防伪能力。

Therefore, the DecentriLicense client not only can generate and verify license tokens but also fundamentally guarantees the validity, integrity, and anti-counterfeiting capability of the license throughout its entire lifecycle through a complete, layered security mechanism.


## 🔒 增强验证与溯源功能 | Enhanced Verification and Traceability

DecentriLicense 现在支持增强的验证和溯源功能，包括：

1. **设备密钥管理**：在激活时为每个设备生成唯一的 Ed25519 密钥对
2. **设备身份验证**：使用设备私钥对设备信息进行签名，确保令牌与特定设备绑定
3. **使用链跟踪**：记录每次 API 调用，形成不可篡改的使用链
4. **防作弊机制**：通过 hash_prev 字段确保链的完整性，防止令牌被复制到其他设备

### 令牌结构示例

```json
{
  "license_id": "LIC-001",
  "product_id": "PROD-A",
  "alg": "Ed25519",
  "state_payload": {
    "max_calls": 1000,
    "expire_time": 1893456000
  },
  "device_info": {
    "fingerprint": "abc123...",
    "public_key": "-----BEGIN PUBLIC KEY-----\nMFYwEAYHKoZIzj0CAQYFK4EEAA...\n-----END PUBLIC KEY-----",
    "signature": "设备私钥对上述信息的签名"
  },
  "usage_chain": [
    {
      "seq": 1,
      "time": "2024-01-01T10:00:00Z",
      "action": "api_call",
      "params": {"function": "process_image"},
      "hash_prev": "0",
      "signature": "产品私钥的签名"
    },
    {
      "seq": 2,
      "time": "2024-01-01T10:05:00Z",
      "action": "api_call",
      "params": {"function": "export_pdf"},
      "hash_prev": "hash_of_seq1",
      "signature": "产品私钥的签名"
    }
  ],
  "current_signature": "产品私钥对整个文件的最新签名"
}
```

### 工作流程

1. **激活**：用户首次使用时，SDK 生成设备密钥对并将设备公钥写入令牌
2. **计量**：每次 API 调用时，SDK 在 usage_chain 中追加记录并重新签名
3. **验证**：每次启动时，SDK 验证 current_signature 和 usage_chain 的完整性
4. **防作弊**：hash_prev 字段确保链不可篡改，device_fingerprint 防止令牌被复制

## 🔄 核心创新：可重写、可溯源的授权码 | Core Innovation: Rewritable and Traceable License Codes

每个授权码都是一个可追加写、不可篡改的微型状态链，支持：

- **状态迁移**：通过 state_index 和 prev_state_hash 实现状态版本控制
- **离线验证**：完整的签名链确保即使在网络中断时也能验证令牌有效性
- **业务隔离**：通过 app_id 实现不同应用间的许可证隔离
- **环境绑定**：可选的 environment_hash 实现许可证与运行环境绑定

## 🧪 dl-issuer 签发工具使用说明

### 主要功能

dl-issuer 是 DecentriLicense 系统的许可证签发工具，提供了以下主要功能：

1. **生成根密钥对（用于信任链模型）**
   - 生成系统的根密钥对，用于建立信任链
   - 根私钥需安全保管，根公钥会被硬编码到SDK中

2. **生成产品密钥对（用于客户SDK集成）**
   - 使用根私钥签名生成产品密钥对
   - 产品公钥提供给客户集成到SDK中
   - 产品私钥用于生成授权令牌

3. **使用产品密钥生成授权令牌**
   - 使用产品私钥生成授权令牌
   - 支持多种算法（RSA、Ed25519、SM2）
   - 生成格式化的许可证代码（如 PRO-2024-001-ABC123）

4. **验证现有令牌**
   - 验证令牌的有效性和完整性
   - 检查根签名和令牌签名

5. **加载根私钥文件**
   - 加载已有的根私钥文件

6. **构建各语言SDK**
   - 使用各语言目录中的构建脚本构建SDK

7. **一键打包dl-core到各个SDK**
   - 将核心库打包到各语言SDK中

## 🔄 编译和测试

### 重新编译所有库文件与应用程序

在进行任何修改后，请确保重新编译所有组件：

```bash
# 编译 C++ 核心库
cd dl-core
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 编译签发工具
cd ../../../dl-issuer
go build -o dl-issuer .

# 编译 Go SDK
cd ../sdks/go
./build.sh

# 返回项目根目录
cd ../../
```

### 运行测试

```bash
# 运行 C++ 单元测试
cd sdks/cpp/build
./bin/unit_tests

# 运行签发工具测试
cd ../../../dl-issuer
./dl-issuer --help
```

## 📚 SDK使用指南 | SDK Usage Guide

我们为所有支持的编程语言提供了详细的SDK使用指南：
We provide detailed SDK usage guides for all supported programming languages:

### ⚠️ 重要安全提示 | Important Security Notice

**测试公钥警告 | Test Public Key Warning:**

`sdks/` 目录下的所有公钥文件（如 `public_test_*.pem`）仅供开发和测试使用，**请勿在生产环境中使用**。
All public key files in the `sdks/` directory (such as `public_test_*.pem`) are for development and testing purposes only. **DO NOT use them in production environments.**

生产环境必须使用 `dl-issuer` 工具生成的专用密钥对。
Production environments must use dedicated key pairs generated by the `dl-issuer` tool.

---

- **[SDK使用指南 | SDK Usage Guide](sdks/README.md)** - 包含所有语言SDK的详细使用说明、代码示例和常见问题解答
  Contains detailed usage instructions, code examples, and FAQs for all language SDKs
  - Java SDK - JNI绑定，支持Java 17+ | JNI bindings, supports Java 17+
  - Python SDK - ctypes绑定，支持Python 3.8+ | ctypes bindings, supports Python 3.8+
  - Node.js SDK - N-API原生模块 | N-API native module
  - C# SDK - P/Invoke绑定，支持.NET 6+ | P/Invoke bindings, supports .NET 6+
  - Rust SDK - FFI绑定 | FFI bindings
  - C++ SDK - 原生C++接口 | Native C++ interface
  - Go SDK - CGO绑定 | CGO bindings
  - C SDK - 原生C接口 | Native C interface
  - PHP SDK - FFI绑定 | FFI bindings

## 🧪 SDK验证向导

为了方便测试和验证各语言SDK的功能，我们提供了统一的验证向导工具。详细信息请参考整合文档：

- [`SDK_VALIDATION_INTEGRATED.md`](SDK_VALIDATION_INTEGRATED.md) - 包含所有SDK验证向导的使用说明、状态报告和测试验证结果
- [`sdks/README.md`](sdks/README.md) - 各语言SDK的详细使用指南和操作说明

## 🤝 贡献指南 | Contributing

我们欢迎各种形式的贡献！

We welcome contributions of all kinds!

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request


## 🛣️ 下一步计划 | What's Next

我们正在积极开发以下功能，并热切欢迎社区贡献：

- [ ] **更完善的管理控制台**：用于监控许可证状态和使用情况。
- [ ] **Java / Python SDK**：扩展更多语言支持。
- [ ] **增强的选举算法**：进一步优化P2P网络的稳定性和公平性。
- [ ] **详尽的文档与教程**：包括从集成到部署的完整指南。

## 🤝 如何贡献 | How to Contribute

我们相信开源的力量。如果您对以下任何一项感兴趣，我们都非常欢迎您的加入：

1.  **代码贡献**：请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) （待创建）了解代码规范。
2.  **文档改进**：帮助完善文档、翻译或创建教程。
3.  **测试与反馈**：试用SDK，提交Issue报告Bug或提出建议。
4.  **特性讨论**：在GitHub Discussions中分享您的想法。

让我们共同构建未来软件授权的基础设施。

## 📄 许可证 | License

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📧 联系方式 | Contact

- 项目主页 | Project Home: https://github.com/linlurui/decentri-license
- 问题反馈 | Issue Tracker: https://github.com/linlurui/decentri-license/issues

## 🙏 致谢 | Acknowledgments

- [ASIO](https://think-async.com/Asio/) - 异步网络库
- [OpenSSL](https://www.openssl.org/) - 加密库
- 所有贡献者 | All contributors
