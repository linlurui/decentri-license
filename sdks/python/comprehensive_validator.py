#!/usr/bin/env python3
"""
DecentriLicense Python SDK Comprehensive Validator
=============================================

Validates tokens using the trust chain model via dl-core C library.
Supports RSA, Ed25519, SM2 algorithms.

Trust chain verification:
1. Root signature of the license public key is verified using the root public key
2. Token signature is verified using the verified license public key

Usage:
    python comprehensive_validator.py <token_file> <root_public_key_file>
    python comprehensive_validator.py  (interactive mode)
"""

import sys
import json
import os
from typing import Dict, Any, Optional

from decenlicense import DecentriLicenseClient, LicenseError


def list_files_for_selection(exts=None):
    files = []
    for name in sorted(os.listdir('.')):
        if not os.path.isfile(name):
            continue
        if exts:
            lower = name.lower()
            if not any(lower.endswith(ext) for ext in exts):
                continue
        files.append(name)
    return files


def pick_file_from_cwd(title, exts=None):
    files = list_files_for_selection(exts)
    print(title)
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


class ComprehensiveValidator:
    """Comprehensive validator for DecentriLicense tokens using trust chain model.

    Uses the dl-core C library for real cryptographic verification:
    - Root signature verification via root public key
    - Token signature verification via license public key
    """

    def __init__(self):
        self._client: Optional[DecentriLicenseClient] = None

    def _ensure_client(self) -> DecentriLicenseClient:
        """Get or create a client instance."""
        if self._client is None:
            self._client = DecentriLicenseClient()
            self._client.initialize("", udp_port=0, tcp_port=0, registry_server_url="")
        return self._client

    def close(self):
        """Release client resources."""
        if self._client is not None:
            try:
                self._client.shutdown()
            except Exception:
                pass
            self._client = None

    def validate_token_with_trust_chain(self, token_file: str, root_public_key_file: str) -> bool:
        """Validate a token using the trust chain model via dl-core.

        Algorithm is auto-detected from the token's alg field
        (determined by product key type at issuance).

        Args:
            token_file: Path to the token JSON file
            root_public_key_file: Path to the root public key PEM file

        Returns:
            True if validation succeeds, False otherwise
        """
        try:
            # Load token from file
            with open(token_file, 'r', encoding='utf-8') as f:
                token_data = json.load(f)

            # Auto-detect algorithm from token's alg field
            algorithm = token_data.get('alg', 'RSA')
            print(f"Auto-detected algorithm from token: {algorithm}")

            # Load root public key from file
            with open(root_public_key_file, 'r', encoding='utf-8') as f:
                root_public_key = f.read()

            return self._verify_trust_chain(token_data, root_public_key, algorithm)
        except Exception as e:
            print(f"Error validating token: {e}")
            return False

    def _verify_trust_chain(self, token_data: Dict[str, Any], root_public_key: str,
                            algorithm: str) -> bool:
        """Verify the trust chain for a token using dl-core C library.

        Steps:
        1. Validate required fields and algorithm match
        2. Set product public key (contains ROOT_SIGNATURE for trust chain)
        3. Import token into client
        4. Use dl_client_verify_token_trust_chain for cryptographic verification

        Args:
            token_data: Token data dictionary
            root_public_key: Root public key PEM content
            algorithm: Expected algorithm

        Returns:
            True if trust chain is valid, False otherwise
        """
        token_id = token_data.get('token_id', 'unknown')
        license_code = token_data.get('license_code', 'unknown')
        token_alg = token_data.get('alg', 'unknown')

        print(f"Verifying trust chain for token {token_id}")
        print(f"  License Code: {license_code}")
        print(f"  Algorithm: {token_alg} (auto-detected from token)")
        print(f"  License Public Key present: {bool(token_data.get('license_public_key'))}")
        print(f"  Root Signature present: {bool(token_data.get('root_signature'))}")

        # Algorithm is determined by product key type, no manual selection needed

        # Step 2: Check required trust chain fields
        if not token_data.get('license_public_key'):
            print("❌ Missing license_public_key field (required for trust chain)")
            return False
        if not token_data.get('root_signature'):
            print("❌ Missing root_signature field (required for trust chain)")
            return False
        if not token_data.get('signature'):
            print("❌ Missing signature field (required for trust chain)")
            return False

        # Step 3: Use dl-core for cryptographic verification
        try:
            client = self._ensure_client()

            # Set the product public key (PEM containing ROOT_SIGNATURE)
            product_key = token_data.get('license_public_key', '')
            client.set_product_public_key(product_key)

            # Import the token first
            token_json = json.dumps(token_data)
            client.import_token(token_json)

            # Verify using trust chain (root public key + token struct)
            result = client.verify_token_trust_chain(token_data, root_public_key)

            if result.get('valid'):
                print("✅ Trust chain verification passed (dl-core cryptographic verification)")
                return True
            else:
                print(f"❌ Trust chain verification failed: {result.get('error_message', 'unknown')}")
                return False

        except LicenseError as e:
            print(f"❌ dl-core verification error: {e}")
            return False
        except Exception as e:
            print(f"❌ Unexpected error during verification: {e}")
            return False

    def validate_token_offline(self, token_file: str, product_public_key_file: str) -> bool:
        """Validate a token using offline verification (import + verify).

        This is the standard offline flow without trust chain.

        Args:
            token_file: Path to the token file (encrypted or JSON)
            product_public_key_file: Path to the product public key PEM file

        Returns:
            True if validation succeeds, False otherwise
        """
        try:
            with open(product_public_key_file, 'r', encoding='utf-8') as f:
                product_key = f.read()
            with open(token_file, 'r', encoding='utf-8') as f:
                token_input = f.read().strip()

            client = self._ensure_client()
            client.set_product_public_key(product_key)

            # Use validate_token (import + offline verify)
            result = client.validate_token(token_input)

            if result.get('valid'):
                print("✅ Offline token validation passed")
                return True
            else:
                print(f"❌ Offline token validation failed: {result.get('error_message', 'unknown')}")
                return False

        except LicenseError as e:
            print(f"❌ Offline validation error: {e}")
            return False
        except Exception as e:
            print(f"❌ Unexpected error: {e}")
            return False


def main():
    """Main validation function."""
    print("==========================================")
    print("DecentriLicense Comprehensive Validator")
    print("==========================================")
    print()

    if len(sys.argv) >= 3:
        # CLI mode: comprehensive_validator <token_file> <root_public_key_file>
        token_file = sys.argv[1]
        root_public_key_file = sys.argv[2]
    else:
        # Interactive mode
        print("请选择验证模式:")
        print("1. 信任链验证 (Trust Chain)")
        print("2. 离线验证 (Offline)")
        mode = input("请输入选项 (1-2): ").strip()

        if mode == "2":
            token_file = pick_file_from_cwd("请选择 token 文件:", exts=['.txt', '.json'])
            product_key_file = pick_file_from_cwd("请选择产品公钥文件:", exts=['.pem'])
            if not token_file or not product_key_file:
                print("文件路径不能为空")
                sys.exit(1)
            validator = ComprehensiveValidator()
            valid = validator.validate_token_offline(token_file, product_key_file)
            validator.close()
            sys.exit(0 if valid else 1)

        # Trust chain mode
        token_file = pick_file_from_cwd("请选择 token 文件:", exts=['.json', '.txt'])
        root_public_key_file = pick_file_from_cwd("请选择根公钥文件:", exts=['.pem'])

    if not token_file or not root_public_key_file:
        print("Usage: comprehensive_validator <token_file> <root_public_key_file>")
        sys.exit(1)

    validator = ComprehensiveValidator()
    try:
        valid = validator.validate_token_with_trust_chain(token_file, root_public_key_file)
    finally:
        validator.close()

    if valid:
        print("✅ Token validation successful!")
        sys.exit(0)
    else:
        print("❌ Token validation failed!")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())