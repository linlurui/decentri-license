#!/usr/bin/env bash
# Android NDK 交叉编译 dl-core 共享库（链接已编好的 OpenSSL+libcurl 静态库）
# 输出: $PROJ/build-all/android/<ABI>/libdecentrilicense.so
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJ="$(cd "$SCRIPT_DIR/.." && pwd)"

ANDROID_NDK="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk/30.0.14904198}"
API="${ANDROID_API:-24}"
OPENSSL_PREFIX_BASE="${OPENSSL_PREFIX_BASE:-$HOME/dl-thirdparty/openssl-android}"
CURL_PREFIX_BASE="${CURL_PREFIX_BASE:-$HOME/dl-thirdparty/curl-android}"
ABIS_INPUT="${1:-arm64-v8a armeabi-v7a x86 x86_64}"

build_one() {
  local ABI="$1"
  local OPENSSL_PREFIX="$OPENSSL_PREFIX_BASE/$ABI"
  local CURL_PREFIX="$CURL_PREFIX_BASE/$ABI"
  local OUT_DIR="$PROJ/build-all/android/$ABI"
  local BUILD_DIR="$PROJ/build-android-$ABI"

  echo "=== 编译 dl-core for Android $ABI ==="
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR" "$OUT_DIR"

  cmake -S "$PROJ" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR="$OPENSSL_PREFIX" \
    -DOPENSSL_INCLUDE_DIR="$OPENSSL_PREFIX/include" \
    -DOPENSSL_SSL_LIBRARY="$OPENSSL_PREFIX/lib/libssl.a" \
    -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_PREFIX/lib/libcrypto.a" \
    -DCURL_LIBRARY="$CURL_PREFIX/lib/libcurl.a" \
    -DCURL_INCLUDE_DIR="$CURL_PREFIX/include"

  cmake --build "$BUILD_DIR" -j"$(sysctl -n hw.ncpu)"

  cp -f "$BUILD_DIR/libdecentrilicense.so" "$OUT_DIR/"
  echo "[done] $ABI -> $OUT_DIR/libdecentrilicense.so"
}

for abi in $ABIS_INPUT; do
  build_one "$abi"
done

echo "=== 全部完成 ==="
ls -lh "$PROJ/build-all/android/"*/libdecentrilicense.so 2>/dev/null
