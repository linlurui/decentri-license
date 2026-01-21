# DecentriLicense Token 使用指南

## Token 类型说明

DecentriLicense 系统中有三种类型的 Token：

### 1. 加密Token (Encrypted Token)
**文件名格式**: `token_*encrypted.txt`

- **来源**: 从软件供应商处首次获得
- **用途**: 需要进行首次激活
- **包含信息**: 基础许可证信息（加密格式）
- **使用次数**: 仅用于首次激活

### 2. 已激活Token (Activated Token)
**文件名格式**: `token_activated_*.txt`

- **来源**: 首次激活后自动生成
- **用途**: 可以直接导入使用，无需再次激活
- **包含信息**: 许可证信息 + 设备绑定信息
- **使用次数**: 可重复使用
- **重要性**: ⭐⭐⭐ 这是首次激活的核心成果

### 3. 状态变更Token (State Changed Token)
**文件名格式**: `token_state_*_idx*.txt`

- **来源**: 每次记账操作后自动生成
- **用途**: 包含最新的使用记录链
- **包含信息**: 许可证信息 + 设备绑定 + 使用记录链
- **使用次数**: 可重复使用
- **重要性**: ⭐⭐⭐ 记录了完整的使用历史

## 正确的使用流程

### 场景一：首次激活（新设备）

```
加密Token (encrypted)
    ↓
【选项1: 激活令牌】
    ↓
ImportToken + ActivateBindDevice
    ↓
生成已激活Token (activated)
    ↓
✅ 保存此Token供后续使用
```

**重要**: 激活后生成的 `token_activated_*.txt` 文件非常重要，应妥善保存。

### 场景二：后续使用（同一设备）

```
已激活Token (activated) 或 状态变更Token (state)
    ↓
【选项4: 记账信息】
    ↓
ImportToken + ActivateBindDevice（恢复激活状态）
    ↓
✅ 可以使用，token内容不变
```

**关键**: 需要调用 `ActivateBindDevice`，但这是**幂等操作**：
- 对于已激活token，不会重新生成新token
- 只是恢复客户端的激活状态
- Token内容保持不变

### 场景三：记账操作

```
当前已激活的Token
    ↓
【选项4: 记账信息】
    ↓
RecordUsage（记录使用信息）
    ↓
生成新的状态变更Token (state)
    ↓
✅ 新Token包含最新的使用记录链
```

每次记账后都会生成新的 Token，新 Token 包含了完整的使用历史。

## 常见问题

### Q1: 为什么首次激活后要生成新的Token？

**A**: 首次激活时，系统会：
1. 绑定当前设备ID
2. 创建设备专属的加密密钥
3. 生成包含设备信息的新Token

这个新Token包含了设备绑定信息，后续使用时不需要再次激活。

### Q2: 已激活的Token可以在其他设备使用吗？

**A**: 可以，但需要注意：
- Token中包含了原设备的绑定信息
- 在新设备上导入后，系统会检测到设备不匹配
- 根据许可证策略，可能需要重新激活或允许多设备使用

### Q3: 每次记账都要重新激活吗？

**A**: ✅ **需要调用 `ActivateBindDevice`，但这不是"重新激活"**

SDK的设计逻辑：
- `ImportToken` 只导入token数据，不设置激活状态
- `ActivateBindDevice` 设置/恢复激活状态（对已激活token是幂等操作）
- `RecordUsage` 需要激活状态才能执行

对于已激活token：
- 调用 `ActivateBindDevice` 只是恢复激活状态
- **不会重新生成新token**
- Token内容保持不变，只是客户端状态变为"已激活"

### Q4: 如果丢失了已激活的Token怎么办？

**A**:
1. 检查 `.decentrilicense_state/` 目录下的本地状态
2. 如果本地状态存在，可以继续使用
3. 如果本地状态也丢失，需要使用原始加密Token重新激活

## 向导工具使用说明

### 选项1: 🔓 激活令牌
- **用途**: 仅用于首次激活加密Token
- **输入**: 加密Token (`token_*encrypted.txt`)
- **输出**: 已激活Token (`token_activated_*.txt`)
- **重要**: 激活成功后保存输出的Token文件

### 选项2: ✅ 校验已激活令牌
- **用途**: 验证已激活的Token是否有效
- **输入**: 本地状态目录中的Token
- **特点**: 不需要再次激活

### 选项3: 🔍 验证令牌合法性
- **用途**: 验证任意Token的签名合法性
- **输入**: 任意类型的Token
- **特点**: 仅验证签名，不激活

### 选项4: 📊 记账信息
- **用途**: 记录使用信息到Token
- **输入**: 已激活Token或状态变更Token
- **输出**: 新的状态变更Token (`token_state_*.txt`)
- **特点**: 自动识别Token类型，已激活Token不会重复激活

## 最佳实践

1. **妥善保存Token文件**
   - 首次激活后的 `token_activated_*.txt` 是核心资产
   - 定期备份最新的 `token_state_*.txt`

2. **正确识别Token类型**
   - 文件名包含 `encrypted` → 需要激活
   - 文件名包含 `activated` 或 `state` → 可直接使用

3. **理解ActivateBindDevice的幂等性**
   - 首次激活：encrypted token → 生成新的activated token
   - 恢复状态：activated token → 不生成新token，只恢复状态
   - 每次导入token后都需要调用，但行为不同

4. **保持状态同步**
   - 记账后使用最新的 `token_state_*.txt`
   - 最新的Token包含完整的使用历史链

## 技术细节

### SDK调用流程

```go
// 1. 导入Token（所有类型Token都需要）
client.ImportToken(tokenString)

// 2. 激活/恢复状态（所有场景都需要）
client.ActivateBindDevice()
// - 加密Token：首次激活，生成新token
// - 已激活Token：恢复状态，token不变（幂等操作）

// 3. 验证Token（可选）
client.OfflineVerifyCurrentToken()

// 4. 记账操作（需要激活状态）
client.RecordUsage(accountingData)
```

**关键理解**：`ActivateBindDevice` 不是"激活"，而是"确保激活状态"
- 首次调用：执行激活，生成新token
- 后续调用：恢复状态，token不变

### Token状态转换

```
[加密Token]
    ↓ ActivateBindDevice
[已激活Token] (state_index = 0)
    ↓ RecordUsage
[状态变更Token] (state_index = 1)
    ↓ RecordUsage
[状态变更Token] (state_index = 2)
    ↓ ...
```

每次记账后，`state_index` 会递增，形成不可篡改的使用记录链。

## SDK设计说明

### ActivateBindDevice 的幂等性

**核心理解**：`ActivateBindDevice` 对不同类型的token行为不同：

1. **加密Token（首次激活）**
   ```go
   client.ImportToken(encryptedToken)
   client.ActivateBindDevice()  // 执行激活，生成新token
   newToken := client.ExportActivatedTokenEncrypted()  // 返回新token
   ```

2. **已激活Token（恢复状态）**
   ```go
   client.ImportToken(activatedToken)
   client.ActivateBindDevice()  // 恢复状态，token不变
   newToken := client.ExportActivatedTokenEncrypted()  // 返回空或相同token
   ```

### 为什么需要调用 ActivateBindDevice？

`ImportToken` 和 `ActivateBindDevice` 的职责分离：
- `ImportToken`：仅导入token数据到内存
- `ActivateBindDevice`：设置客户端为"已激活"状态

`RecordUsage` 等操作需要客户端处于"已激活"状态，因此即使是已激活token，也需要调用 `ActivateBindDevice` 来恢复这个状态。

### 实现逻辑

```go
// accountingWizard 的正确实现
if isAlreadyActivated {
    fmt.Println("💡 检测到已激活令牌")
    fmt.Println("🔄 正在恢复激活状态...")
} else {
    fmt.Println("🎯 正在首次激活令牌...")
}

// 统一调用ActivateBindDevice
// - 已激活token：幂等操作，恢复状态
// - 加密token：执行激活，生成新token
client.ActivateBindDevice()
```

**结论**：首次激活生成的Token仍然有意义：
- 后续使用时导入此token
- 调用 `ActivateBindDevice` 恢复状态（不生成新token）
- 可以继续使用，无需重新绑定设备
