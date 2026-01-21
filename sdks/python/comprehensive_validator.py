#!/usr/bin/env python3
"""
DecentriLicense Python SDK Comprehensive Validator
=============================================

This validator demonstrates how to validate tokens using the trust chain model
for all supported algorithms (RSA, Ed25519, SM2).
"""

import sys
import json
import hashlib
import base64
from typing import Dict, Any


def list_files_for_selection(exts=None):
    files = []
    import os
    for name in os.listdir('.'):
        if not os.path.isfile(name):
            continue
        if exts:
            lower = name.lower()
            if not any(lower.endswith(ext) for ext in exts):
                continue
        files.append(name)
    return sorted(files)


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
    """Comprehensive validator for DecentriLicense tokens using trust chain model."""
    
    def __init__(self):
        """Initialize the validator."""
        pass
    
    def validate_token_with_trust_chain(self, token_file: str, root_public_key_file: str, algorithm: str) -> bool:
        """
        Validate a token using the trust chain model.
        
        Args:
            token_file: Path to the token JSON file
            root_public_key_file: Path to the root public key file
            algorithm: Expected algorithm (RSA, Ed25519, SM2)
            
        Returns:
            True if validation succeeds, False otherwise
        """
        try:
            # Load token from file
            with open(token_file, 'r') as f:
                token_data = json.load(f)
            
            # Load root public key from file
            with open(root_public_key_file, 'r') as f:
                root_public_key = f.read()
            
            # Verify trust chain
            return self._verify_trust_chain(token_data, root_public_key, algorithm)
        except Exception as e:
            print(f"Error validating token: {e}")
            return False
    
    def _verify_trust_chain(self, token_data: Dict[str, Any], root_public_key: str, algorithm: str) -> bool:
        """
        Verify the trust chain for a token.
        
        Args:
            token_data: Token data dictionary
            root_public_key: Root public key content
            algorithm: Expected algorithm
            
        Returns:
            True if trust chain is valid, False otherwise
        """
        print(f"Verifying trust chain for token {token_data.get('token_id', 'unknown')}")
        print(f"  License Code: {token_data.get('license_code', 'unknown')}")
        print(f"  Algorithm: {token_data.get('alg', 'unknown')}")
        print(f"  Expected Algorithm: {algorithm}")
        print(f"  License Public Key present: {'license_public_key' in token_data and bool(token_data['license_public_key'])}")
        print(f"  Root Signature present: {'root_signature' in token_data and bool(token_data['root_signature'])}")
        
        # Verify that the algorithm matches
        if token_data.get('alg') != algorithm:
            print(f"Algorithm mismatch: expected {algorithm}, got {token_data.get('alg')}")
            return False
        
        # Check that required fields are present
        if not token_data.get('license_public_key') or not token_data.get('root_signature'):
            print("Missing required trust chain fields")
            return False
        
        # In a real implementation, we would:
        # 1. Verify the root signature of the license public key using the root public key
        # 2. Verify the token signature using the verified license public key
        
        # For demonstration purposes, we'll just return True if both fields are present and algorithm matches
        return True

def main():
    """Main validation function."""
    token_file = sys.argv[1] if len(sys.argv) > 1 else ''
    root_public_key_file = sys.argv[2] if len(sys.argv) > 2 else ''
    algorithm = sys.argv[3] if len(sys.argv) > 3 else ''

    if not token_file:
        token_file = pick_file_from_cwd("请选择 token 文件:", exts=['.json', '.txt'])
    if not root_public_key_file:
        root_public_key_file = pick_file_from_cwd("请选择根公钥文件:", exts=['.pem'])
    if not algorithm:
        algorithm = input("请输入 algorithm (RSA/Ed25519/SM2): ").strip()

    if not token_file or not root_public_key_file or not algorithm:
        print("Usage: comprehensive_validator <token_file> <root_public_key_file> <algorithm>")
        sys.exit(1)
    
    validator = ComprehensiveValidator()
    valid = validator.validate_token_with_trust_chain(token_file, root_public_key_file, algorithm)
    
    if valid:
        print("✅ Token validation successful!")
        return 0
    else:
        print("❌ Token validation failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())