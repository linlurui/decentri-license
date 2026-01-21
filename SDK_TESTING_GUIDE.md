# DecentriLicense SDK 手工测试指南

**创建时间**: 2026-01-15 12:00

本指南提供所有9个SDK的validation_wizard手工测试步骤。

---

## 📋 测试前准备

### 1. 准备测试文件

在测试前，您需要准备以下文件：

- **产品公钥文件**: `public_*.pem` (包含ROOT_SIGNATURE)
- **Token文件**:
  - 加密token: `token_*_encrypted.txt`
  - 或已激活token: `token_activated_*.txt`
  - 或状态token: `token_state_*.txt`

### 2. 测试建议

建议的测试流程：
1. 先测试**退出功能**（选项7），确认菜单显示正确
2. 测试**选择产品公钥**（选项0）
3. 测试**激活令牌**（选项1）
4. 测试其他验证功能（选项2-6）

### 3. 快速测试命令

如果只想验证菜单是否正确，可以使用：
```bash
echo "7" | <运行命令>
```

---

## 1️⃣ Go SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/go
```

### 编译（如果需要）
```bash
go build -o validation_wizard validation_wizard/validation_wizard.go
```

### 运行
```bash
./validation_wizard
```

### 快速测试（仅验证菜单）
```bash
echo "7" | ./validation_wizard
```

### 预期输出
应该看到8个菜单选项（0-7），包括：
- 0. 🔑 选择产品公钥
- 1. 🔓 激活令牌
- ...
- 7. 🚪 退出

---

## 2️⃣ Python SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/python
```

### 运行（无需编译）
```bash
python3 validation_wizard.py
```

### 快速测试
```bash
echo "7" | python3 validation_wizard.py
```

### 注意事项
- 需要Python 3.6+
- 确保已安装依赖: `pip install -r requirements.txt`（如果有）

---

## 3️⃣ Node.js SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/nodejs
```

### 运行（无需编译）
```bash
node validation_wizard.js
```

### 快速测试
```bash
echo "7" | node validation_wizard.js
```

### 注意事项
- 需要Node.js 12+
- 确保已安装依赖: `npm install`（如果有package.json）

---

## 4️⃣ Java SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/java
```

### 编译
```bash
# 使用Maven编译
mvn clean compile

# 或使用Gradle（如果有build.gradle）
gradle build
```

### 运行
```bash
# 方式1: 使用Maven
mvn exec:java -Dexec.mainClass="com.decentrilicense.ValidationWizard"

# 方式2: 直接运行编译后的class
java -cp target/classes:lib/* com.decentrilicense.ValidationWizard
```

### 快速测试
```bash
echo "7" | mvn exec:java -Dexec.mainClass="com.decentrilicense.ValidationWizard"
```

### 注意事项
- 需要Java 8+
- 确保JNI库路径正确（libdecentrilicense.so/dylib/dll）

---

## 5️⃣ PHP SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/php
```

### 运行（无需编译）
```bash
php validation_wizard.php
```

### 快速测试
```bash
echo "7" | php validation_wizard.php
```

### 注意事项
- 需要PHP 7.4+
- 需要FFI扩展: `php -m | grep FFI`
- 确保C库路径正确

---

## 6️⃣ C SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/c
```

### 编译
```bash
# 编译validation_wizard
gcc -o validation_wizard validation_wizard.c -I. -L./lib -ldecentrilicense -std=c99

# 或使用提供的Makefile（如果有）
make validation_wizard
```

### 运行
```bash
# macOS系统
DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH ./validation_wizard

# Linux系统
LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH ./validation_wizard
```

### 快速测试
```bash
# macOS
echo "7" | DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH ./validation_wizard

# Linux
echo "7" | LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH ./validation_wizard
```

### 注意事项
- 需要C99编译器（gcc/clang）
- 确保libdecentrilicense库在lib目录下
- macOS使用DYLD_LIBRARY_PATH，Linux使用LD_LIBRARY_PATH

---

## 7️⃣ C++ SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/cpp/example
```

### 编译
```bash
# 使用g++编译（C++14标准）
g++ -std=c++14 -o validation_wizard validation_wizard.cpp -I../include -L../lib -ldecentrilicense

# 或使用CMake（如果有CMakeLists.txt）
mkdir -p build && cd build
cmake ..
make
```

### 运行
```bash
# macOS系统
DYLD_LIBRARY_PATH=../lib:$DYLD_LIBRARY_PATH ./validation_wizard

# Linux系统
LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH ./validation_wizard
```

### 快速测试
```bash
# macOS
echo "7" | DYLD_LIBRARY_PATH=../lib:$DYLD_LIBRARY_PATH ./validation_wizard

# Linux
echo "7" | LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH ./validation_wizard
```

### 注意事项
- 需要C++14编译器（g++ 5+, clang++ 3.4+）
- 确保include路径指向../include
- 确保库路径指向../lib

---

## 8️⃣ Rust SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/rust
```

### 编译
```bash
# 使用Cargo编译
cargo build --example validation_wizard

# 或编译release版本（更快）
cargo build --release --example validation_wizard
```

### 运行
```bash
# 运行debug版本
DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH cargo run --example validation_wizard

# 或直接运行编译后的二进制文件
DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH ./target/debug/examples/validation_wizard
```

### 快速测试
```bash
# macOS
echo "7" | DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH cargo run --example validation_wizard

# Linux
echo "7" | LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH cargo run --example validation_wizard
```

### 注意事项
- 需要Rust 1.56+（支持2021 edition）
- 编译时可能有deprecation警告（chrono库），可以忽略
- 确保Cargo.toml中包含validation_wizard example配置

---

## 9️⃣ C# SDK

### 目录
```bash
cd /Volumes/project/decentri-license-issuer/sdks/csharp
```

### 编译
```bash
# 使用dotnet编译
dotnet build

# 或使用MSBuild（如果有.sln文件）
msbuild DecentriLicense.sln
```

### 运行
```bash
# 使用dotnet运行
dotnet run --project DecentriLicense

# 或直接运行编译后的exe/dll
dotnet bin/Debug/netcoreapp3.1/DecentriLicense.dll
```

### 快速测试
```bash
echo "7" | dotnet run --project DecentriLicense
```

### 注意事项
- 需要.NET Core 3.1+ 或 .NET 5+
- **按用户要求，C# SDK无需测试**（无测试环境）
- 代码已完成重写，功能与其他SDK一致

---

## 📝 测试检查清单

对每个SDK进行测试时，请确认以下内容：

### ✅ 菜单显示检查
- [ ] 显示8个菜单选项（0-7）
- [ ] 选项0: 🔑 选择产品公钥
- [ ] 选项1: 🔓 激活令牌
- [ ] 选项2: ✅ 校验已激活令牌
- [ ] 选项3: 🔍 验证令牌合法性
- [ ] 选项4: 📊 记账信息
- [ ] 选项5: 🔗 信任链验证
- [ ] 选项6: 🎯 综合验证
- [ ] 选项7: 🚪 退出

### ✅ 功能测试检查
- [ ] 选项0能正确发现并选择产品公钥文件
- [ ] 选项1能正确激活令牌并导出token文件
- [ ] 选项2能正确验证已激活的令牌
- [ ] 选项3能正确验证令牌合法性
- [ ] 选项4能正确记录使用量并导出状态token
- [ ] 选项5能执行4项信任链验证
- [ ] 选项6能执行5项综合验证
- [ ] 选项7能正常退出程序

---

## 🚀 快速测试命令汇总

以下是所有SDK的快速测试命令（仅验证菜单显示）：

```bash
# 1. Go SDK
cd /Volumes/project/decentri-license-issuer/sdks/go
echo "7" | ./validation_wizard

# 2. Python SDK
cd /Volumes/project/decentri-license-issuer/sdks/python
echo "7" | python3 validation_wizard.py

# 3. Node.js SDK
cd /Volumes/project/decentri-license-issuer/sdks/nodejs
echo "7" | node validation_wizard.js

# 4. Java SDK
cd /Volumes/project/decentri-license-issuer/sdks/java
echo "7" | mvn exec:java -Dexec.mainClass="com.decentrilicense.ValidationWizard"

# 5. PHP SDK
cd /Volumes/project/decentri-license-issuer/sdks/php
echo "7" | php validation_wizard.php

# 6. C SDK
cd /Volumes/project/decentri-license-issuer/sdks/c
echo "7" | DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH ./validation_wizard

# 7. C++ SDK
cd /Volumes/project/decentri-license-issuer/sdks/cpp/example
echo "7" | DYLD_LIBRARY_PATH=../lib:$DYLD_LIBRARY_PATH ./validation_wizard

# 8. Rust SDK
cd /Volumes/project/decentri-license-issuer/sdks/rust
echo "7" | DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH cargo run --example validation_wizard

# 9. C# SDK (可选，无测试环境)
cd /Volumes/project/decentri-license-issuer/sdks/csharp
echo "7" | dotnet run --project DecentriLicense
```

---

## ❓ 常见问题解答

### Q1: 编译时找不到库文件怎么办？

**A:** 确保libdecentrilicense库文件在正确的位置：
- C/C++: 检查`lib/`目录下是否有`libdecentrilicense.dylib`（macOS）或`libdecentrilicense.so`（Linux）
- 编译时使用`-L./lib`或`-L../lib`指定库路径

### Q2: 运行时提示"library not loaded"怎么办？

**A:** 需要设置动态库搜索路径：
- macOS: `export DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH`
- Linux: `export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH`

### Q3: Java SDK提示找不到JNI库怎么办？

**A:** 确保JNI库在系统库路径中，或使用：
```bash
java -Djava.library.path=./lib -cp target/classes:lib/* com.decentrilicense.ValidationWizard
```

### Q4: PHP SDK提示FFI扩展未安装怎么办？

**A:** 安装并启用FFI扩展：
```bash
# 检查是否已安装
php -m | grep FFI

# 如果未安装，在php.ini中启用
extension=ffi
```

### Q5: 所有SDK的菜单选项应该是什么？

**A:** 所有SDK应该显示完全相同的8个菜单选项（0-7）：
- 0. 🔑 选择产品公钥
- 1. 🔓 激活令牌
- 2. ✅ 校验已激活令牌
- 3. 🔍 验证令牌合法性
- 4. 📊 记账信息
- 5. 🔗 信任链验证
- 6. 🎯 综合验证
- 7. 🚪 退出

---

## 📊 测试总结

### 测试优先级

建议按以下优先级进行测试：

1. **高优先级**（必须测试）:
   - Go SDK（参考标准）
   - Python SDK
   - Node.js SDK
   - Java SDK
   - PHP SDK

2. **中优先级**（建议测试）:
   - C SDK
   - C++ SDK
   - Rust SDK

3. **低优先级**（可选）:
   - C# SDK（无测试环境）

### 预期结果

所有SDK应该：
- 显示完全相同的8个菜单选项（0-7）
- 功能行为完全一致
- 能够互相导入和验证token文件
- 生成的token文件格式相同

### 测试报告

测试完成后，建议记录：
- 每个SDK的测试状态（通过/失败）
- 发现的问题和错误信息
- 各SDK之间的差异（如果有）

---

## 🎯 开始测试

现在您可以按照本指南开始测试各个SDK了！

**建议的测试流程**：
1. 先运行快速测试命令，确认所有SDK的菜单显示正确
2. 选择一个SDK（建议Go SDK）进行完整的功能测试
3. 使用生成的token文件在其他SDK中进行交叉验证
4. 记录测试结果

祝测试顺利！如有问题，请参考常见问题解答部分。

