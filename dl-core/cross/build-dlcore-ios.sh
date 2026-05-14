#!/usr/bin/env bash
# Xcode 工具链交叉编译 dl-core 静态库 for iOS（iOS 禁动态库，输出 .a）
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJ="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPLOY_TARGET="${IOS_DEPLOYMENT_TARGET:-12.0}"
OPENSSL_PREFIX_BASE="${OPENSSL_PREFIX_BASE:-$HOME/dl-thirdparty/openssl-ios}"
CURL_PREFIX_BASE="${CURL_PREFIX_BASE:-$HOME/dl-thirdparty/curl-ios}"
TARGETS_INPUT="${1:-ios-arm64 iossimulator-arm64 iossimulator-x86_64}"

build_one() {
  local TARGET="$1"
  local SDK ARCH MIN_FLAG
  case "$TARGET" in
    ios-arm64)           SDK="iphoneos";        ARCH="arm64";  MIN_FLAG="-mios-version-min=$DEPLOY_TARGET" ;;
    iossimulator-arm64)  SDK="iphonesimulator"; ARCH="arm64";  MIN_FLAG="-mios-simulator-version-min=$DEPLOY_TARGET" ;;
    iossimulator-x86_64) SDK="iphonesimulator"; ARCH="x86_64"; MIN_FLAG="-mios-simulator-version-min=$DEPLOY_TARGET" ;;
    *) echo "未知 target"; return 1 ;;
  esac

  local SDK_PATH
  SDK_PATH=$(xcrun --sdk "$SDK" --show-sdk-path)
  local OPENSSL_PREFIX="$OPENSSL_PREFIX_BASE/$TARGET"
  local CURL_PREFIX="$CURL_PREFIX_BASE/$TARGET"
  local OUT_DIR="$PROJ/build-all/ios/$TARGET"
  local BUILD_DIR="$PROJ/build-ios-$TARGET"

  echo "=== 编译 dl-core for iOS $TARGET ==="
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR" "$OUT_DIR"

  # iOS 禁止动态库：编为静态库
  cmake -S "$PROJ" -B "$BUILD_DIR" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
    -DCMAKE_C_FLAGS="$MIN_FLAG -fPIC" \
    -DCMAKE_CXX_FLAGS="$MIN_FLAG -fPIC" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DOPENSSL_ROOT_DIR="$OPENSSL_PREFIX" \
    -DOPENSSL_INCLUDE_DIR="$OPENSSL_PREFIX/include" \
    -DOPENSSL_SSL_LIBRARY="$OPENSSL_PREFIX/lib/libssl.a" \
    -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_PREFIX/lib/libcrypto.a" \
    -DCURL_LIBRARY="$CURL_PREFIX/lib/libcurl.a" \
    -DCURL_INCLUDE_DIR="$CURL_PREFIX/include"

  cmake --build "$BUILD_DIR" -j"$(sysctl -n hw.ncpu)"

  # 找到产物（静态库可能名为 libdecentrilicense.a 或 .dylib）
  local PRODUCT
  if [ -f "$BUILD_DIR/libdecentrilicense.a" ]; then
    PRODUCT="$BUILD_DIR/libdecentrilicense.a"
  elif [ -f "$BUILD_DIR/libdecentrilicense.dylib" ]; then
    PRODUCT="$BUILD_DIR/libdecentrilicense.dylib"
  else
    echo "未找到 dl-core 产物"; return 1
  fi
  cp -f "$PRODUCT" "$OUT_DIR/"
  echo "[done] $TARGET -> $OUT_DIR/$(basename "$PRODUCT")"
}

for t in $TARGETS_INPUT; do
  build_one "$t"
done

echo "=== 全部完成 ==="
find "$PROJ/build-all/ios" -name "libdecentrilicense*" 2>/dev/null
