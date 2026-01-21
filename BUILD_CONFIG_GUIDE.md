# DecentriLicense SDK 构建配置使用说明

## 📋 概述

所有语言SDK现在都使用统一的 `build_config.sh` 配置文件来管理动态库路径。一键打包程序只需要更新这个文件即可。

## 🗂️ 配置文件位置

```
sdks/
├── go/build_config.sh
├── python/build_config.sh
├── rust/build_config.sh
├── java/build_config.sh
├── javascript/build_config.sh
├── php/build_config.sh
├── csharp/build_config.sh
└── c/build_config.sh
```

## 🔧 一键打包程序集成方式

### 方法1: 自动获取动态库路径并批量更新（推荐）

```bash
#!/bin/bash
# 一键打包程序示例

# 1. 获取dl-core/build的绝对路径
DYLIB_ABSOLUTE_PATH="$(cd dl-core/build && pwd)"

# 2. 批量更新所有SDK的build_config.sh
for config_file in sdks/*/build_config.sh; do
    echo "更新: $config_file"
    sed -i '' "s|export DYLIB_PATH_OVERRIDE=\"\"|export DYLIB_PATH_OVERRIDE=\"${DYLIB_ABSOLUTE_PATH}\"|g" "$config_file"
done

echo "✅ 所有SDK配置已更新为: ${DYLIB_ABSOLUTE_PATH}"
```

### 方法2: 不更新配置文件（使用自动模式）

如果一键打包程序不更新 `build_config.sh`，SDK会自动计算相对路径并转换为绝对路径。这适用于开发环境。

## 📝 配置文件格式

每个 `build_config.sh` 都包含：

```bash
# AUTO_REPLACE_DYLIB_PATH_START
export DYLIB_PATH_OVERRIDE=""
# AUTO_REPLACE_DYLIB_PATH_END
```

一键打包程序需要替换 `DYLIB_PATH_OVERRIDE=""` 这一行中的空字符串为实际的绝对路径。

### 示例：

**更新前：**
```bash
export DYLIB_PATH_OVERRIDE=""
```

**更新后：**
```bash
export DYLIB_PATH_OVERRIDE="/Users/lamrocky/project/decentri-license-issuer/dl-core/build"
```

## 🚀 用户使用方式

### Go SDK

```bash
# 编译
cd sdks/go/validation_wizard
./build.sh

# 运行
./run.sh
```

### Python SDK

```bash
cd sdks/python
source build_config.sh
python your_script.py
```

### 其他SDK

类似地，先 `source build_config.sh` 加载配置，然后正常使用。

## 🔍 验证配置是否生效

```bash
# 检查某个SDK的配置
source sdks/go/build_config.sh
echo "动态库路径: $DYLIB_PATH"
echo "DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
```

## 💡 工作原理

1. **开发模式**（DYLIB_PATH_OVERRIDE为空）
   - SDK自动计算相对路径 `../../dl-core/build` 并转换为绝对路径
   - 适合源码构建环境

2. **打包模式**（DYLIB_PATH_OVERRIDE已设置）
   - 使用一键打包程序预设的绝对路径
   - 适合分发打包后的版本

## 📦 一键打包程序完整示例

```bash
#!/bin/bash
# 完整的一键打包脚本

echo "🚀 DecentriLicense 一键打包程序"
echo ""

# 1. 编译dl-core
echo "📦 编译dl-core..."
cd dl-core
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
cd ../..

# 2. 获取动态库绝对路径
DYLIB_PATH="$(cd dl-core/build && pwd)"
echo "✅ 动态库路径: ${DYLIB_PATH}"

# 3. 更新所有SDK配置
echo "🔧 更新SDK配置..."
for config in sdks/*/build_config.sh; do
    if [ -f "$config" ]; then
        sed -i '' "s|export DYLIB_PATH_OVERRIDE=\"\"|export DYLIB_PATH_OVERRIDE=\"${DYLIB_PATH}\"|g" "$config"
        echo "  ✅ ${config}"
    fi
done

# 4. 编译Go SDK验证向导
echo "📦 编译Go SDK验证向导..."
cd sdks/go/validation_wizard
./build.sh
cd ../../..

echo ""
echo "🎉 一键打包完成！"
echo ""
echo "现在可以运行验证："
echo "  cd sdks/go/validation_wizard && ./run.sh"
```

## ⚠️ 注意事项

1. **sed命令差异**: macOS使用 `sed -i ''`，Linux使用 `sed -i`
2. **路径分隔符**: 使用 `|` 作为sed分隔符，避免路径中的 `/` 干扰
3. **权限问题**: 确保build_config.sh有执行权限
4. **Git提交**: build_config.sh应该提交到仓库（带空的DYLIB_PATH_OVERRIDE）

## 🔄 恢复默认配置

如果需要恢复到开发模式：

```bash
# 批量恢复
for config in sdks/*/build_config.sh; do
    sed -i '' 's|export DYLIB_PATH_OVERRIDE=".*"|export DYLIB_PATH_OVERRIDE=""|g' "$config"
done
```

---

**最后更新**: 2026-01-15
**维护者**: Linlurui
