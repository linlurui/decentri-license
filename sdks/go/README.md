# DecentriLicense Go SDK

DecentriLicense Go SDK 提供Go语言绑定，支持完整的离线授权闭环功能，包括令牌验证、激活、记账和状态查询。

## 功能特性

- ✅ **令牌验证** - 密码学签名验证
- ✅ **许可证激活** - Token与设备绑定
- ✅ **使用记账** - 状态链更新
- ✅ **状态查询** - 完整状态信息
- ✅ **P2P协调** - dl-core提供的网络功能
- ✅ **离线优先** - 支持WAN/LAN/Offline智能降级

## 架构设计

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ License Issuer  │    │   Go SDK        │    │   dl-core       │
│   (外部工具)    │    │   (API包装)     │    │   (C++核心)     │
│                 │    │                 │    │                 │
│ • 生成密钥      │    │ • 配置传递      │    │ • P2P网络       │
│ • 签发令牌      │    │ • API调用       │    │ • 选举协调      │
│ • 管理产品      │    │ • 错误处理      │    │ • 令牌验证      │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 核心功能演示

Go SDK 支持完整的离线授权闭环架构：
- 🔍 **验证Token** - 密码学签名验证
- 🔓 **激活许可证** - Token与设备绑定
- 📝 **记录使用情况** - 状态链更新
- 📊 **查询状态** - 状态信息读取

### 待dl-core更新后支持的功能

- P2P设备发现 (UDP 13325)
- 选举协调 (TCP 23325)
- 状态链比较和令牌传递
- 一物一码冲突检测

### 架构优势

- ✅ 去中心化P2P选举系统
- ✅ 智能降级：WAN → LAN → Offline
- ✅ 固定端口避免冲突
- ✅ 状态链不可篡改
- ✅ license_code激活后自动归档

## 快速开始

### 1. 环境准备

确保dl-core动态库已正确编译和安装。

**自动构建** (推荐):
```bash
# 如有构建脚本，请运行对应的构建命令
# 确保 libdecentrilicense.dylib 文件存在于正确位置
```

**手动验证**:
```bash
# 检查dl-core库文件是否存在
ls -la dl-core/build/libdecentrilicense.dylib
```

### 2. 准备产品公钥

从软件提供商处获取产品公钥文件 (`product_public.pem` 或 `public_*.pem`)，并将其放置在SDK目录中。

### 3. 运行验证向导

验证向导是一个功能完整的交互式工具，提供了所有SDK功能的端到端测试环境。

#### 启动向导

```bash
cd /Volumes/project/decentri-license-issuer/sdks/go/validation_wizard
./validation_wizard
```

#### 功能菜单

```
==========================================
DecentriLicense Go SDK 验证向导
==========================================

请选择要执行的操作:
0. 🔑 选择产品公钥
1. 🔓 激活令牌
2. ✅ 校验已激活令牌
3. 🔍 验证令牌合法性
4. 📊 记账信息
5. 🔗 信任链验证
6. 🎯 综合验证
7. 🚪 退出
```

#### 功能说明

| 功能 | 说明 | Token类型 | 修改数据 |
|------|------|----------|---------|
| **0. 选择产品公钥** | 选择用于验证的产品公钥文件 | - | ❌ |
| **1. 激活令牌** | 首次激活加密token，绑定设备 | Encrypted | ✅ 生成activated token |
| **2. 校验已激活令牌** | 验证本地已激活的token | Activated/State | ❌ |
| **3. 验证令牌合法性** | 验证任意token签名合法性 | Any | ❌ |
| **4. 记账信息** | 记录使用情况到状态链 | Activated/State | ✅ 生成state token |
| **5. 信任链验证** | 验证完整签名信任链 | Activated/State | ❌ |
| **6. 综合验证** | 全面测试所有功能 | Activated/State | ✅ 生成state token |

#### Token文件类型

验证向导会自动识别和处理三种类型的Token：

1. **加密Token** (`token_*encrypted.txt`)
   - 从供应商获得的原始token
   - 需要首次激活（选项1）
   - 激活后生成activated token

2. **已激活Token** (`token_activated_*.txt`)
   - 首次激活后自动生成
   - 包含设备绑定信息
   - 可直接用于记账等操作

3. **状态Token** (`token_state_*_idx*.txt`)
   - 记账后自动生成
   - 包含完整使用记录链
   - state_index递增表示版本

#### 详细文档

- **[Token使用指南](validation_wizard/TOKEN_USAGE_GUIDE.md)** - Token类型、使用流程、SDK设计说明
- **[验证功能说明](validation_wizard/VALIDATION_FEATURES.md)** - 信任链验证、综合验证功能详解

## 快速开始 - 代码集成

### 1. 基本使用

```go
package main

import (
    "fmt"
    "io/ioutil"
    "log"
    "path/filepath"
    "sort"

    decenlicense "github.com/linlurui/decentrilicense/sdks/go"
)

// findProductPublicKey 查找当前目录下的产品公钥文件
func findProductPublicKey() string {
    // 查找匹配模式的文件
    patterns := []string{"public_*.pem", "product_public*.pem", "*public*.pem"}
    var candidates []string

    for _, pattern := range patterns {
        matches, _ := filepath.Glob(pattern)
        candidates = append(candidates, matches...)
    }

    // 去重并排序
    seen := make(map[string]bool)
    var unique []string
    for _, file := range candidates {
        if !seen[file] {
            seen[file] = true
            unique = append(unique, file)
        }
    }
    sort.Strings(unique)

    // 如果找到文件，返回第一个
    if len(unique) > 0 {
        return unique[0]
    }

    return ""
}

func main() {
    // 创建客户端
    client, err := decenlicense.NewClient()
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    // 初始化配置 - 使用实际的许可证代码
    config := decenlicense.Config{
        LicenseCode:       "TEST-2025-001-FMGDXU", // 示例许可证代码
        PreferredMode:     decenlicense.ConnectionModeOffline,
        UDPPort:          13325,
        TCPPort:          23325,
    }

    err = client.Initialize(config)
    if err != nil {
        log.Printf("初始化失败 (需要产品公钥): %v", err)
        log.Println("请确保当前目录下有产品公钥文件 (public_*.pem)")
    }

    // 自动查找和设置产品公钥 (生产环境必需)
    productKeyPath := findProductPublicKey()
    if productKeyPath != "" {
        productKeyData, err := ioutil.ReadFile(productKeyPath)
        if err != nil {
            log.Printf("读取产品公钥文件失败: %v", err)
        } else {
            err = client.SetProductPublicKey(string(productKeyData))
            if err != nil {
                log.Printf("设置产品公钥失败: %v", err)
            } else {
                log.Printf("✅ 产品公钥已设置: %s", productKeyPath)
            }
        }
    } else {
        log.Println("⚠️  未找到产品公钥文件，请手动设置:")
        log.Println("   运行 dl-issuer 生成产品密钥，然后将产品公钥文件复制到当前目录")
    }

    // 激活令牌 - 使用加密令牌字符串
    tokenString := "RxkL7icTTtygiH5XlByp_lnHWZjPn-gxFmqKGX6V4CXC0ioc4Qs86df4ESUV0qe-dT9VGAhPTuYCFnTC67xZ0qZ_r6Y1mKdBzLLKh2Tfxwoa8wJDaFP8ttnZDXu1EDHr9p9CVm-kpf7v-acvYtwxaiu9WvhWuXjsdj--A8N-nBISJnmhw6FiAJPJw1j_vZTQkTW-jqSsXJArgDn_mNaC9q2re582Y7Ai0lsFu39u5vmqE3NJiQggnuZj5l3EHANjvuQnzi5x5ZQOGRMox5Sn8WzRiv4jpaMOSkJz7cxoZ6eeerSuqMiZ1NzgeOOmawn3pVC1UyYuzQsFtNLZU_Es-NIHVwF7w3rmKe9acpAuoQ1nAjdR8dRb4kota_4VCmd2pT0dIv3DigiUjGppk72QJdnVY8wgOQMA6Gq3Tt_rC-GXTwcfOvYDElOBs_M9d6bIT1h5LQtyLtxqqpkd0ECEnnl0aBhSwW716jTs_vzn5ReDjckMdkmp8J9TUOe3s-HW9Lhkje0CKMVN9jpH6H75hg1YPb0DqaGvznQINeuOduJZ1GQIiEeTl7Go-O9HsVgKXXDFKNQ1B6tNm8aStlv6mYzAdgof6YjnykPGLCBWBVGQPZY3bLkaMPoNlD_IruEzsiRU1ApggDNx6mVHwuXYbs2F4G8gvddYNqKW68GT3OS335ZJEEHz1CdJOsOuWQZxr6LDBvRXmtbYTZjtlIIrbV-4GjiW_wBTNKfwuPef7spp0QeeQsMED5PambiUVtpem7_BwnqFLDwIBOikBYaN7zcVjmT3uARdEGhPwfer4q1zklO_td3f2kMZKOFcve8eFmW-hKAHKNecC1XSN4tARsLa4OE6hUUDxvov7bXAYDO47BIDQdxmTzkFKgHUgSGmg0wgXwn8J8HYB-4zz-ZwLI6HLnpCC7LMrNcRKA7WIYsmfq0gPKNLIMCHSb9qNbNOsMNEhAk5yGdNmblD3ch_2PaIFQVTbRQoQln17ROVCJpVaOrPFdVEQbSPMkw_1aZ49HkVvZk7YSbcHrv6GdicHh3J0pFUdDM3HdHU1UM2O1ye32VaMA9pu-yJX3UufTwVyRu8vboAkZ6IjClFfpb2vfKgE51BEoXNQal5sdVqecw0-HAHNa4d2gj3UT8ZOFKj3XKRuHbEKO_soOwpd-Gqj_qRJd7oTeMxtpI3uOrLubaI33jSkDNcbeDvef9_JlDv3MrVcANadMjX4lkhwk1EgvBfz97IFnmm-0cGmwT0_aWF0JA9lPb5noYlF2EYeQklyX26Nezp_dtwVH5dy_C4ILIEohh0VU5Ix-_Dk8_z7BPK7UXtmVbfgmL84GLD-ogz5GSB8ohQdK6PDfnwEa9XkeGXeTDW3uaVTfOnrwnt1aHIjvlEvZETZeb_6cDX7gdgqD8XmDTuP5g8WGajTV7dR5sjq7ysIEZR8NP_otUAONP2|9Ur7e-wCHJkOUHXN"
    success, err := client.ActivateWithToken(tokenString)
    if err != nil {
        log.Printf("激活失败: %v", err)
    } else if success {
        fmt.Println("激活成功！")
    }

    // 检查状态
    activated, err := client.IsActivated()
    if err != nil {
        log.Printf("状态检查失败: %v", err)
    } else {
        fmt.Printf("激活状态: %t\n", activated)
    }
}
```

## 完整示例

### 运行演示程序

SDK包含示例代码，演示如何集成DecentriLicense：

```bash
# 编译演示程序
cd sdks/go/example
go build -o demo demo.go

# 运行演示
./demo
```

演示程序展示了：
- 客户端初始化
- 产品公钥设置
- Token激活流程
- 状态查询

## API 参考

### 配置参数

```go
type Config struct {
    LicenseCode       string         // 许可证代码，用于P2P冲突检测
    PreferredMode     ConnectionMode // 首选连接模式
    UDPPort           uint16         // UDP发现端口 (默认: 13325)
    TCPPort           uint16         // TCP通信端口 (默认: 23325)
    RegistryServerURL string         // WAN注册中心URL (可选，详见 server/ 目录)
}
```

### 连接模式

```go
const (
    ConnectionModeWANRegistry ConnectionMode = 0 // 广域网注册中心优先
    ConnectionModeLANP2P      ConnectionMode = 1 // 局域网P2P
    ConnectionModeOffline     ConnectionMode = 2 // 离线模式
)
```

### WAN服务器

当使用 `ConnectionModeWANRegistry` 模式时，需要配置广域网注册中心服务器：

```go
config := decenlicense.Config{
    PreferredMode:     decenlicense.ConnectionModeWANRegistry,
    RegistryServerURL: "http://your-registry-server:3883",
    // ... 其他配置
}
```

**WAN服务器位置**: `server/` 目录
**默认端口**: 3883
**编译运行**:
```bash
cd server
go build -o dl-server cmd/main.go
./dl-server -port 3883 -workers 4  # 使用4个CPU核心
```

**生产部署**:
```bash
./dl-server -port 3883 -workers 16  # 高并发配置
```

### 主要方法

#### NewClient()
创建新的DecentriLicense客户端实例。

#### Initialize(config Config)
使用指定配置初始化客户端。

#### SetProductPublicKey(key string)
设置产品公钥用于令牌验证。

#### ActivateWithToken(tokenString string)
使用令牌字符串激活许可证。

#### Activate()
激活许可证（传统方法）。

#### IsActivated()
检查许可证是否已激活。

#### GetCurrentToken()
获取当前令牌信息。

#### GetDeviceID()
获取设备ID。

#### GetDeviceState()
获取设备状态。

#### RecordUsage(payload string)
记录使用情况。

#### OfflineVerifyCurrentToken()
离线验证当前令牌。

#### GetStatus()
获取完整状态信息。

## 令牌格式

支持两种令牌格式：

### 1. JSON格式 (明文)
```json
{
  "token_id": "unique-token-id",
  "license_code": "LICENSE-CODE",
  "issue_time": 1640995200,
  "expire_time": 0,
  "signature": "base64-signature",
  "app_id": "APP-ID",
  "root_signature": "base64-root-sig"
}
```

### 2. 加密格式
```
[Base64加密令牌]|[解密密钥]
```

## 错误处理

所有方法都返回 `(result, error)` 格式，便于错误处理：

```go
success, err := client.ActivateWithToken(token)
if err != nil {
    log.Printf("激活失败: %v", err)
    return
}
if !success {
    log.Println("激活被拒绝")
    return
}
fmt.Println("激活成功")
```

## 编译选项

### CGO 配置

SDK使用CGO调用C库，编译时需要：

```bash
# 包含路径
#cgo CFLAGS: -I../../dl-core/include

# 库路径
#cgo LDFLAGS: -L../../dl-core/build -ldecentrilicense
```

### 依赖项

- Go 1.19+
- dl-core 已编译
- OpenSSL (通过dl-core)

## 故障排除

### 编译错误

**错误**: `ld: library not found for -ldecentrilicense`
**解决**: 确保dl-core已正确编译，库文件在指定路径中

**错误**: `fatal error: 'decenlicense_c.h' file not found`
**解决**: 检查dl-core头文件路径

### 运行时错误

**错误**: `dl_client_initialize failed`
**解决**: 检查产品公钥是否正确设置

**错误**: `激活失败`
**解决**: 检查令牌格式和签名是否有效

## 架构说明

### 智能降级架构

DecentriLicense采用三层智能降级架构：

1. **WAN层** - 广域网注册中心 (`server/` 目录)
   - 设备注册和心跳
   - 跨地域许可证协调
   - 令牌转移请求处理

2. **LAN层** - 局域网P2P (dl-core)
   - UDP 13325: 设备发现和心跳
   - TCP 23325: 选举协调和令牌传递

3. **Offline层** - 本地离线验证
   - 无需网络连接
   - 本地令牌验证
   - 状态链完整性检查

### 降级策略

系统会按优先级自动尝试连接：
- **首选**: WAN注册中心 (需要网络)
- **备选**: LAN P2P (局域网可用)
- **兜底**: Offline模式 (完全离线)

### 安全特性

- **RSA签名验证**: 保证令牌真实性
- **状态链溯源**: 防止篡改
- **一物一码**: 许可证唯一性保证
- **设备指纹**: 防复制保护

### 状态链

每个令牌维护状态链，支持：
- 使用量记录
- 时间戳验证
- 哈希链接完整性

## 贡献

欢迎提交问题和改进建议！

## 许可证

请查看项目根目录的LICENSE文件。