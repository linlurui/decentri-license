#include "decentrilicense/device_key_manager.hpp"
#include "decentrilicense/crypto_utils.hpp"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cstring>

#ifdef __APPLE__
#include <Security/Security.h>
#elif _WIN32
#include <windows.h>
#include <wincred.h>
#elif __linux__
#include <cstdlib>
#endif

namespace decentrilicense {

DeviceKeyManager::DeviceKeyManager() : initialized_(false) {
    device_id_ = generate_hardware_fingerprint();
    initialized_ = true;
}

DeviceKeyManager::~DeviceKeyManager() {
    // Cleanup if needed
}

std::string DeviceKeyManager::activate_and_generate_device_key(const std::string& license_token_json,
                                                              const std::string& license_id,
                                                              const std::string& product_id) {
    // Generate device key pair
    auto device_keypair = generate_device_keypair();
    
    // Store device private key securely
    if (!store_device_private_key_securely(device_keypair.private_key_pem)) {
        throw std::runtime_error("Failed to store device private key securely");
    }
    
    // Create device info
    DeviceInfo device_info;
    device_info.fingerprint = device_id_;
    device_info.public_key_pem = device_keypair.public_key_pem;
    device_info.signature = sign_device_info(device_info);
    
    // Update token JSON with device info
    // This is a simplified implementation - in practice, you would parse the JSON,
    // add the device_info fields, and re-serialize it
    std::ostringstream updated_token;
    updated_token << license_token_json.substr(0, license_token_json.length() - 1); // Remove closing brace
    updated_token << ",\"device_info\":{";
    updated_token << "\"fingerprint\":\"" << device_info.fingerprint << "\",";
    updated_token << "\"public_key\":\"" << device_info.public_key_pem << "\",";
    updated_token << "\"signature\":\"" << device_info.signature << "\"";
    updated_token << "}}";
    
    return updated_token.str();
}

bool DeviceKeyManager::verify_device_identity(const DeviceInfo& device_info) const {
    try {
        // Retrieve device private key (we only need the public key for verification)
        // In practice, the public key would be extracted from the device_info
        std::string public_key_pem = device_info.public_key_pem;
        
        // Create data to verify (fingerprint + public key)
        std::string data_to_verify = device_info.fingerprint + device_info.public_key_pem;
        
        // Verify signature using Ed25519
        return CryptoUtils::verify_ed25519_signature(data_to_verify, device_info.signature, public_key_pem);
    } catch (const std::exception& e) {
        std::cerr << "Error verifying device identity: " << e.what() << std::endl;
        return false;
    }
}

std::string DeviceKeyManager::add_usage_record(const std::string& token_json,
                                              const std::string& action,
                                              const std::string& params) {
    // This is a simplified implementation - in practice, you would:
    // 1. Parse the JSON
    // 2. Extract the usage_chain array
    // 3. Add a new record with seq, time, action, params, hash_prev, and signature
    // 4. Update current_signature
    // 5. Re-serialize the JSON
    
    // For now, we'll just return the original token_json
    return token_json;
}

bool DeviceKeyManager::verify_usage_chain(const std::string& token_json) const {
    // This is a simplified implementation - in practice, you would:
    // 1. Parse the JSON
    // 2. Extract the usage_chain array
    // 3. Verify each record's hash_prev points to the previous record
    // 4. Verify each record's signature using the product public key
    // 5. Verify the current_signature matches the entire token
    
    // For now, we'll just return true
    return true;
}

CryptoUtils::KeyPair DeviceKeyManager::generate_device_keypair() {
    return CryptoUtils::generate_ed25519_keypair();
}

bool DeviceKeyManager::store_device_private_key_securely(const std::string& private_key_pem) {
#ifdef __APPLE__
    return store_key_macos(private_key_pem);
#elif _WIN32
    return store_key_windows(private_key_pem);
#elif __linux__
    return store_key_linux(private_key_pem);
#else
    // Fallback: store in encrypted file
    // This is not secure and should only be used for development/testing
    std::cerr << "Warning: Using insecure fallback storage for device private key" << std::endl;
    return false;
#endif
}

std::string DeviceKeyManager::retrieve_device_private_key_securely() const {
#ifdef __APPLE__
    return retrieve_key_macos();
#elif _WIN32
    return retrieve_key_windows();
#elif __linux__
    return retrieve_key_linux();
#else
    // Fallback: read from encrypted file
    // This is not secure and should only be used for development/testing
    std::cerr << "Warning: Using insecure fallback retrieval for device private key" << std::endl;
    return "";
#endif
}

std::string DeviceKeyManager::generate_hardware_fingerprint() const {
    // Simple implementation - in a real-world scenario, this would collect
    // machine-specific information like MAC addresses, CPU info, etc.
    return CryptoUtils::sha256("device_specific_data_" + std::to_string(std::time(nullptr)));
}

std::string DeviceKeyManager::sign_device_info(const DeviceInfo& device_info) const {
    try {
        // Retrieve device private key
        std::string private_key_pem = retrieve_device_private_key_securely();
        if (private_key_pem.empty()) {
            throw std::runtime_error("Device private key not found");
        }
        
        // Create data to sign (fingerprint + public key)
        std::string data_to_sign = device_info.fingerprint + device_info.public_key_pem;
        
        // Sign using Ed25519
        return CryptoUtils::sign_ed25519_data(data_to_sign, private_key_pem);
    } catch (const std::exception& e) {
        std::cerr << "Error signing device info: " << e.what() << std::endl;
        throw;
    }
}

#ifdef __APPLE__
bool DeviceKeyManager::store_key_macos(const std::string& key_data) {
    // Implementation for macOS using Keychain Services
    // This is a simplified placeholder - a real implementation would use Security.framework
    std::cerr << "macOS key storage not implemented" << std::endl;
    return false;
}

std::string DeviceKeyManager::retrieve_key_macos() const {
    // Implementation for macOS using Keychain Services
    // This is a simplified placeholder - a real implementation would use Security.framework
    std::cerr << "macOS key retrieval not implemented" << std::endl;
    return "";
}
#elif _WIN32
bool DeviceKeyManager::store_key_windows(const std::string& key_data) {
    // Implementation for Windows using Data Protection API (DPAPI)
    // This is a simplified placeholder - a real implementation would use Windows Credential Manager
    std::cerr << "Windows key storage not implemented" << std::endl;
    return false;
}

std::string DeviceKeyManager::retrieve_key_windows() const {
    // Implementation for Windows using Data Protection API (DPAPI)
    // This is a simplified placeholder - a real implementation would use Windows Credential Manager
    std::cerr << "Windows key retrieval not implemented" << std::endl;
    return "";
}
#elif __linux__
bool DeviceKeyManager::store_key_linux(const std::string& key_data) {
    // Implementation for Linux using libsecret or similar
    // This is a simplified placeholder - a real implementation would use libsecret
    std::cerr << "Linux key storage not implemented" << std::endl;
    return false;
}

std::string DeviceKeyManager::retrieve_key_linux() const {
    // Implementation for Linux using libsecret or similar
    // This is a simplified placeholder - a real implementation would use libsecret
    std::cerr << "Linux key retrieval not implemented" << std::endl;
    return "";
}
#endif

} // namespace decentrilicense