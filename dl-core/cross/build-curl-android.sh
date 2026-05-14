#!/usr/bin/env bash
# Android NDK 交叉编译 libcurl 静态库（依赖已编好的 OpenSSL）
# 输出: $PREFIX_BASE/<ABI>/{include/curl,lib/libcurl.a}
set -euo pipefail

CURL_SRC="${CURL_SRC:-$HOME/dl-thirdparty/curl}"
ANDROID_NDK="${ANDROID_NDK:-$HOME/Library/Android/sdk/ndk/30.0.14904198}"
API="${ANDROID_API:-24}"
OPENSSL_PREFIX_BASE="${OPENSSL_PREFIX_BASE:-$HOME/dl-thirdparty/openssl-android}"
PREFIX_BASE="${PREFIX_BASE:-$HOME/dl-thirdparty/curl-android}"
ABIS_INPUT="${1:-arm64-v8a}"

[ -d "$CURL_SRC" ] || { echo "缺少 curl 源码: $CURL_SRC"; exit 1; }
[ -d "$ANDROID_NDK" ] || { echo "缺少 NDK"; exit 1; }

build_one() {
  local ABI="$1"
  local OPENSSL_PREFIX="$OPENSSL_PREFIX_BASE/$ABI"
  local PREFIX="$PREFIX_BASE/$ABI"
  if [ -f "$PREFIX/lib/libcurl.a" ]; then
    echo "[skip] $ABI 已编译: $PREFIX"
    return 0
  fi
  [ -f "$OPENSSL_PREFIX/lib/libssl.a" ] || { echo "$ABI OpenSSL 未编译"; return 1; }

  echo "=== 编译 libcurl for Android $ABI ==="
  local BUILD_DIR="/tmp/curl-android-$ABI"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  cmake "$CURL_SRC" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON \
    -DCURL_USE_OPENSSL=ON \
    -DOPENSSL_ROOT_DIR="$OPENSSL_PREFIX" \
    -DOPENSSL_INCLUDE_DIR="$OPENSSL_PREFIX/include" \
    -DOPENSSL_SSL_LIBRARY="$OPENSSL_PREFIX/lib/libssl.a" \
    -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_PREFIX/lib/libcrypto.a" \
    -DCURL_USE_LIBPSL=OFF \
    -DCURL_USE_LIBSSH2=OFF \
    -DCURL_DISABLE_LDAP=ON \
    -DCURL_DISABLE_LDAPS=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_CURL_EXE=OFF \
    -DENABLE_THREADED_RESOLVER=ON

  cmake --build . -j"$(sysctl -n hw.ncpu)"
  cmake --install .
  echo "[done] $ABI -> $PREFIX"
}

for abi in $ABIS_INPUT; do
  build_one "$abi"
done

echo "=== 全部完成 ==="
ls -lh "$PREFIX_BASE"/*/lib/libcurl.a 2>/dev/null
