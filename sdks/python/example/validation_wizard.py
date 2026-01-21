#!/usr/bin/env python3
"""
DecentriLicense Python SDK 验证向导
================================

这是一个交互式的验证工具，用于测试DecentriLicense Python SDK的所有功能。
"""

import json
import os
import sys
import time
from datetime import datetime
from decenlicense import DecentriLicenseClient, LicenseError


g_client = None
g_initialized = False


class AccountingInfo:
    """记账信息结构"""
    def __init__(self, token_id, device_id, license_code):
        self.token_id = token_id
        self.activation_time = datetime.now().isoformat()
        self.device_id = device_id
        self.license_code = license_code
        self.is_valid = True


class ValidationResult:
    """验证结果结构"""
    def __init__(self, valid=True, token_id=None, error=None, app_id=None, 
                 license_code=None, issue_time=None, expire_time=None, holder_device_id=None):
        self.valid = valid
        self.token_id = token_id
        self.error = error
        self.app_id = app_id
        self.license_code = license_code
        self.issue_time = issue_time
        self.expire_time = expire_time
        self.holder_device_id = holder_device_id


def list_files_for_selection(exts=None):
    files = []
    for name in os.listdir('.'):
        if not os.path.isfile(name):
            continue
        if exts:
            lower = name.lower()
            if not any(lower.endswith(ext) for ext in exts):
                continue
        files.append(name)
    return sorted(files)


def pick_file_from_cwd(prompt_title, exts=None):
    files = list_files_for_selection(exts)
    print(prompt_title)
    if not files:
        return input("当前目录没有可选文件，请手动输入路径: ").strip()

    for i, f in enumerate(files, 1):
        print(f"{i}. {f}")
    print("0. 手动输入路径")

    sel = input("请选择文件编号: ").strip()
    try:
        n = int(sel)
    except ValueError:
        n = -1

    if 1 <= n <= len(files):
        return files[n - 1]
    return input("请输入文件路径: ").strip()


def main():
    """主函数"""
    print("==========================================")
    print("DecentriLicense Python SDK 验证向导")
    print("==========================================")
    print()

    while True:
        print("请选择要执行的操作:")
        print("1. 导入 token(密文/JSON)")
        print("2. 离线验证")
        print("3. 激活绑定本机")
        print("4. 查询状态")
        print("5. 记录使用量/状态迁移")
        print("0. 退出")
        choice = input("请输入选项 (0-5): ").strip()

        if choice == "1":
            activate_token_wizard()
        elif choice == "2":
            verify_token_wizard()
        elif choice == "3":
            accounting_wizard()
        elif choice == "4":
            trust_chain_validation_wizard()
        elif choice == "5":
            comprehensive_validation_wizard()
        elif choice == "0":
            shutdown_client()
            print("感谢使用 DecentriLicense Python SDK 验证向导!")
            break
        else:
            print("无效选项，请重新输入。")

        print("\n" + "-" * 50 + "\n")


def activate_token_wizard():
    """导入 token(密文/JSON)"""
    global g_client, g_initialized
    print("\n--- 1) 导入 token(密文/JSON) ---")

    product_key_file = pick_file_from_cwd("请选择产品公钥文件(含 ROOT_SIGNATURE):", exts=['.pem'])
    if not product_key_file:
        print("产品公钥文件路径不能为空")
        return

    token_file_path = pick_file_from_cwd("请选择 token 文件:", exts=['.txt', '.json'])
    if not token_file_path:
        print("token 文件路径不能为空")
        return

    try:
        with open(product_key_file, 'r', encoding='utf-8') as f:
            product_public_key = f.read()
        with open(token_file_path, 'r', encoding='utf-8') as f:
            token_input = f.read()

        shutdown_client()
        g_client = DecentriLicenseClient()
        g_client.initialize("", udp_port=0, tcp_port=0, registry_server_url="")
        g_client.set_product_public_key(product_public_key)
        g_client.import_token(token_input)
        g_initialized = True

        print("导入成功。产品公钥预览: " + product_public_key[:120] + '...')
        print("token 预览: " + token_input[:120] + '...')
    except Exception as e:
        print(f"导入失败: {e}")
        shutdown_client()


def verify_token_wizard():
    """离线验证"""
    print("\n--- 2) 离线验证 ---")

    if not ensure_initialized():
        return

    try:
        result = g_client.offline_verify_current_token()
        print("离线验证结果: %s" % ("✓ 成功" if result.get('valid') else "✗ 失败"))
        if not result.get('valid'):
            print("错误信息: %s" % result.get('error_message'))
    except Exception as e:
        print(f"离线验证失败: {e}")


def accounting_wizard():
    """激活绑定本机"""
    print("\n--- 3) 激活绑定本机 ---")

    if not ensure_initialized():
        return

    try:
        result = g_client.activate_bind_device()
        print("激活绑定结果: %s" % ("✓ 成功" if result.get('valid') else "✗ 失败"))
        if not result.get('valid'):
            print("错误信息: %s" % result.get('error_message'))
    except Exception as e:
        print(f"激活绑定失败: {e}")


def trust_chain_validation_wizard():
    """查询状态"""
    print("\n--- 4) 查询状态 ---")

    if not ensure_initialized():
        return

    try:
        status = g_client.get_status()
        print("has_token: %s" % int(bool(status.get('has_token'))))
        print("is_activated: %s" % int(bool(status.get('is_activated'))))
        print("state_index: %s" % status.get('state_index'))
        print("token_id: %s" % status.get('token_id'))
        print("holder_device_id: %s" % status.get('holder_device_id'))
        print("app_id: %s" % status.get('app_id'))
        print("license_code: %s" % status.get('license_code'))
    except Exception as e:
        print(f"查询状态失败: {e}")


def comprehensive_validation_wizard():
    """记录使用量/状态迁移"""
    print("\n--- 5) 记录使用量/状态迁移 ---")

    if not ensure_initialized():
        return

    payload_path_or_input = pick_file_from_cwd("请选择 state payload JSON 文件(或 0 手动输入 JSON):", exts=['.json'])
    if not payload_path_or_input:
        print("payload 不能为空")
        return

    payload = payload_path_or_input
    if os.path.isfile(payload_path_or_input):
        try:
            with open(payload_path_or_input, 'r', encoding='utf-8') as f:
                payload = f.read()
        except Exception as e:
            print(f"读取 payload 文件失败: {e}")
            return

    try:
        result = g_client.record_usage(payload)
        print("记录使用量结果: %s" % ("✓ 成功" if result.get('valid') else "✗ 失败"))
        if not result.get('valid'):
            print("错误信息: %s" % result.get('error_message'))
    except Exception as e:
        print(f"记录使用量失败: {e}")


def ensure_initialized():
    if not g_initialized or g_client is None:
        print("请先执行 1) 导入 token(密文/JSON)")
        return False
    return True


def shutdown_client():
    global g_client, g_initialized
    if g_client is not None:
        try:
            g_client.shutdown()
        except Exception:
            pass
    g_client = None
    g_initialized = False


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n程序发生未预期的错误: {e}")
        sys.exit(1)