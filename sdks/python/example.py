#!/usr/bin/env python3
"""
DecentriLicense Python SDK Example
================================

This example demonstrates how to use the DecentriLicense Python SDK
for both online (P2P) and offline token validation flows.
"""

import sys
import os
import time
from decenlicense import DecentriLicenseClient, LicenseError


def main():
    """Main example function."""
    # License code to use
    license_code = "EXAMPLE-LICENSE-12345"
    
    # If a license code was provided as a command line argument, use it
    if len(sys.argv) > 1:
        license_code = sys.argv[1]
    
    print("==========================================")
    print("DecentriLicense Python SDK Example")
    print("==========================================")
    print(f"Using license code: {license_code}")
    print()
    
    try:
        # Create and initialize the client
        with DecentriLicenseClient() as client:
            print("[1] Initializing client...")
            client.initialize(license_code, udp_port=8888, tcp_port=8889)
            print(f"    Device ID: {client.get_device_id()}")
            print("    Initialization successful!")
            print()
            
            # ── Online activation (P2P) ──────────────────────────────
            print("[2] Activating license (P2P)...")
            try:
                result = client.activate()
                print(f"    Success: {result['success']}")
                print(f"    Message: {result['message']}")
            except LicenseError as e:
                print(f"    Activation failed: {e}")
            print()
            
            # ── Offline token validation ─────────────────────────────
            print("[3] Offline token validation (validate_token)...")
            print("    This validates a token without activating it.")
            print("    Flow: import_token → offline_verify_current_token")
            # Example: validate an encrypted token string
            # token_str = "ciphertext_base64url|nonce_base64url"
            # result = client.validate_token(token_str)
            # print(f"    Valid: {result['valid']}")
            # print(f"    Error: {result['error_message']}")
            print("    (See comprehensive_validator.py for full example)")
            print()
            
            # ── Activate with offline token ──────────────────────────
            print("[4] Activate with offline token (activate_with_token)...")
            print("    This imports and activates a token in one call.")
            # Example: activate with a token string
            # try:
            #     result = client.activate_with_token(token_str)
            #     print(f"    Success: {result['success']}")
            # except LicenseError as e:
            #     print(f"    Failed: {e}")
            print("    (See validation_wizard.py for full example)")
            print()
            
            # ── Check activation status ──────────────────────────────
            print("[5] Current status:")
            print(f"    Activated: {client.is_activated()}")
            print(f"    Device State: {client.get_device_state()}")
            print()
            
            # ── Get current token ────────────────────────────────────
            print("[6] Current token:")
            token = client.get_current_token()
            if token:
                print(f"    Token ID: {token['token_id']}")
                print(f"    Holder: {token['holder_device_id']}")
                print(f"    License: {license_code}")
            else:
                print("    No token available")
            print()
            
            # ── Get detailed status ──────────────────────────────────
            print("[7] Detailed status:")
            try:
                status = client.get_status()
                print(f"    Has Token: {status['has_token']}")
                print(f"    Is Activated: {status['is_activated']}")
                print(f"    Token ID: {status['token_id']}")
                print(f"    Holder Device: {status['holder_device_id']}")
                print(f"    App ID: {status['app_id']}")
                print(f"    License Code: {status['license_code']}")
                print(f"    State Index: {status['state_index']}")
            except LicenseError as e:
                print(f"    Status query failed: {e}")
            print()
            
    except LicenseError as e:
        print(f"License error: {e}")
        return 1
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1
    
    print("[8] Example completed successfully!")
    return 0


if __name__ == "__main__":
    sys.exit(main())