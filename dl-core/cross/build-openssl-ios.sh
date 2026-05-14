#!/usr/bin/env bash
# 用 Xcode 工具链交叉编译 OpenSSL 静态库 for iOS
# 输出: $PREFIX_BASE/<TARGET>/{include,lib/libssl.a,libcrypto.a}
# TARGET ∈ { ios-arm64, iossimulator-arm64, iossimulator-x86_64 }
set -euo pipefail

OPENSSL_SRC="${OPENSSL_SRC:-$HOME/dl-thirdparty/openssl}"
PREFIX_BASE="${PREFIX_BASE:-$HOME/dl-thirdparty/openssl-ios}"
DEPLOY_TARGET="${IOS_DEPLOYMENT_TARGET:-12.0}"
TARGETS_INPUT="${1:-ios-arm64 iossimulator-arm64 iossimulator-x86_64}"

[ -d "$OPENSSL_SRC" ] || { echo "缺少 OpenSSL 源码: $OPENSSL_SRC"; exit 1; }

build_one() {
  local TARGET="$1"
  local OPENSSL_TARGET SDK ARCH MIN_FLAG
  case "$TARGET" in
    ios-arm64)              OPENSSL_TARGET="ios64-xcrun";        SDK="iphoneos";       ARCH="arm64";  MIN_FLAG="-mios-version-min=$DEPLOY_TARGET" ;;
    iossimulator-arm64)     OPENSSL_TARGET="iossimulator-xcrun"; SDK="iphonesimulator"; ARCH="arm64";  MIN_FLAG="-mios-simulator-version-min=$DEPLOY_TARGET" ;;
    iossimulator-x86_64)    OPENSSL_TARGET="iossimulator-xcrun"; SDK="iphonesimulator"; ARCH="x86_64"; MIN_FLAG="-mios-simulator-version-min=$DEPLOY_TARGET" ;;
    *) echo "未知 target: $TARGET"; return 1 ;;
  esac

  local PREFIX="$PREFIX_BASE/$TARGET"
  if [ -f "$PREFIX/lib/libssl.a" ] && [ -f "$PREFIX/lib/libcrypto.a" ]; then
    echo "[skip] $TARGET 已编译: $PREFIX"
    return 0
  fi

  echo "=== 编译 OpenSSL for iOS $TARGET ==="
  local BUILD_DIR="/tmp/openssl-ios-$TARGET"
  rm -rf "$BUILD_DIR"
  cp -R "$OPENSSL_SRC" "$BUILD_DIR"
  cd "$BUILD_DIR"

  local SDK_PATH
  SDK_PATH=$(xcrun --sdk "$SDK" --show-sdk-path)
  export CC="$(xcrun -find -sdk "$SDK" clang) -arch $ARCH -isysroot $SDK_PATH $MIN_FLAG"
  export CXX="$(xcrun -find -sdk "$SDK" clang++) -arch $ARCH -isysroot $SDK_PATH $MIN_FLAG"
  export CFLAGS="-fPIC -arch $ARCH -isysroot $SDK_PATH $MIN_FLAG"
  export CROSS_TOP="$(xcode-select -p)/Platforms/$(echo $SDK | sed 's/iphoneos/iPhoneOS/;s/iphonesimulator/iPhoneSimulator/').platform/Developer"
  export CROSS_SDK="$(echo $SDK | sed 's/iphoneos/iPhoneOS/;s/iphonesimulator/iPhoneSimulator/').sdk"

  ./Configure "$OPENSSL_TARGET" \
    --prefix="$PREFIX" \
    --openssldir="$PREFIX/ssl" \
    no-shared no-tests no-apps no-docs no-asm

  make -j"$(sysctl -n hw.ncpu)" build_libs
  make install_dev

  unset CC CXX CFLAGS CROSS_TOP CROSS_SDK
  echo "[done] $TARGET -> $PREFIX"
}

for t in $TARGETS_INPUT; do
  build_one "$t"
done

echo "=== 全部完成 ==="
ls -lh "$PREFIX_BASE"/*/lib/libssl.a 2>/dev/null
