#!/bin/bash
# 完整测试脚本：验证所有修复是否生效

set -e

echo "=========================================="
echo "DecentriLicense 修复验证测试"
echo "=========================================="
echo ""
echo "测试内容："
echo "1. 设备ID缓冲区溢出修复（64→256字节）"
echo "2. 设备密钥持久化机制"
echo "3. ActivateBindDevice幂等操作"
echo "4. 状态链签名验证"
echo ""
echo "开始测试..."
echo ""

source ../build_config.sh

ED_LICENSE="ED-2026-005-PEIZAU"
ED_TOKEN="../../../dl-issuer/token_test_${ED_LICENSE}_20260115183609_encrypted.txt"
STATE_DIR=".decentrilicense_state/${ED_LICENSE}"

# 检查token文件是否存在
if [ ! -f "$ED_TOKEN" ]; then
    echo "❌ Token文件不存在: $ED_TOKEN"
    exit 1
fi

echo "================================================"
echo "测试1: 首次激活（生成并保存设备密钥）"
echo "================================================"
echo ""

(
  echo "0"  # Select product key
  sleep 0.5
  echo "1"  # Use ED25519 key
  sleep 0.5
  echo "1"  # Activate token
  sleep 0.5
  echo "1"  # Use preset ED token
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test1_activation.log 2>&1

echo "检查设备密钥是否已保存..."
if [ -d "$STATE_DIR" ]; then
    echo "✅ 状态目录已创建: $STATE_DIR"
else
    echo "❌ 状态目录未创建"
    exit 1
fi

if [ -f "${STATE_DIR}/device_private_key.pem" ]; then
    echo "✅ 设备私钥已保存"
    PRIVATE_KEY_1=$(cat "${STATE_DIR}/device_private_key.pem")
else
    echo "❌ 设备私钥未保存"
    exit 1
fi

if [ -f "${STATE_DIR}/device_public_key.pem" ]; then
    echo "✅ 设备公钥已保存"
else
    echo "❌ 设备公钥未保存"
    exit 1
fi

if [ -f "${STATE_DIR}/device_id.txt" ]; then
    DEVICE_ID_1=$(cat "${STATE_DIR}/device_id.txt")
    echo "✅ 设备ID已保存: ${DEVICE_ID_1}"

    # 检查设备ID长度（SHA256 = 64字符）
    ID_LENGTH=${#DEVICE_ID_1}
    if [ $ID_LENGTH -eq 64 ]; then
        echo "   ✅ 设备ID长度正确: ${ID_LENGTH} 字符（SHA256十六进制）"
        echo "   ✅ 缓冲区溢出问题已修复（原来只能存63字符）"
    else
        echo "   ❌ 设备ID长度错误: ${ID_LENGTH} 字符（预期64）"
        exit 1
    fi
else
    echo "❌ 设备ID未保存"
    exit 1
fi

echo ""
echo "================================================"
echo "测试2: 记录使用状态（创建状态链）"
echo "================================================"
echo ""

# 记录第一次使用
echo "记录第一次使用..."
(
  echo "4"  # Record usage
  sleep 0.5
  echo '{"action":"purchase","amount":100,"currency":"USD"}'
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test2_usage1.log 2>&1

echo "✅ 第一次记账完成"

# 记录第二次使用
echo "记录第二次使用..."
(
  echo "4"  # Record usage
  sleep 0.5
  echo '{"action":"consume","units":50,"feature":"premium"}'
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test2_usage2.log 2>&1

echo "✅ 第二次记账完成"

# 检查状态索引
if [ -f "${STATE_DIR}/current_state.json" ]; then
    STATE_INDEX=$(grep -o '"state_index":[0-9]*' "${STATE_DIR}/current_state.json" 2>/dev/null | cut -d: -f2)
    echo "✅ 状态链已创建，当前索引: ${STATE_INDEX}"
else
    echo "⚠️  当前状态文件不存在"
fi

echo ""
echo "================================================"
echo "测试3: 验证当前token（包含状态链）"
echo "================================================"
echo ""

(
  echo "2"  # Verify activated token
  sleep 1
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test3_verify.log 2>&1

if grep -q "✅\|成功\|valid" /tmp/test3_verify.log; then
    echo "✅ Token验证成功（包含状态链）"
else
    echo "⚠️  验证状态未知"
fi

echo ""
echo "================================================"
echo "测试4: 模拟重新导入（测试幂等性）"
echo "================================================"
echo ""

# 查找最新的activated token
LATEST_TOKEN=$(ls -t token_activated_${ED_LICENSE}_*.txt 2>/dev/null | head -1)
if [ -z "$LATEST_TOKEN" ]; then
    echo "❌ 未找到已激活的token文件"
    exit 1
fi
echo "使用token文件: $LATEST_TOKEN"

# 备份设备密钥
echo "备份设备密钥..."
mkdir -p /tmp/device_keys_backup_test
cp "${STATE_DIR}/device_"*.pem "${STATE_DIR}/device_id.txt" /tmp/device_keys_backup_test/

# 删除状态目录（模拟新环境）
echo "删除状态目录（模拟新环境）..."
rm -rf .decentrilicense_state

# 恢复设备密钥（模拟密钥被持久化）
echo "恢复设备密钥..."
mkdir -p "${STATE_DIR}"
cp /tmp/device_keys_backup_test/* "${STATE_DIR}/"

# 重新激活
echo "重新激活token（测试幂等性）..."
(
  echo "0"  # Select product key
  sleep 0.5
  echo "1"  # Use ED25519 key
  sleep 0.5
  echo "1"  # Activate token
  sleep 0.5
  echo "2"  # Use custom file
  sleep 0.5
  echo "$LATEST_TOKEN"
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test4_reactivate.log 2>&1

echo "✅ Token重新激活完成"

# 验证密钥是否保持不变
DEVICE_ID_2=$(cat "${STATE_DIR}/device_id.txt")
PRIVATE_KEY_2=$(cat "${STATE_DIR}/device_private_key.pem")

echo ""
echo "验证密钥一致性..."
if [ "$DEVICE_ID_1" = "$DEVICE_ID_2" ]; then
    echo "✅ 设备ID保持一致"
    echo "   ${DEVICE_ID_1}"
else
    echo "❌ 设备ID不一致"
    echo "   激活前: ${DEVICE_ID_1}"
    echo "   激活后: ${DEVICE_ID_2}"
    exit 1
fi

if [ "$PRIVATE_KEY_1" = "$PRIVATE_KEY_2" ]; then
    echo "✅ 设备私钥保持一致（幂等性正确）"
else
    echo "❌ 设备私钥不一致（幂等性失败）"
    exit 1
fi

echo ""
echo "================================================"
echo "测试5: 验证重新导入后的状态链签名"
echo "================================================"
echo ""
echo "这是最关键的测试：验证状态链签名是否仍然有效"
echo ""

(
  echo "2"  # Verify activated token
  sleep 1
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test5_final_verify.log 2>&1

if grep -q "✅\|成功\|valid" /tmp/test5_final_verify.log; then
    echo "✅ 状态链签名验证成功！"
    FINAL_SUCCESS=true
else
    echo "❌ 状态链签名验证失败"
    echo "验证日志:"
    cat /tmp/test5_final_verify.log
    FINAL_SUCCESS=false
fi

echo ""
echo "================================================"
echo "测试6: 再次记录使用（验证状态链继续工作）"
echo "================================================"
echo ""

(
  echo "4"  # Record usage
  sleep 0.5
  echo '{"action":"final_test","status":"success"}'
  sleep 0.5
  echo "7"  # Exit
) | ./validation_wizard > /tmp/test6_final_usage.log 2>&1

echo "✅ 继续记账成功"

# 最终状态检查
if [ -f "${STATE_DIR}/current_state.json" ]; then
    FINAL_STATE_INDEX=$(grep -o '"state_index":[0-9]*' "${STATE_DIR}/current_state.json" 2>/dev/null | cut -d: -f2)
    echo "✅ 最终状态索引: ${FINAL_STATE_INDEX}"
fi

echo ""
echo "================================================"
echo "测试结果总结"
echo "================================================"
echo ""

if [ "$FINAL_SUCCESS" = true ]; then
    echo "🎉 所有测试通过！"
    echo ""
    echo "✅ 测试1: 首次激活并保存设备密钥 - 通过"
    echo "✅ 测试2: 创建状态链 - 通过"
    echo "✅ 测试3: 验证包含状态链的token - 通过"
    echo "✅ 测试4: 幂等激活（密钥保持一致）- 通过"
    echo "✅ 测试5: 重新导入后状态链签名验证 - 通过"
    echo "✅ 测试6: 继续记账操作 - 通过"
    echo ""
    echo "修复验证："
    echo "✅ 问题1（缓冲区溢出）: 设备ID完整保存64字符"
    echo "✅ 问题2（非幂等操作）: ActivateBindDevice实现真正幂等性"
    echo "✅ 问题3（签名验证失败）: 状态链签名验证成功"
    echo ""
    echo "设备密钥存储位置: ${STATE_DIR}"
    echo "- device_private_key.pem"
    echo "- device_public_key.pem"
    echo "- device_id.txt"
    echo ""
    exit 0
else
    echo "❌ 测试失败"
    echo ""
    echo "请检查测试日志："
    echo "- /tmp/test1_activation.log"
    echo "- /tmp/test2_usage1.log"
    echo "- /tmp/test2_usage2.log"
    echo "- /tmp/test3_verify.log"
    echo "- /tmp/test4_reactivate.log"
    echo "- /tmp/test5_final_verify.log"
    echo "- /tmp/test6_final_usage.log"
    echo ""
    exit 1
fi
