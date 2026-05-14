#!/usr/bin/env bash
# Xcode 工具链交叉编译 libcurl 静态库 for iOS
set -euo pipefail

CURL_SRC="${CURL_SRC:-$HOME/dl-thirdparty/curl}"
OPENSSL_PREFIX_BASE="${OPENSSL_PREFIX_BASE:-$HOME/dl-thirdparty/openssl-ios}"
PREFIX_BASE="${PREFIX_BASE:-$HOME/dl-thirdparty/curl-ios}"
DEPLOY_TARGET="${IOS_DEPLOYMENT_TARGET:-12.0}"
TARGETS_INPUT="${1:-ios-arm64 iossimulator-arm64 iossimulator-x86_64}"

build_one() {
  local TARGET="$1"
  local SDK ARCH SYSROOT_FLAG IOS_PLATFORM
  case "$TARGET" in
    ios-arm64)              SDK="iphoneos";        ARCH="arm64";  IOS_PLATFORM="OS" ;;
    iossimulator-arm64)     SDK="iphonesimulator"; ARCH="arm64";  IOS_PLATFORM="SIMULATOR" ;;
    iossimulator-x86_64)    SDK="iphonesimulator"; ARCH="x86_64"; IOS_PLATFORM="SIMULATOR" ;;
    *) echo "未知 target"; return 1 ;;
  esac

  local OPENSSL_PREFIX="$OPENSSL_PREFIX_BASE/$TARGET"
  local PREFIX="$PREFIX_BASE/$TARGET"
  if [ -f "$PREFIX/lib/libcurl.a" ]; then
    echo "[skip] $TARGET 已编译"
    return 0
  fi
  [ -f "$OPENSSL_PREFIX/lib/libssl.a" ] || { echo "$TARGET OpenSSL 未编"; return 1; }

  echo "=== 编译 libcurl for iOS $TARGET ==="
  local SDK_PATH
  SDK_PATH=$(xcrun --sdk "$SDK" --show-sdk-path)
  local MIN_FLAG="-mios-version-min=$DEPLOY_TARGET"
  [ "$IOS_PLATFORM" = "SIMULATOR" ] && MIN_FLAG="-mios-simulator-version-min=$DEPLOY_TARGET"

  local BUILD_DIR="/tmp/curl-ios-$TARGET"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  cmake "$CURL_SRC" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
    -DCMAKE_C_FLAGS="$MIN_FLAG -fPIC" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
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
    -DBUILD_CURL_EXE=OFF

  cmake --build . -j"$(sysctl -n hw.ncpu)"
  cmake --install .
  echo "[done] $TARGET -> $PREFIX"
}

for t in $TARGETS_INPUT; do
  build_one "$t"
done

echo "=== 全部完成 ==="
ls -lh "$PREFIX_BASE"/*/lib/libcurl.a 2>/dev/null
