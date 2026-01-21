# Validation Wizard - 验证向导

DecentriLicense Go SDK 的交互式验证工具，提供完整的SDK功能测试环境。

## 快速启动

```bash
./validation_wizard
```

## 功能概览

| 功能 | 说明 | 文档 |
|------|------|------|
| **0. 选择产品公钥** | 选择验证用的产品公钥文件 | - |
| **1. 激活令牌** | 首次激活加密token | [Token使用指南](TOKEN_USAGE_GUIDE.md) |
| **2. 校验已激活令牌** | 验证本地已激活的token | [Token使用指南](TOKEN_USAGE_GUIDE.md) |
| **3. 验证令牌合法性** | 验证任意token签名合法性 | [Token使用指南](TOKEN_USAGE_GUIDE.md) |
| **4. 记账信息** | 记录使用情况到状态链 | [Token使用指南](TOKEN_USAGE_GUIDE.md) |
| **5. 信任链验证** | 验证完整签名信任链 | [验证功能说明](VALIDATION_FEATURES.md) |
| **6. 综合验证** | 全面测试所有SDK功能 | [验证功能说明](VALIDATION_FEATURES.md) |

## 文档

- **[TOKEN_USAGE_GUIDE.md](TOKEN_USAGE_GUIDE.md)** - Token类型、使用流程、SDK设计原理
- **[VALIDATION_FEATURES.md](VALIDATION_FEATURES.md)** - 信任链验证和综合验证详解

## Token类型

### 1. 加密Token (`token_*encrypted.txt`)
- 从供应商获得
- 需要首次激活
- 激活后生成activated token

### 2. 已激活Token (`token_activated_*.txt`)
- 首次激活后自动生成
- 包含设备绑定信息
- 可重复使用

### 3. 状态Token (`token_state_*_idx*.txt`)
- 记账后自动生成
- 包含完整使用记录链
- state_index递增

## 编译

```bash
go build -o validation_wizard validation_wizard.go
```

## 依赖

- Go 1.19+
- dl-core (已编译的C++库)
- 产品公钥文件 (*.pem)

## 使用场景

- **开发测试** - 验证SDK功能
- **集成测试** - 端到端测试
- **故障排查** - 定位问题
- **演示工具** - 展示功能

## 注意事项

1. **全局状态** - 向导使用全局client实例，状态在会话期间保持
2. **Token管理** - 激活和记账操作会生成新的token文件
3. **产品公钥** - 首次使用需要选择正确的产品公钥
4. **离线模式** - 默认使用离线模式，无需网络连接

## 更多信息

查看上级目录的 [Go SDK README](../README.md) 了解完整的SDK文档。
