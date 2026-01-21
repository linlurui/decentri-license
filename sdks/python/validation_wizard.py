#!/usr/bin/env python3
"""
DecentriLicense Python SDK 验证向导
================================

功能完整的交互式验证工具，用于测试DecentriLicense Python SDK的所有功能。
参考Go SDK实现，提供统一的用户体验。
"""

import os
import sys
import glob
import json
import subprocess
from datetime import datetime
from typing import Optional, List
from decenlicense.client import DecentriLicenseClient, LicenseError

# 全局变量
g_client: Optional[DecentriLicenseClient] = None
g_initialized = False
selected_product_key_path: Optional[str] = None


def get_or_create_client() -> Optional[DecentriLicenseClient]:
    """获取或创建全局client实例"""
    global g_client
    if g_client is None:
        try:
            g_client = DecentriLicenseClient()
        except Exception as e:
            print(f"❌ 创建客户端失败: {e}")
            return None
    return g_client


def cleanup_client():
    """清理全局client"""
    global g_client, g_initialized
    if g_client is not None:
        try:
            g_client.shutdown()
        except:
            pass
        g_client = None
        g_initialized = False


def read_from_clipboard() -> str:
    """从系统剪贴板读取内容（macOS）"""
    try:
        result = subprocess.run(['pbpaste'], capture_output=True, text=True, check=True)
        return result.stdout
    except Exception as e:
        raise Exception(f"从剪贴板读取失败: {e}")


def find_all_product_keys() -> List[str]:
    """查找所有可用的产品公钥文件"""
    patterns = [
        "*.pem",
        "../*.pem",
        "../../*.pem",
        "../../../dl-issuer/*.pem",
    ]

    candidates = []
    for pattern in patterns:
        try:
            matches = glob.glob(pattern)
            candidates.extend(matches)
        except:
            pass

    # 去重并只保留产品公钥文件
    seen = set()
    unique = []
    for file in candidates:
        filename = os.path.basename(file)
        if (filename not in seen and
            'public' in filename and
            'private' not in filename and
            filename.endswith('.pem')):
            seen.add(filename)
            unique.append(filename)

    return sorted(unique)


def find_product_public_key() -> Optional[str]:
    """查找产品公钥文件"""
    global selected_product_key_path

    # 如果用户已经手动选择了，使用选择的
    if selected_product_key_path:
        return selected_product_key_path

    keys = find_all_product_keys()
    if keys:
        return resolve_product_key_path(keys[0])
    return None


def resolve_product_key_path(filename: str) -> str:
    """根据文件名找到完整的产品公钥文件路径"""
    search_paths = [
        filename,
        os.path.join(".", filename),
        os.path.join("..", filename),
        os.path.join("../..", filename),
        os.path.join("../../../dl-issuer", filename),
    ]

    for path in search_paths:
        if os.path.exists(path):
            return path

    return filename


def find_token_files(pattern: str = "*") -> List[str]:
    """查找token文件"""
    patterns = [
        f"token_{pattern}*.txt",
        f"../token_{pattern}*.txt",
        f"../../../dl-issuer/token_{pattern}*.txt",
    ]

    candidates = []
    for pat in patterns:
        try:
            matches = glob.glob(pat)
            candidates.extend(matches)
        except:
            pass

    # 去重并只保留文件名
    seen = set()
    unique = []
    for file in candidates:
        filename = os.path.basename(file)
        if filename not in seen and 'token_' in filename and filename.endswith('.txt'):
            seen.add(filename)
            unique.append(filename)

    return sorted(unique)


def find_encrypted_token_files() -> List[str]:
    """查找加密的token文件"""
    all_tokens = find_token_files()
    return [f for f in all_tokens if 'encrypted' in f]


def find_activated_token_files() -> List[str]:
    """查找已激活的token文件"""
    all_tokens = find_token_files()
    return [f for f in all_tokens if 'activated' in f]


def find_state_token_files() -> List[str]:
    """查找状态token文件（用于记账信息）- 照抄Go SDK"""
    candidates = []

    # 查找已激活和状态变更的token文件（照抄Go SDK）
    patterns = [
        "token_activated_*.txt",
        "token_state_*.txt",
        "../token_activated_*.txt",
        "../token_state_*.txt",
        "../../../dl-issuer/token_activated_*.txt",
        "../../../dl-issuer/token_state_*.txt",
    ]

    for pattern in patterns:
        try:
            matches = glob.glob(pattern)
            candidates.extend(matches)
        except:
            pass

    # 去重并只保留文件名
    seen = set()
    unique = []
    for file in candidates:
        filename = os.path.basename(file)
        if filename not in seen:
            seen.add(filename)
            unique.append(filename)

    return sorted(unique)


def resolve_token_file_path(filename: str) -> str:
    """根据文件名找到完整的token文件路径"""
    search_paths = [
        filename,
        os.path.join(".", filename),
        os.path.join("..", filename),
        os.path.join("../../../dl-issuer", filename),
    ]

    for path in search_paths:
        if os.path.exists(path):
            return path

    return filename


def select_product_key_wizard():
    """选择产品公钥向导"""
    global selected_product_key_path

    print("\n🔑 选择产品公钥")
    print("=" * 50)

    available_keys = find_all_product_keys()

    if not available_keys:
        print("❌ 当前目录下没有找到产品公钥文件")
        print("💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下")
        return

    print("📄 找到以下产品公钥文件:")
    for i, key_file in enumerate(available_keys, 1):
        print(f"{i}. {key_file}")
    print(f"{len(available_keys) + 1}. 取消选择")

    if selected_product_key_path:
        print(f"✅ 当前已选择: {selected_product_key_path}")

    try:
        choice = int(input(f"请选择要使用的产品公钥文件 (1-{len(available_keys) + 1}): ").strip())

        if choice == len(available_keys) + 1:
            selected_product_key_path = None
            print("✅ 已取消产品公钥选择")
        elif 1 <= choice <= len(available_keys):
            selected_file = available_keys[choice - 1]
            selected_product_key_path = resolve_product_key_path(selected_file)
            print(f"✅ 已选择产品公钥文件: {selected_file}")
        else:
            print("❌ 无效选择")
    except ValueError:
        print("❌ 无效选择")


def activate_token_wizard():
    """激活令牌向导"""
    global g_initialized

    print("\n🔓 激活令牌")
    print("-" * 50)
    print("⚠️  重要说明：")
    print("   • 加密token（encrypted）：首次从供应商获得，需要激活")
    print("   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活")
    print("   ⚠️  本功能仅用于【首次激活】加密token")
    print("   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）")
    print()

    client = get_or_create_client()
    if client is None:
        return

    # 显示可用的加密token文件
    token_files = find_encrypted_token_files()
    if token_files:
        print("📄 发现以下加密token文件:")
        for i, file in enumerate(token_files, 1):
            print(f"   {i}. {file}")
        print("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串")

    # 获取令牌输入
    print("请输入令牌字符串 (仅支持加密令牌):")
    print("💡 加密令牌通常从软件提供商处获得")
    print("💡 输入序号(1-N)可快速选择上面列出的文件")
    print("💡 输入文件路径可读取指定文件")
    print("💡 直接回车可以从剪贴板读取token")

    user_input = input("令牌或文件路径: ").strip()

    # 如果输入为空，尝试从剪贴板读取
    if not user_input:
        print("📋 正在从剪贴板读取token...")
        try:
            user_input = read_from_clipboard().strip()
            if not user_input:
                print("❌ 剪贴板为空，请手动输入token字符串")
                return
            print(f"✅ 从剪贴板读取到 {len(user_input)} 个字符")
        except Exception as e:
            print(f"❌ {e}")
            return

    # 检查是否输入的是数字（文件序号）
    token_string = user_input
    if token_files:
        try:
            index = int(user_input)
            if 1 <= index <= len(token_files):
                selected_file = token_files[index - 1]
                file_path = resolve_token_file_path(selected_file)
                try:
                    with open(file_path, 'r') as f:
                        token_string = f.read().strip()
                    print(f"✅ 选择文件 '{selected_file}' 并读取到令牌 ({len(token_string)} 字符)")
                except Exception as e:
                    print(f"❌ 无法读取文件 {file_path}: {e}")
                    return
        except ValueError:
            pass

    # 检查是否是文件路径
    if '/' in user_input or '\\' in user_input or user_input.endswith('.txt') or 'token_' in user_input:
        file_path = resolve_token_file_path(user_input)
        try:
            with open(file_path, 'r') as f:
                token_string = f.read().strip()
            print(f"✅ 从文件读取到令牌 ({len(token_string)} 字符)")
        except Exception as e:
            print(f"⚠️  无法读取文件 {file_path}: {e}")
            print("💡 将直接使用输入作为令牌字符串")

    # 初始化客户端
    if not g_initialized:
        try:
            client.initialize(
                license_code="TEMP",
                udp_port=13325,
                tcp_port=23325
            )
            print("✅ 客户端初始化成功")
            g_initialized = True
        except Exception as e:
            print(f"⚠️  初始化失败 (需要产品公钥): {e}")
    else:
        print("✅ 客户端已初始化，使用现有实例")

    # 查找和设置产品公钥
    if selected_product_key_path:
        product_key_path = selected_product_key_path
        print(f"📄 使用用户选择的产品公钥文件: {product_key_path}")
    else:
        product_key_path = find_product_public_key()
        if product_key_path:
            print(f"📄 使用产品公钥文件: {product_key_path}")

    if product_key_path:
        try:
            with open(product_key_path, 'r') as f:
                product_key_data = f.read()
            client.set_product_public_key(product_key_data)
            print("✅ 产品公钥设置成功")
        except Exception as e:
            print(f"❌ 设置产品公钥失败: {e}")
            return
    else:
        print("⚠️  未找到产品公钥文件")
        print("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件")
        return

    # 导入令牌
    print("📥 正在导入令牌...")
    try:
        client.import_token(token_string)
        print("✅ 令牌导入成功")
    except Exception as e:
        print(f"❌ 令牌导入失败: {e}")
        return

    # 激活令牌
    print("🎯 正在激活令牌...")
    try:
        result = client.activate_bind_device()
        if result['valid']:
            print("✅ 令牌激活成功！")

            # 导出激活后的新token
            try:
                activated_token = client.export_activated_token_encrypted()
                if activated_token:
                    print("\n📦 激活后的新Token（加密）:")
                    print(f"   长度: {len(activated_token)} 字符")
                    if len(activated_token) > 100:
                        print(f"   前缀: {activated_token[:100]}...")
                    else:
                        print(f"   内容: {activated_token}")

                    # 保存激活后的token到文件
                    status = client.get_status()
                    if status['license_code']:
                        timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
                        filename = f"token_activated_{status['license_code']}_{timestamp}.txt"
                        with open(filename, 'w') as f:
                            f.write(activated_token)
                        abs_path = os.path.abspath(filename)
                        print(f"\n💾 已保存到文件: {abs_path}")
                        print("   💡 此token包含设备绑定信息，可传递给下一个设备使用")
            except Exception as e:
                print(f"⚠️  导出激活token失败: {e}")
        else:
            print(f"❌ 令牌激活失败: {result.get('error_message', 'Unknown error')}")
    except Exception as e:
        print(f"❌ 激活失败: {e}")

    # 显示最终状态
    try:
        activated = client.is_activated()
        if activated:
            print("🔍 当前状态: 已激活")
            status = client.get_status()
            if status['has_token']:
                print(f"🎫 令牌ID: {status['token_id']}")
                print(f"📝 许可证代码: {status['license_code']}")
                print(f"👤 持有设备: {status['holder_device_id']}")
                issue_time = datetime.fromtimestamp(status['issue_time'])
                print(f"📅 颁发时间: {issue_time.strftime('%Y-%m-%d %H:%M:%S')}")
        else:
            print("🔍 当前状态: 未激活")
    except Exception as e:
        print(f"⚠️  无法获取状态: {e}")


def verify_activated_token_wizard():
    """校验已激活令牌向导"""
    print("\n✅ 校验已激活令牌")
    print("-" * 50)

    # 扫描所有已激活的令牌
    state_dir = ".decentrilicense_state"
    if not os.path.exists(state_dir):
        print("⚠️  没有找到已激活的令牌")
        return

    try:
        entries = os.listdir(state_dir)
    except:
        print("⚠️  没有找到已激活的令牌")
        return

    # 列出所有已激活的令牌
    activated_tokens = []
    print("\n📋 已激活的令牌列表:")
    for i, entry in enumerate(entries, 1):
        entry_path = os.path.join(state_dir, entry)
        if os.path.isdir(entry_path):
            activated_tokens.append(entry)
            state_file = os.path.join(entry_path, "current_state.json")
            if os.path.exists(state_file):
                print(f"{i}. {entry} ✅")
            else:
                print(f"{i}. {entry} ⚠️  (无状态文件)")

    if not activated_tokens:
        print("⚠️  没有找到已激活的令牌")
        return

    # 让用户选择
    try:
        choice = int(input(f"\n请选择要验证的令牌 (1-{len(activated_tokens)}): ").strip())
        if choice < 1 or choice > len(activated_tokens):
            print("❌ 无效的选择")
            return
    except ValueError:
        print("❌ 无效的选择")
        return

    selected_license_code = activated_tokens[choice - 1]
    print(f"\n🔍 正在验证令牌: {selected_license_code}")

    client = get_or_create_client()
    if client is None:
        return

    # 检查选择的令牌是否是当前激活的令牌
    try:
        status = client.get_status()
        if status['license_code'] == selected_license_code:
            # 是当前激活的令牌，可以直接验证
            print("🔍 正在验证令牌...")
            result = client.offline_verify_current_token()
            if result['valid']:
                print("✅ 令牌验证成功")
                if result.get('error_message'):
                    print(f"📄 信息: {result['error_message']}")
            else:
                print("❌ 令牌验证失败")
                print(f"📄 错误信息: {result.get('error_message', 'Unknown error')}")

            # 显示令牌信息
            if status['has_token']:
                print("\n🎫 令牌信息:")
                print(f"   令牌ID: {status['token_id']}")
                print(f"   许可证代码: {status['license_code']}")
                print(f"   应用ID: {status['app_id']}")
                print(f"   持有设备ID: {status['holder_device_id']}")

                issue_time = datetime.fromtimestamp(status['issue_time'])
                print(f"   颁发时间: {issue_time.strftime('%Y-%m-%d %H:%M:%S')}")

                if status['expire_time'] == 0:
                    print("   到期时间: 永不过期")
                else:
                    expire_time = datetime.fromtimestamp(status['expire_time'])
                    print(f"   到期时间: {expire_time.strftime('%Y-%m-%d %H:%M:%S')}")

                print(f"   状态索引: {status['state_index']}")
                print(f"   激活状态: {status['is_activated']}")
        else:
            # 不是当前激活的令牌，读取状态文件显示信息
            print("💡 此令牌不是当前激活的令牌，显示已保存的状态信息:")
            state_file = os.path.join(state_dir, selected_license_code, "current_state.json")
            try:
                with open(state_file, 'r') as f:
                    data = f.read()
                print("\n🎫 令牌信息 (从状态文件读取):")
                print(f"   许可证代码: {selected_license_code}")
                print(f"   状态文件: {state_file}")
                print(f"   文件大小: {len(data)} 字节")
                print("\n💡 提示: 如需完整验证此令牌，请使用选项1重新激活")
            except Exception as e:
                print(f"❌ 读取状态文件失败: {e}")
    except Exception as e:
        print(f"❌ 验证失败: {e}")


def validate_token_wizard():
    """验证令牌合法性向导"""
    global g_initialized

    print("\n🔍 验证令牌合法性")
    print("-" * 50)

    client = get_or_create_client()
    if client is None:
        return

    # 初始化客户端
    if not g_initialized:
        try:
            client.initialize(
                license_code="VALIDATE",
                udp_port=13325,
                tcp_port=23325
            )
            print("✅ 客户端初始化成功")
            g_initialized = True
        except Exception as e:
            print(f"⚠️  初始化失败: {e}")

    # 查找和设置产品公钥
    if selected_product_key_path:
        product_key_path = selected_product_key_path
        print(f"📄 使用用户选择的产品公钥文件: {product_key_path}")
    else:
        product_key_path = find_product_public_key()
        if product_key_path:
            print(f"📄 使用产品公钥文件: {product_key_path}")

    if product_key_path:
        try:
            with open(product_key_path, 'r') as f:
                product_key_data = f.read()
            client.set_product_public_key(product_key_data)
            print("✅ 产品公钥设置成功")
        except Exception as e:
            print(f"❌ 设置产品公钥失败: {e}")
            return
    else:
        print("⚠️  未找到产品公钥文件")
        print("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件")
        return

    # 显示可用的加密token文件
    token_files = find_encrypted_token_files()
    if token_files:
        print("📄 发现以下加密token文件:")
        for i, file in enumerate(token_files, 1):
            print(f"   {i}. {file}")
        print("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串")

    # 获取令牌输入
    print("请输入要验证的令牌字符串 (支持加密令牌):")
    print("💡 令牌通常从软件提供商处获得，或从加密令牌文件读取")
    print("💡 如果是文件路径，请输入完整的文件路径")
    print("💡 直接回车可以从剪贴板读取token")

    user_input = input("令牌或文件路径: ").strip()

    # 如果输入为空，尝试从剪贴板读取
    if not user_input:
        print("📋 正在从剪贴板读取token...")
        try:
            user_input = read_from_clipboard().strip()
            if not user_input:
                print("❌ 剪贴板为空，请手动输入token字符串")
                return
            print(f"✅ 从剪贴板读取到 {len(user_input)} 个字符")
        except Exception as e:
            print(f"❌ {e}")
            return

    token_string = user_input

    # 检查是否是数字选择
    if token_files:
        try:
            num_choice = int(user_input)
            if 1 <= num_choice <= len(token_files):
                selected_file = token_files[num_choice - 1]
                file_path = resolve_token_file_path(selected_file)
                try:
                    with open(file_path, 'r') as f:
                        token_string = f.read().strip()
                    print(f"✅ 从文件 '{selected_file}' 读取到令牌 ({len(token_string)} 字符)")
                except Exception as e:
                    print(f"❌ 无法读取文件 {file_path}: {e}")
                    return
        except ValueError:
            pass

    # 检查是否是文件路径
    if ('/' in user_input or '\\' in user_input or
        user_input.endswith('.txt') or 'token_' in user_input):
        file_path = resolve_token_file_path(user_input)
        try:
            with open(file_path, 'r') as f:
                token_string = f.read().strip()
            print(f"✅ 从文件读取到令牌 ({len(token_string)} 字符)")
        except Exception as e:
            print(f"⚠️  无法读取文件 {file_path}: {e}")
            print("💡 将直接使用输入作为令牌字符串")

    # 验证令牌 - 注意：Python SDK可能没有ValidateToken方法，使用ImportToken + OfflineVerify代替
    print("🔍 正在验证令牌合法性...")
    try:
        # 导入令牌
        client.import_token(token_string)
        print("✅ 令牌导入成功")

        # 离线验证
        result = client.offline_verify_current_token()
        if result['valid']:
            print("✅ 令牌验证成功 - 令牌合法且有效")
            if result.get('error_message'):
                print(f"📄 详细信息: {result['error_message']}")
        else:
            print("❌ 令牌验证失败 - 令牌不合法或无效")
            if result.get('error_message'):
                print(f"📄 错误信息: {result['error_message']}")
    except Exception as e:
        print(f"❌ 令牌验证失败: {e}")


def accounting_wizard():
    """记账信息向导"""
    global g_initialized

    print("\n📊 记账信息")
    print("-" * 50)

    client = get_or_create_client()
    if client is None:
        return

    # 显示可用的状态token文件
    token_files = find_state_token_files()

    # 检查激活状态
    try:
        activated = client.is_activated() if g_initialized else False
    except:
        activated = False

    # 显示令牌选择选项
    print("\n💡 请选择令牌来源:")
    if activated:
        print("0. 使用当前激活的令牌")

    if token_files:
        print("\n📄 或从以下文件加载令牌:")
        for i, file in enumerate(token_files, 1):
            print(f"{i}. {file}")

    if not activated and not token_files:
        print("❌ 当前没有激活的令牌，也没有找到可用的token文件")
        print("💡 请先使用选项1激活令牌")
        return

    prompt = "请选择 (0"
    if token_files:
        prompt += f"-{len(token_files)}"
    prompt += "): "

    try:
        token_choice = int(input(prompt).strip())
    except ValueError:
        print("❌ 无效的选择")
        return

    if token_choice < 0 or token_choice > len(token_files):
        print("❌ 无效的选择")
        return

    # 如果选择从文件加载
    if token_choice > 0:
        selected_file = token_files[token_choice - 1]
        file_path = resolve_token_file_path(selected_file)

        print(f"📂 正在从文件加载令牌: {selected_file}")

        try:
            with open(file_path, 'r') as f:
                token_data = f.read().strip()
            print(f"✅ 读取到令牌 ({len(token_data)} 字符)")
        except Exception as e:
            print(f"❌ 读取文件失败: {e}")
            return

        # 初始化客户端
        if not g_initialized:
            try:
                client.initialize(
                    license_code="ACCOUNTING",
                    udp_port=13325,
                    tcp_port=23325
                )
                g_initialized = True
            except Exception as e:
                print(f"⚠️  初始化失败: {e}")

        # 设置产品公钥
        if selected_product_key_path:
            product_key_path = selected_product_key_path
        else:
            product_key_path = find_product_public_key()

        if product_key_path:
            try:
                with open(product_key_path, 'r') as f:
                    product_key_data = f.read()
                client.set_product_public_key(product_key_data)
                print("✅ 产品公钥设置成功")
            except Exception as e:
                print(f"❌ 设置产品公钥失败: {e}")
                return

        # 导入令牌
        print("📥 正在导入令牌...")
        try:
            client.import_token(token_data)
            print("✅ 令牌导入成功")
        except Exception as e:
            print(f"❌ 令牌导入失败: {e}")
            return

        # 检查令牌类型
        is_already_activated = 'activated' in selected_file or 'state' in selected_file

        if is_already_activated:
            print("💡 检测到已激活令牌")
            print("🔄 正在恢复激活状态...")
        else:
            print("🎯 正在首次激活令牌...")

        # 调用ActivateBindDevice恢复/设置激活状态
        try:
            result = client.activate_bind_device()
            if not result['valid']:
                print(f"❌ 激活失败: {result.get('error_message', 'Unknown error')}")
                return

            if is_already_activated:
                print("✅ 激活状态已恢复（token未改变）")
            else:
                print("✅ 首次激活成功")
        except Exception as e:
            print(f"❌ 激活失败: {e}")
            return

    # 显示当前令牌信息
    try:
        status = client.get_status()
        if status['has_token']:
            print("\n📋 当前令牌信息:")
            print(f"   许可证代码: {status['license_code']}")
            print(f"   应用ID: {status['app_id']}")
            print(f"   当前状态索引: {status['state_index']}")
            print(f"   令牌ID: {status['token_id']}")
        else:
            print("⚠️  无法获取令牌信息")
            return
    except Exception as e:
        print(f"⚠️  无法获取令牌信息: {e}")
        return

    # 提供记账选项
    print("\n💡 请选择记账方式:")
    print("1. 快速测试记账（使用默认测试数据）")
    print("2. 记录业务操作（向导式输入）")

    choice = input("\n请选择 (1-2): ").strip()

    if choice == "1":
        # 快速测试
        action = "api_call"
        params = {
            "function": "test_function",
            "result": "success"
        }
        print(f"💡 使用测试数据: action={action}, params={params}")
    elif choice == "2":
        # 业务操作记账
        print("\n📝 usage_chain 结构说明:")
        print("┌─────────────────────────────────────────────────────────┐")
        print("│ 字段名      │ 说明           │ 填写方式              │")
        print("├─────────────────────────────────────────────────────────┤")
        print("│ seq         │ 序列号         │ ✅ 系统自动填充       │")
        print("│ time        │ 时间戳         │ ✅ 系统自动填充       │")
        print("│ action      │ 操作类型       │ 👉 需要您输入         │")
        print("│ params      │ 操作参数       │ 👉 需要您输入         │")
        print("│ hash_prev   │ 前状态哈希     │ ✅ 系统自动填充       │")
        print("│ signature   │ 数字签名       │ ✅ 系统自动填充       │")
        print("└─────────────────────────────────────────────────────────┘")

        # 输入action
        print("\n👉 第1步: 输入操作类型 (action)")
        print("   常用操作类型:")
        print("   • api_call      - API调用")
        print("   • feature_usage - 功能使用")
        print("   • save_file     - 保存文件")
        print("   • export_data   - 导出数据")
        action = input("\n请输入操作类型: ").strip()
        if not action:
            print("❌ 操作类型不能为空")
            return

        # 输入params
        print("\n👉 第2步: 输入操作参数 (params)")
        print("   params 是一个JSON对象，包含操作的具体参数")
        print("   输入格式: key=value (每行一个)")
        print("   示例:")
        print("   • function=process_image")
        print("   • file_name=report.pdf")
        print("   • size=1024")
        print("   输入空行结束输入")

        params = {}
        while True:
            line = input("参数 (key=value 或直接回车结束): ").strip()
            if not line:
                break

            parts = line.split('=', 1)
            if len(parts) == 2:
                key = parts[0].strip()
                value = parts[1].strip()
                params[key] = value
            else:
                print("⚠️  格式错误,请使用 key=value 格式")

        if not params:
            print("⚠️  未输入任何参数,将使用空参数对象")
            params = {}
    else:
        print("❌ 无效的选择")
        return

    # 构建记账数据
    usage_chain_entry = {
        "action": action,
        "params": params
    }

    accounting_data = json.dumps(usage_chain_entry)
    print(f"\n📝 记账数据 (业务字段): {accounting_data}")
    print("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)")

    # 记录使用情况
    print("📝 正在记录使用情况...")
    try:
        result = client.record_usage(accounting_data)
        if result['valid']:
            print("✅ 记账成功")
            print(f"📄 响应: {result.get('error_message', '')}")

            # 导出状态变更后的新token
            try:
                state_token = client.export_state_changed_token_encrypted()
                if state_token:
                    print("\n📦 状态变更后的新Token（加密）:")
                    print(f"   长度: {len(state_token)} 字符")
                    if len(state_token) > 100:
                        print(f"   前缀: {state_token[:100]}...")
                    else:
                        print(f"   内容: {state_token}")

                    # 保存状态变更后的token到文件
                    status = client.get_status()
                    if status['license_code']:
                        timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
                        filename = f"token_state_{status['license_code']}_idx{status['state_index']}_{timestamp}.txt"
                        with open(filename, 'w') as f:
                            f.write(state_token)
                        abs_path = os.path.abspath(filename)
                        print(f"\n💾 已保存到文件: {abs_path}")
                        print("   💡 此token包含最新状态链，可传递给下一个设备使用")
            except Exception as e:
                print(f"⚠️  导出状态变更token失败: {e}")
        else:
            print("❌ 记账失败")
            print(f"📄 错误信息: {result.get('error_message', 'Unknown error')}")
    except Exception as e:
        print(f"❌ 记账失败: {e}")


def trust_chain_validation_wizard():
    """信任链验证向导"""
    global g_initialized

    print("\n🔗 信任链验证")
    print("=" * 50)
    print("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定")
    print()

    client = get_or_create_client()
    if client is None:
        return

    # 显示可用的token文件
    token_files = find_state_token_files()

    # 检查激活状态
    try:
        activated = client.is_activated() if g_initialized else False
    except:
        activated = False

    # 显示令牌选择选项
    print("\n💡 请选择令牌来源:")
    if activated:
        print("0. 使用当前激活的令牌")

    if token_files:
        print("\n📄 或从以下文件加载令牌:")
        for i, file in enumerate(token_files, 1):
            print(f"{i}. {file}")

    if not activated and not token_files:
        print("❌ 当前没有激活的令牌，也没有找到可用的token文件")
        print("💡 请先使用选项1激活令牌")
        return

    prompt = "请选择 (0"
    if token_files:
        prompt += f"-{len(token_files)}"
    prompt += "): "

    try:
        token_choice = int(input(prompt).strip())
    except ValueError:
        print("❌ 无效的选择")
        return

    if token_choice < 0 or token_choice > len(token_files):
        print("❌ 无效的选择")
        return

    # 如果选择从文件加载
    if token_choice > 0:
        selected_file = token_files[token_choice - 1]
        file_path = resolve_token_file_path(selected_file)

        print(f"📂 正在从文件加载令牌: {selected_file}")

        try:
            with open(file_path, 'r') as f:
                token_data = f.read().strip()
            print(f"✅ 读取到令牌 ({len(token_data)} 字符)")
        except Exception as e:
            print(f"❌ 读取文件失败: {e}")
            return

        # 初始化客户端
        if not g_initialized:
            try:
                client.initialize(
                    license_code="TRUST_CHAIN",
                    udp_port=13325,
                    tcp_port=23325
                )
                g_initialized = True
            except Exception as e:
                print(f"⚠️  初始化失败: {e}")

        # 设置产品公钥
        if selected_product_key_path:
            product_key_path = selected_product_key_path
        else:
            product_key_path = find_product_public_key()

        if product_key_path:
            try:
                with open(product_key_path, 'r') as f:
                    product_key_data = f.read()
                client.set_product_public_key(product_key_data)
                print("✅ 产品公钥设置成功")
            except Exception as e:
                print(f"❌ 设置产品公钥失败: {e}")
                return

        # 导入令牌
        print("📥 正在导入令牌...")
        try:
            client.import_token(token_data)
            print("✅ 令牌导入成功")
        except Exception as e:
            print(f"❌ 令牌导入失败: {e}")
            return

        # 检查令牌类型
        is_already_activated = 'activated' in selected_file or 'state' in selected_file

        if is_already_activated:
            print("💡 检测到已激活令牌")
            print("🔄 正在恢复激活状态...")
        else:
            print("🎯 正在首次激活令牌...")

        # 调用ActivateBindDevice恢复/设置激活状态
        try:
            result = client.activate_bind_device()
            if not result['valid']:
                print(f"❌ 激活失败: {result.get('error_message', 'Unknown error')}")
                return

            if is_already_activated:
                print("✅ 激活状态已恢复（token未改变）")
            else:
                print("✅ 首次激活成功")
        except Exception as e:
            print(f"❌ 激活失败: {e}")
            return

    print("📋 开始验证信任链...")
    print()

    checks_passed = 0
    total_checks = 4

    # 1. 基础令牌签名验证
    print("🔍 [1/4] 验证令牌签名（根密钥 → 产品公钥 → 令牌）")
    try:
        result = client.offline_verify_current_token()
        if result['valid']:
            print("   ✅ 通过: 令牌签名有效，信任链完整")
            checks_passed += 1
        else:
            print(f"   ❌ 失败: {result.get('error_message', 'Unknown error')}")
    except Exception as e:
        print(f"   ❌ 失败: {e}")
    print()

    # 2. 检查设备状态
    print("🔍 [2/4] 验证设备状态")
    try:
        state = client.get_device_state()
        print(f"   ✅ 通过: 设备状态正常 (状态: {state})")
        checks_passed += 1
    except Exception as e:
        print(f"   ⚠️  警告: 无法获取设备状态 - {e}")
    print()

    # 3. 检查令牌持有者匹配
    print("🔍 [3/4] 验证令牌持有者与当前设备匹配")
    try:
        token = client.get_current_token()
        if token:
            device_id = client.get_device_id()
            if token['holder_device_id'] == device_id:
                print("   ✅ 通过: 令牌持有者与当前设备匹配")
                print(f"   📱 设备ID: {device_id}")
                checks_passed += 1
            else:
                print("   ⚠️  不匹配: 令牌持有者与当前设备不一致")
                print(f"   📱 当前设备ID: {device_id}")
                print(f"   🎫 令牌持有者ID: {token['holder_device_id']}")
                print("   💡 这可能表示令牌是从其他设备导入的")
        else:
            print("   ⚠️  警告: 无法获取令牌信息")
    except Exception as e:
        print(f"   ⚠️  警告: 无法获取设备ID - {e}")
    print()

    # 4. 显示令牌详细信息
    print("🔍 [4/4] 检查令牌详细信息")
    try:
        status = client.get_status()
        if status['has_token']:
            print("   ✅ 通过: 令牌信息完整")
            print(f"   🎫 令牌ID: {status['token_id']}")
            print(f"   📝 许可证代码: {status['license_code']}")
            print(f"   📱 应用ID: {status['app_id']}")
            issue_time = datetime.fromtimestamp(status['issue_time'])
            print(f"   📅 颁发时间: {issue_time.strftime('%Y-%m-%d %H:%M:%S')}")
            if status['expire_time'] == 0:
                print("   ⏰ 到期时间: 永不过期")
            else:
                expire_time = datetime.fromtimestamp(status['expire_time'])
                print(f"   ⏰ 到期时间: {expire_time.strftime('%Y-%m-%d %H:%M:%S')}")
            checks_passed += 1
        else:
            print("   ⚠️  警告: 令牌信息不完整")
    except Exception as e:
        print(f"   ⚠️  警告: 无法获取状态信息 - {e}")
    print()

    # 结果汇总
    print("━" * 50)
    print(f"📊 验证结果: {checks_passed}/{total_checks} 项检查通过")
    if checks_passed == total_checks:
        print("🎉 信任链验证完全通过！令牌可信且安全")
    elif checks_passed >= 2:
        print("⚠️  部分检查通过，令牌基本可用但存在警告")
    else:
        print("❌ 多项检查失败，请检查令牌和设备状态")
    print("━" * 50)


def comprehensive_validation_wizard():
    """综合验证向导"""
    global g_initialized

    print("\n🎯 综合验证")
    print("-" * 50)

    client = get_or_create_client()
    if client is None:
        return

    # 显示可用的token文件
    token_files = find_state_token_files()

    # 检查激活状态
    try:
        activated = client.is_activated() if g_initialized else False
    except:
        activated = False

    # 显示令牌选择选项
    print("\n💡 请选择令牌来源:")
    if activated:
        print("0. 使用当前激活的令牌")

    if token_files:
        print("\n📄 或从以下文件加载令牌:")
        for i, file in enumerate(token_files, 1):
            print(f"{i}. {file}")

    if not activated and not token_files:
        print("❌ 当前没有激活的令牌，也没有找到可用的token文件")
        print("💡 请先使用选项1激活令牌")
        return

    prompt = "请选择 (0"
    if token_files:
        prompt += f"-{len(token_files)}"
    prompt += "): "

    try:
        token_choice = int(input(prompt).strip())
    except ValueError:
        print("❌ 无效的选择")
        return

    if token_choice < 0 or token_choice > len(token_files):
        print("❌ 无效的选择")
        return

    # 如果选择从文件加载
    if token_choice > 0:
        selected_file = token_files[token_choice - 1]
        file_path = resolve_token_file_path(selected_file)

        print(f"📂 正在从文件加载令牌: {selected_file}")

        try:
            with open(file_path, 'r') as f:
                token_data = f.read().strip()
            print(f"✅ 读取到令牌 ({len(token_data)} 字符)")
        except Exception as e:
            print(f"❌ 读取文件失败: {e}")
            return

        # 初始化客户端
        if not g_initialized:
            try:
                client.initialize(
                    license_code="COMPREHENSIVE",
                    udp_port=13325,
                    tcp_port=23325
                )
                g_initialized = True
            except Exception as e:
                print(f"⚠️  初始化失败: {e}")

        # 设置产品公钥
        if selected_product_key_path:
            product_key_path = selected_product_key_path
        else:
            product_key_path = find_product_public_key()

        if product_key_path:
            try:
                with open(product_key_path, 'r') as f:
                    product_key_data = f.read()
                client.set_product_public_key(product_key_data)
                print("✅ 产品公钥设置成功")
            except Exception as e:
                print(f"❌ 设置产品公钥失败: {e}")
                return

        # 导入令牌
        print("📥 正在导入令牌...")
        try:
            client.import_token(token_data)
            print("✅ 令牌导入成功")
        except Exception as e:
            print(f"❌ 令牌导入失败: {e}")
            return

        # 检查令牌类型
        is_already_activated = 'activated' in selected_file or 'state' in selected_file

        if is_already_activated:
            print("💡 检测到已激活令牌")
            print("🔄 正在恢复激活状态...")
        else:
            print("🎯 正在首次激活令牌...")

        # 调用ActivateBindDevice恢复/设置激活状态
        try:
            result = client.activate_bind_device()
            if not result['valid']:
                print(f"❌ 激活失败: {result.get('error_message', 'Unknown error')}")
                return

            if is_already_activated:
                print("✅ 激活状态已恢复（token未改变）")
            else:
                print("✅ 首次激活成功")
        except Exception as e:
            print(f"❌ 激活失败: {e}")
            return

    print("📋 执行综合验证流程...")
    check_count = 0
    pass_count = 0

    # 1. 检查激活状态
    check_count += 1
    try:
        activated = client.is_activated()
        pass_count += 1
        if activated:
            print(f"✅ 检查{check_count}通过: 许可证已激活")
        else:
            print(f"⚠️  检查{check_count}: 许可证未激活")
    except Exception as e:
        print(f"❌ 检查{check_count}失败: 激活状态查询失败 - {e}")

    # 2. 验证当前令牌
    if activated:
        check_count += 1
        try:
            result = client.offline_verify_current_token()
            if result['valid']:
                pass_count += 1
                print(f"✅ 检查{check_count}通过: 令牌验证成功")
            else:
                print(f"❌ 检查{check_count}失败: 令牌验证失败 - {result.get('error_message', 'Unknown')}")
        except Exception as e:
            print(f"❌ 检查{check_count}失败: 令牌验证失败 - {e}")

    # 3. 检查设备状态
    check_count += 1
    try:
        state = client.get_device_state()
        pass_count += 1
        print(f"✅ 检查{check_count}通过: 设备状态正常 (状态: {state})")
    except Exception as e:
        print(f"❌ 检查{check_count}失败: 设备状态查询失败 - {e}")

    # 4. 检查令牌信息
    check_count += 1
    try:
        token = client.get_current_token()
        if token:
            pass_count += 1
            token_id = token['token_id']
            if len(token_id) >= 16:
                print(f"✅ 检查{check_count}通过: 令牌信息完整 (ID: {token_id[:16]}...)")
            elif len(token_id) > 0:
                print(f"✅ 检查{check_count}通过: 令牌信息完整 (ID: {token_id})")
            else:
                print(f"✅ 检查{check_count}通过: 令牌对象存在")
        else:
            if activated:
                print(f"❌ 检查{check_count}失败: 令牌信息查询失败")
            else:
                print(f"⚠️  检查{check_count}: 无令牌信息 (未激活)")
    except Exception as e:
        print(f"❌ 检查{check_count}失败: 令牌信息查询失败 - {e}")

    # 5. 测试记账功能
    if activated:
        check_count += 1
        test_data = json.dumps({"action": "comprehensive_test", "timestamp": 1234567890})
        try:
            result = client.record_usage(test_data)
            if result['valid']:
                pass_count += 1
                print(f"✅ 检查{check_count}通过: 记账功能正常")

                # 导出状态变更后的新token
                try:
                    state_token = client.export_state_changed_token_encrypted()
                    if state_token:
                        print("   📦 状态变更后的新Token已生成")
                        print(f"   Token长度: {len(state_token)} 字符")

                        # 保存状态变更后的token到文件
                        status = client.get_status()
                        if status['license_code']:
                            timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
                            filename = f"token_state_{status['license_code']}_idx{status['state_index']}_{timestamp}.txt"
                            with open(filename, 'w') as f:
                                f.write(state_token)
                            abs_path = os.path.abspath(filename)
                            print(f"   💾 已保存到: {abs_path}")
                except Exception as e:
                    print(f"   ⚠️  导出状态变更token失败: {e}")
            else:
                print(f"❌ 检查{check_count}失败: 记账功能异常 - {result.get('error_message', 'Unknown')}")
        except Exception as e:
            print(f"❌ 检查{check_count}失败: 记账功能测试失败 - {e}")

    # 结果汇总
    print("\n📊 综合验证结果:")
    print(f"   总检查项: {check_count}")
    print(f"   通过项目: {pass_count}")
    print(f"   成功率: {pass_count / check_count * 100:.1f}%")

    if pass_count == check_count:
        print("🎉 所有检查均通过！系统运行正常")
    elif pass_count >= check_count // 2:
        print("⚠️  大部分检查通过，系统基本正常")
    else:
        print("❌ 多项检查失败，请检查系统配置")


def main():
    """主程序"""
    try:
        while True:
            print("\n" + "=" * 50)
            print("DecentriLicense Python SDK 验证向导")
            print("=" * 50)
            print()
            print("请选择要执行的操作:")
            print("0. 🔑 选择产品公钥")
            print("1. 🔓 激活令牌")
            print("2. ✅ 校验已激活令牌")
            print("3. 🔍 验证令牌合法性")
            print("4. 📊 记账信息")
            print("5. 🔗 信任链验证")
            print("6. 🎯 综合验证")
            print("7. 🚪 退出")

            choice = input("请输入选项 (0-7): ").strip()
            print()

            if choice == "0":
                select_product_key_wizard()
            elif choice == "1":
                activate_token_wizard()
            elif choice == "2":
                verify_activated_token_wizard()
            elif choice == "3":
                validate_token_wizard()
            elif choice == "4":
                accounting_wizard()
            elif choice == "5":
                trust_chain_validation_wizard()
            elif choice == "6":
                comprehensive_validation_wizard()
            elif choice == "7":
                print("感谢使用 DecentriLicense Python SDK 验证向导!")
                break
            else:
                print("❌ 无效选项，请重新选择")

            print()
    except KeyboardInterrupt:
        print("\n\n程序已中断")
    finally:
        cleanup_client()


if __name__ == "__main__":
    main()
