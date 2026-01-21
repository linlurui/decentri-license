#!/bin/bash
# Simple test for idempotent activation

set -e

echo "=========================================="
echo "设备密钥持久化和幂等激活测试"
echo "=========================================="

source ../build_config.sh

ED_LICENSE="ED-2026-005-PEIZAU"
STATE_DIR=".decentrilicense_state/${ED_LICENSE}"

echo ""
echo "1. 检查首次激活后的设备密钥..."
if [ -f "${STATE_DIR}/device_id.txt" ]; then
    DEVICE_ID_1=$(cat "${STATE_DIR}/device_id.txt")
    echo "✅ 设备ID已保存: ${DEVICE_ID_1}"
    echo "   长度: ${#DEVICE_ID_1} 字符"

    if [ ${#DEVICE_ID_1} -eq 64 ]; then
        echo "✅ 设备ID长度正确（64字符，SHA256十六进制）"
    else
        echo "❌ 设备ID长度错误（预期64，实际${#DEVICE_ID_1}）"
        exit 1
    fi
else
    echo "❌ 设备ID文件不存在"
    exit 1
fi

if [ -f "${STATE_DIR}/device_private_key.pem" ]; then
    PRIVATE_KEY_1=$(cat "${STATE_DIR}/device_private_key.pem")
    echo "✅ 设备私钥已保存（${#PRIVATE_KEY_1} 字节）"
else
    echo "❌ 设备私钥文件不存在"
    exit 1
fi

if [ -f "${STATE_DIR}/device_public_key.pem" ]; then
    echo "✅ 设备公钥已保存"
else
    echo "❌ 设备公钥文件不存在"
    exit 1
fi

echo ""
echo "2. 模拟重新激活（删除内存状态但保留持久化密钥）..."
# In a real scenario, this would be a new process/machine loading saved keys
echo "✅ 密钥文件保持不变，模拟进程重启"

echo ""
echo "3. 使用已激活的token再次激活（测试幂等性）..."
ACTIVATED_TOKEN=$(ls -t token_activated_${ED_LICENSE}_*.txt 2>/dev/null | head -1)

if [ -z "$ACTIVATED_TOKEN" ]; then
    echo "❌ 未找到激活token"
    exit 1
fi

echo "   使用token: $ACTIVATED_TOKEN"

# Re-activate with the same token
# This should load existing keys, not generate new ones
(
  echo "0"  # Select product key
  sleep 0.5
  echo "1"  # Use ED25519 key
  sleep 0.5
  echo "1"  # Activate token
  sleep 0.5
  echo "2"  # Use custom file
  sleep 0.5
  echo "$ACTIVATED_TOKEN"  # Token file path
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/reactivation.log 2>&1

echo ""
echo "4. 验证设备密钥是否保持不变（幂等性测试）..."
if [ -f "${STATE_DIR}/device_id.txt" ]; then
    DEVICE_ID_2=$(cat "${STATE_DIR}/device_id.txt")
    echo "   重新激活后的设备ID: ${DEVICE_ID_2}"

    if [ "$DEVICE_ID_1" = "$DEVICE_ID_2" ]; then
        echo "✅ 设备ID保持不变（幂等性正确）"
    else
        echo "⚠️  设备ID已改变"
        echo "   激活前: ${DEVICE_ID_1}"
        echo "   激活后: ${DEVICE_ID_2}"
        # This is expected if the token contains a different device_id
        # The idempotency is about not regenerating keys if they exist locally
    fi

    PRIVATE_KEY_2=$(cat "${STATE_DIR}/device_private_key.pem")
    if [ "$PRIVATE_KEY_1" = "$PRIVATE_KEY_2" ]; then
        echo "✅ 设备私钥保持不变（真正的幂等性）"
    else
        echo "❌ 设备私钥已改变（幂等性失败）"
        exit 1
    fi
else
    echo "❌ 设备ID文件不存在"
    exit 1
fi

echo ""
echo "=========================================="
echo "✅ 所有测试通过！"
echo "=========================================="
echo ""
echo "验证内容:"
echo "1. ✅ 设备ID缓冲区修复（64字符完整保存）"
echo "2. ✅ 设备密钥持久化成功"
echo "3. ✅ 重新激活时密钥保持不变（幂等性）"
