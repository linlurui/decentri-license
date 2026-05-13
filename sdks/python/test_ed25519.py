#!/usr/bin/env python3
"""Test Python SDK with windsurf-free and cursor-free Ed25519 tokens."""

import json
import os
import sys

# Set library path
os.environ['DYLD_LIBRARY_PATH'] = '/Volumes/workspace/project/dl-issuer/dl-core/build:' + os.environ.get('DYLD_LIBRARY_PATH', '')

import decenlicense

SDK_DIR = os.path.dirname(os.path.abspath(__file__))
DL_ISSUER_DIR = '/Volumes/workspace/project/dl-issuer/dl-issuer'

def read_file(path):
    with open(path, 'r') as f:
        return f.read()

def new_client():
    client = decenlicense.DecentriLicenseClient()
    client.initialize(license_code='', udp_port=13325, tcp_port=23325)
    return client

def test_windsurf_free():
    token_json = read_file(f'{DL_ISSUER_DIR}/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json')
    product_key = read_file(f'{DL_ISSUER_DIR}/public_windsurf-free_20260427090402.pem')

    token_data = json.loads(token_json)
    print(f"windsurf-free token alg: {token_data.get('alg')}")

    # Test 1: Validate token
    client = new_client()
    try:
        client.set_product_public_key(product_key)
        result = client.validate_token(token_json)
        assert result['valid'], f"windsurf-free validate failed: {result['error_message']}"
        print("✅ Python SDK: windsurf-free validate_token passed")
    finally:
        client.shutdown()

    # Test 2: ActivateBindDevice
    client = new_client()
    try:
        client.set_product_public_key(product_key)
        client.import_token(token_json)
        result = client.activate_bind_device()
        assert result['valid'], f"windsurf-free activate failed: {result['error_message']}"
        print("✅ Python SDK: windsurf-free activate_bind_device passed")
    finally:
        client.shutdown()

def test_cursor_free():
    token_json = read_file(f'{DL_ISSUER_DIR}/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json')
    product_key = read_file(f'{DL_ISSUER_DIR}/public_cursor-free_20260427090405.pem')

    token_data = json.loads(token_json)
    print(f"cursor-free token alg: {token_data.get('alg')}")

    # Test 1: Validate token
    client = new_client()
    try:
        client.set_product_public_key(product_key)
        result = client.validate_token(token_json)
        assert result['valid'], f"cursor-free validate failed: {result['error_message']}"
        print("✅ Python SDK: cursor-free validate_token passed")
    finally:
        client.shutdown()

    # Test 2: ActivateBindDevice
    client = new_client()
    try:
        client.set_product_public_key(product_key)
        client.import_token(token_json)
        result = client.activate_bind_device()
        assert result['valid'], f"cursor-free activate failed: {result['error_message']}"
        print("✅ Python SDK: cursor-free activate_bind_device passed")
    finally:
        client.shutdown()

if __name__ == '__main__':
    test_windsurf_free()
    test_cursor_free()
    print("\n🎉 All Python SDK tests passed!")
