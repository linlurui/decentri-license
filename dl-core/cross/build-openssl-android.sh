#!/usr/bin/env bash
# 用 Android NDK 交叉编译 OpenSSL 静态库（每个 ABI 一份）
# 输出: $PREFIX_BASE/<ABI>/{include,lib/libssl.a,lib/libcrypto.a}
set -euo pipefail

OPENSSL_SRC="${OPENSSL_SRC:-$HOME/dl-thirdparty/openssl}"
ANDROID_NDK="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk/30.0.14904198}"
API="${ANDROID_API:-24}"
PREFIX_BASE="${PREFIX_BASE:-$HOME/dl-thirdparty/openssl-android}"
ABIS_INPUT="${1:-arm64-v8a}"   # 用空格分隔，如: "arm64-v8a armeabi-v7a x86_64"

[ -d "$OPENSSL_SRC" ] || { echo "缺少 OpenSSL 源码: $OPENSSL_SRC"; exit 1; }
[ -d "$ANDROID_NDK" ] || { echo "缺少 NDK: $ANDROID_NDK"; exit 1; }

HOST_TAG="darwin-x86_64"   # NDK r25+ 的 toolchain 目录都是 darwin-x86_64（含 arm64 macOS）
TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/$HOST_TAG"
export ANDROID_NDK_ROOT="$ANDROID_NDK"
export PATH="$TOOLCHAIN/bin:$PATH"

build_one() {
  local ABI="$1"
  local OPENSSL_TARGET
  local EXTRA_OPTS=""
  case "$ABI" in
    arm64-v8a)    OPENSSL_TARGET="android-arm64" ;;
    armeabi-v7a) OPENSSL_TARGET="android-arm"; EXTRA_OPTS="no-asm" ;;
    x86)          OPENSSL_TARGET="android-x86" ;;
    x86_64)       OPENSSL_TARGET="android-x86_64" ;;
    *) echo "未知 ABI: $ABI"; return 1 ;;
  esac

  local PREFIX="$PREFIX_BASE/$ABI"
  if [ -f "$PREFIX/lib/libssl.a" ] && [ -f "$PREFIX/lib/libcrypto.a" ]; then
    echo "[skip] $ABI 已编译: $PREFIX"
    return 0
  fi

  echo "=== 编译 OpenSSL for Android $ABI (target=$OPENSSL_TARGET, API=$API) ==="
  local BUILD_DIR="/tmp/openssl-android-$ABI"
  rm -rf "$BUILD_DIR"
  cp -R "$OPENSSL_SRC" "$BUILD_DIR"
  cd "$BUILD_DIR"

  CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./Configure "$OPENSSL_TARGET" \
    -D__ANDROID_API__="$API" \
    --prefix="$PREFIX" \
    --openssldir="$PREFIX/ssl" \
    no-shared no-tests no-apps no-docs $EXTRA_OPTS

  CFLAGS="-fPIC" CXXFLAGS="-fPIC" make -j"$(sysctl -n hw.ncpu)" build_libs
  make install_dev

  echo "[done] $ABI -> $PREFIX"
}

for abi in $ABIS_INPUT; do
  build_one "$abi"
done

echo "=== 全部完成 ==="
ls -lh "$PREFIX_BASE"/*/lib/libssl.a "$PREFIX_BASE"/*/lib/libcrypto.a 2>/dev/null
