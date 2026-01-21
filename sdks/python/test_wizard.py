#!/usr/bin/env python3
"""
自动化测试脚本 - 测试Python validation wizard的核心功能
"""
import sys
import os

# 导入decenlicense模块
from decenlicense import DecentriLicenseClient, LicenseError

def test_basic_functionality():
    """测试基本SDK功能"""
    print("=" * 60)
    print("测试1: 创建客户端")
    print("=" * 60)

    try:
        client = DecentriLicenseClient()
        print("✅ 客户端创建成功")
    except Exception as e:
        print(f"❌ 客户端创建失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试2: 初始化配置")
    print("=" * 60)

    try:
        client.initialize(
            license_code="RSA-2026-020-8WFMPF",
            udp_port=13325,
            tcp_port=23325
        )
        print("✅ 配置初始化成功")
    except Exception as e:
        print(f"❌ 配置初始化失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试3: 设置产品公钥")
    print("=" * 60)

    try:
        # 查找产品公钥文件
        import glob
        pem_files = glob.glob("public_*.pem") + glob.glob("product_public*.pem")
        if not pem_files:
            print("⚠️  未找到产品公钥文件")
            return False

        with open(pem_files[0], 'r') as f:
            public_key = f.read()

        client.set_product_public_key(public_key)
        print(f"✅ 产品公钥已设置: {pem_files[0]}")
    except Exception as e:
        print(f"❌ 设置产品公钥失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试4: 导入加密Token")
    print("=" * 60)

    try:
        # 查找加密token文件
        import glob
        token_files = glob.glob("token_*encrypted.txt")
        if not token_files:
            print("⚠️  未找到加密token文件")
            return False

        with open(token_files[0], 'r') as f:
            token_content = f.read().strip()

        client.import_token(token_content)
        print(f"✅ Token已导入: {token_files[0]}")
    except Exception as e:
        print(f"❌ 导入Token失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试5: 激活Token (首次激活)")
    print("=" * 60)

    try:
        client.activate_bind_device()
        print("✅ Token激活成功")
    except Exception as e:
        print(f"❌ Token激活失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试6: 检查激活状态")
    print("=" * 60)

    try:
        is_activated = client.is_activated()
        if is_activated:
            print("✅ 许可证已激活")
        else:
            print("❌ 许可证未激活")
            return False
    except Exception as e:
        print(f"❌ 检查激活状态失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试7: 离线验证Token")
    print("=" * 60)

    try:
        result = client.offline_verify_current_token()
        if result:
            print("✅ Token验证成功")
        else:
            print("❌ Token验证失败")
            return False
    except Exception as e:
        print(f"❌ Token验证失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试8: 获取设备ID")
    print("=" * 60)

    try:
        device_id = client.get_device_id()
        print(f"✅ 设备ID: {device_id}")
    except Exception as e:
        print(f"❌ 获取设备ID失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试9: 获取当前Token信息")
    print("=" * 60)

    try:
        token_info = client.get_current_token()
        if token_info:
            print(f"✅ Token ID: {token_info['token_id']}")
            print(f"✅ Holder Device ID: {token_info['holder_device_id']}")
        else:
            print("⚠️  无Token信息")
            return False
    except Exception as e:
        print(f"❌ 获取Token信息失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试10: 导出激活后的Token")
    print("=" * 60)

    try:
        activated_token = client.export_activated_token_encrypted()
        if activated_token:
            # 保存激活后的token
            filename = f"token_activated_test_RSA-2026-020-8WFMPF.txt"
            with open(filename, 'w') as f:
                f.write(activated_token)
            print(f"✅ 激活Token已导出: {filename}")
            print(f"✅ Token长度: {len(activated_token)} 字符")
        else:
            print("❌ Token导出失败")
            return False
    except Exception as e:
        print(f"❌ 导出Token失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试11: 记账功能")
    print("=" * 60)

    try:
        import json
        import time
        test_payload = json.dumps({
            "action": "test_usage",
            "timestamp": int(time.time()),
            "test_data": "Python SDK validation"
        })

        result = client.record_usage(test_payload)
        if result.get('valid'):
            print("✅ 记账成功")
        else:
            print(f"❌ 记账失败: {result.get('error_message', 'Unknown error')}")
            return False
    except Exception as e:
        print(f"❌ 记账失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试12: 导出记账后的Token")
    print("=" * 60)

    try:
        state_token = client.export_state_changed_token_encrypted()
        if state_token:
            # 保存状态token
            import time
            timestamp = time.strftime("%Y%m%d%H%M%S")
            filename = f"token_state_RSA-2026-020-8WFMPF_test_{timestamp}.txt"
            with open(filename, 'w') as f:
                f.write(state_token)
            print(f"✅ 状态Token已导出: {filename}")
            print(f"✅ Token长度: {len(state_token)} 字符")
        else:
            print("❌ Token导出失败")
            return False
    except Exception as e:
        print(f"❌ 导出Token失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试13: 获取设备状态")
    print("=" * 60)

    try:
        state = client.get_device_state()
        print(f"✅ 设备状态: {state}")
    except Exception as e:
        print(f"❌ 获取设备状态失败: {e}")
        return False

    print("\n" + "=" * 60)
    print("测试14: 获取完整状态信息")
    print("=" * 60)

    try:
        status = client.get_status()
        print(f"✅ 状态信息获取成功")
        print(f"   激活状态: {status.get('is_activated', 'N/A')}")
        print(f"   设备状态: {status.get('device_state', 'N/A')}")
    except Exception as e:
        print(f"❌ 获取状态信息失败: {e}")
        return False

    # 清理
    try:
        client.shutdown()
        print("\n✅ 客户端已关闭")
    except Exception as e:
        print(f"\n⚠️  客户端关闭时出现警告: {e}")

    return True

if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("DecentriLicense Python SDK 自动化测试")
    print("=" * 60 + "\n")

    try:
        success = test_basic_functionality()

        print("\n" + "=" * 60)
        if success:
            print("🎉 所有测试通过！Python SDK 工作正常")
            sys.exit(0)
        else:
            print("❌ 测试失败")
            sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试过程中发生异常: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
