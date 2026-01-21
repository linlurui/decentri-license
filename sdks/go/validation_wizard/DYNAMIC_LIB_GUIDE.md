# DecentriLicense Go SDK - 动态库加载指南

## 问题说明

使用 `go run` 时,由于程序在临时目录编译,没有包含 rpath 信息,导致运行时找不到动态库:

```
dyld: Library not loaded: @rpath/libdecentrilicense.dylib
Reason: no LC_RPATH's found
```

## 解决方案

本项目已配置**自动动态识别**动态库路径,无需写死路径。

### 🎯 最佳实践:一键打包 + 直接 go run

**签发程序一键打包时的处理:**

```bash
# 在一键打包时调用注入脚本
cd sdks/go
./inject_dylib_path.sh /path/to/dl-core/build
```

这会自动修改`build_config.sh`,写入正确的动态库路径。

**打包后,用户只需要一次性配置:**

```bash
# 方式1: 当前终端session生效
cd sdks/go
source ./setup_go_env.sh

# 之后可以直接使用 go run
cd validation_wizard
go run validation_wizard.go  # ✅ 直接运行,无需任何包装!

# 方式2: 永久生效 (添加到 shell 配置)
echo 'source /path/to/sdks/go/build_config.sh' >> ~/.zshrc
# 重启终端后,任何位置都可以直接 go run
```

### 方案1: 使用 gorun.sh (无需配置环境)

```bash
cd sdks/go/validation_wizard
./gorun.sh
```

**原理**:
- 自动加载 `build_config.sh` 配置
- 自动查找动态库位置
- 通过 `-Wl,-rpath` 将路径编译进临时可执行文件

### 方案2: 手动 source 配置后使用 go run

```bash
cd sdks/go/validation_wizard
source ../build_config.sh
go run validation_wizard.go
```

**原理**:
- `build_config.sh` 会设置 `CGO_LDFLAGS` 环境变量
- 包含 `-Wl,-rpath,${DYLIB_PATH}` 参数
- 动态库路径会自动搜索并设置

### 方案3: 编译后运行 (性能最好)

```bash
cd sdks/go/validation_wizard
./build.sh          # 编译
./validation_wizard # 运行
```

**原理**:
- `build.sh` 使用 `install_name_tool` 添加 rpath
- 编译后的可执行文件包含永久 rpath

## 动态库路径查找机制

`build_config.sh` 会自动按以下顺序查找动态库:

1. 环境变量 `DECENTRI_DL_CORE_PATH` (如果已设置)
2. 相对路径自动搜索:
   - `../../dl-core/build`
   - `../../../dl-core/build`
   - `${PWD}/dl-core/build`
   - 等多个位置

3. 找到后自动设置:
   - `CGO_LDFLAGS="-L${DYLIB_PATH} -ldecentrilicense -Wl,-rpath,${DYLIB_PATH}"`
   - `DYLD_LIBRARY_PATH=${DYLIB_PATH}`

## 配置说明

### CGO_LDFLAGS 配置

```bash
# -L 指定库搜索路径
# -l 指定库名称
# -Wl,-rpath 将路径编译进可执行文件 (go run 关键)
CGO_LDFLAGS="-L${DYLIB_PATH} -ldecentrilicense -Wl,-rpath,${DYLIB_PATH}"
```

### DYLD_LIBRARY_PATH 配置 (备用)

```bash
# macOS 运行时库搜索路径
export DYLD_LIBRARY_PATH="${DYLIB_PATH}:${DYLD_LIBRARY_PATH}"

# Linux 运行时库搜索路径
export LD_LIBRARY_PATH="${DYLIB_PATH}:${LD_LIBRARY_PATH}"
```

## 常见问题

### Q: 为什么 go run 需要特殊处理?

A: `go run` 会在临时目录 (如 `/tmp/go-build*/`) 编译程序,如果不通过 `-Wl,-rpath` 将动态库路径编译进去,运行时就找不到库。

### Q: -Wl,-rpath 和 DYLD_LIBRARY_PATH 的区别?

A:
- `-Wl,-rpath`: 将路径**编译进可执行文件**,运行时自动查找
- `DYLD_LIBRARY_PATH`: **运行时环境变量**,每次运行都要设置

对于 `go run`, `-Wl,-rpath` 是必需的。

### Q: 可以自定义动态库路径吗?

A: 可以,通过环境变量:

```bash
export DECENTRI_DL_CORE_PATH=/your/custom/path
source ../build_config.sh
go run validation_wizard.go
```

## 总结

### ❓ 问题: rpath不写死的话是不是没办法go run?

**答案: 可以!** 有两种方式:

#### 1. 通过环境变量(推荐)
```bash
# 一次性配置(打包后执行一次)
source ./setup_go_env.sh

# 之后可以无限次直接 go run
go run validation_wizard.go
```

**原理**:
- 设置`CGO_LDFLAGS`环境变量,包含`-Wl,-rpath,${DYLIB_PATH}`
- Go编译时自动将rpath编译进临时可执行文件

#### 2. 通过包装脚本
```bash
./gorun.sh  # 自动加载配置并运行
```

### ✅ 能不能动态识别?

**答案: 可以!** 已实现智能动态识别:

1. **自动搜索**: `build_config.sh`会自动搜索多个可能的路径
2. **环境变量**: 支持`DECENTRI_DL_CORE_PATH`环境变量
3. **打包注入**: 一键打包时调用`inject_dylib_path.sh`写入路径

**无需写死路径!**

### 🎯 推荐工作流

**开发阶段**:
```bash
cd sdks/go/validation_wizard
./gorun.sh  # 最简单
```

**打包发布**:
```bash
# 签发程序一键打包时
cd sdks/go
./inject_dylib_path.sh /path/to/dl-core/build

# 用户收到包后
source ./setup_go_env.sh  # 只需一次
go run *.go  # 之后可以随意 go run
```

**生产环境**:
```bash
./build.sh  # 编译
./validation_wizard  # 运行编译好的版本 (性能最好)
```

✅ **不需要写死路径**
✅ **自动动态识别**
✅ **支持 go run**
✅ **跨平台兼容** (macOS/Linux)

推荐使用 `./gorun.sh` 最方便!
