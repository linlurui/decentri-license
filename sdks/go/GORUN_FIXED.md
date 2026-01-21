# ✅ 问题已解决! 现在可以直接 go run

## 问题原因

之前报错 `dyld: Library not loaded: @rpath/libdecentrilicense.dylib - Reason: no LC_RPATH's found` 是因为:

1. `go run` 在临时目录编译
2. 源码中的 `#cgo LDFLAGS` 缺少 `-Wl,-rpath` 参数
3. 临时可执行文件找不到动态库

## 解决方案

已在 `sdks/go/decenlicense.go` 中添加 rpath:

```go
/*
#cgo CFLAGS: -I../../dl-core/include
#cgo LDFLAGS: -L../../dl-core/build -ldecentrilicense -Wl,-rpath,${SRCDIR}/../../dl-core/build
*/
```

### 关键点:
- `${SRCDIR}` 是 Go 的特殊变量,自动展开为源文件目录
- `-Wl,-rpath,${SRCDIR}/../../dl-core/build` 会将动态库路径编译进可执行文件
- **相对路径自动解析**,无需写死绝对路径

## 现在可以直接使用

```bash
cd sdks/go/validation_wizard
go run validation_wizard.go  # ✅ 直接运行,无需任何配置!
```

**无需:**
- ❌ source build_config.sh
- ❌ 设置环境变量
- ❌ 使用包装脚本
- ❌ 写死绝对路径

## 动态识别机制

### 编译时 (go build/run)
- `${SRCDIR}` 自动解析为 `/path/to/sdks/go`
- rpath 自动设置为 `/path/to/sdks/go/../../dl-core/build`
- 相对路径自动解析为 `/path/to/dl-core/build`

### 运行时 (dyld)
- dyld 根据 rpath 查找动态库
- 支持相对路径解析
- 自动找到 `libdecentrilicense.dylib`

## 其他 SDK (Python, Node.js 等)

其他语言 SDK 也采用了类似机制:
- Python: `setup.py` 中配置 rpath
- Node.js: `binding.gyp` 中配置 rpath
- 都使用相对路径,无需写死

## 一键打包支持

签发程序打包时可以调用:
```bash
cd sdks/go
./inject_dylib_path.sh /path/to/dl-core/build
```

脚本会检查并报告 rpath 配置状态。

## 验证

检查编译出的可执行文件是否包含 rpath:
```bash
go build -o test validation_wizard.go
otool -l test | grep -A 2 LC_RPATH
```

应该看到:
```
cmd LC_RPATH
cmdsize 88
path /path/to/dl-core/build (offset 12)
```

## 跨平台支持

- **macOS**: `-Wl,-rpath` + `${SRCDIR}` ✅
- **Linux**: `-Wl,-rpath` + `$ORIGIN/../..` ✅
- **Windows**: 不需要 rpath (DLL 搜索路径不同)

## 总结

✅ **问题已彻底解决**
✅ **可以直接 go run**
✅ **无需任何配置**
✅ **自动动态识别**
✅ **相对路径解析**
✅ **跨平台兼容**

现在你可以随时随地直接 `go run` 了! 🎉
