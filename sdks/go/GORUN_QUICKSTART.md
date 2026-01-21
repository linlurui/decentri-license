# Go Run 快速指南

## 问题
使用 `go run` 时报错: `dyld: Library not loaded: @rpath/libdecentrilicense.dylib`

## 解决方案

### ✅ 方案1: 一键打包 + 一次性配置 (推荐)

**在签发程序一键打包时:**
```bash
cd sdks/go
./inject_dylib_path.sh /path/to/dl-core/build
```

**用户收到包后,只需执行一次:**
```bash
cd sdks/go
source ./setup_go_env.sh
```

**之后可以无限次直接使用 go run:**
```bash
cd validation_wizard
go run validation_wizard.go  # ✅ 成功!
```

### ✅ 方案2: 使用 gorun.sh (无需配置)

```bash
cd sdks/go/validation_wizard
./gorun.sh
```

### ✅ 方案3: 永久配置 (添加到 shell)

```bash
echo 'source /path/to/sdks/go/build_config.sh' >> ~/.zshrc
source ~/.zshrc
# 之后任何终端都可以直接 go run
```

## 原理

### 为什么需要特殊处理?
- `go run` 在临时目录编译 (如 `/tmp/go-build*/`)
- 临时可执行文件没有 rpath 信息
- 找不到动态库

### 解决方式
通过 `CGO_LDFLAGS` 环境变量添加 `-Wl,-rpath` 参数:
```bash
export CGO_LDFLAGS="-L${DYLIB_PATH} -ldecentrilicense -Wl,-rpath,${DYLIB_PATH}"
```

这样 Go 编译临时文件时会自动包含 rpath!

## 动态识别

### 智能路径搜索
`build_config.sh` 会自动搜索动态库:
1. 环境变量 `DECENTRI_DL_CORE_PATH`
2. 一键打包注入的路径
3. 相对路径自动搜索 (多个位置)

### 无需写死路径!
✅ 自动识别
✅ 支持自定义
✅ 跨平台兼容

## 快速对比

| 方式 | 需要配置 | 灵活性 | 性能 |
|------|---------|--------|------|
| gorun.sh | 无 | 中 | 中 |
| source + go run | 一次 | 高 | 中 |
| build.sh + 运行 | 无 | 低 | 最佳 |

## 推荐

- **开发**: `./gorun.sh`
- **打包后**: `source setup_go_env.sh` + `go run`
- **生产**: `./build.sh` + `./validation_wizard`
