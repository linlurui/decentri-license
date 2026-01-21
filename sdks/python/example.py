#!/usr/bin/env python3
"""
DecentriLicense Python SDK Example
================================

This example demonstrates how to use the DecentriLicense Python SDK.
"""

import sys
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
            
            # Activate the license
            print("[2] Activating license...")
            print("    Broadcasting discovery on LAN...")
            print("    Waiting for peers and election...")
            print()
            
            # Note: In a real implementation, this would attempt to activate
            # For this example, we'll just show the interface
            try:
                result = client.activate()
                print(f"    Success: {result['success']}")
                print(f"    Message: {result['message']}")
            except LicenseError as e:
                print(f"    Activation failed: {e}")
            print()
            
            # Check activation status
            print("[3] Current status:")
            print(f"    Activated: {client.is_activated()}")
            print(f"    Device State: {client.get_device_state()}")
            print()
            
            # Get current token if available
            print("[4] Current token:")
            token = client.get_current_token()
            if token:
                print(f"    Token ID: {token['token_id']}")
                print(f"    Holder: {token['holder_device_id']}")
                print(f"    License: {license_code}")
            else:
                print("    No token available")
            print()
            
    except LicenseError as e:
        print(f"License error: {e}")
        return 1
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1
    
    print("[5] Example completed successfully!")
    return 0


if __name__ == "__main__":
    sys.exit(main())