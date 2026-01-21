#ifndef DECENTRILICENSE_DEVICE_KEY_MANAGER_HPP
#define DECENTRILICENSE_DEVICE_KEY_MANAGER_HPP

#include <string>
#include <memory>
#include "crypto_utils.hpp"

namespace decentrilicense {

/**
 * DeviceKeyManager - Manages device-specific key pairs for enhanced verification and traceability
 * 
 * Features:
 * - Generates Ed25519 key pairs for device identity
 * - Securely stores device private keys using platform-specific secure storage
 * - Signs device identity information with device private keys
 * - Verifies device identity signatures
 */
class DeviceKeyManager {
public:
    // Device info structure
    struct DeviceInfo {
        std::string fingerprint;     // Hardware fingerprint
        std::string public_key_pem;  // Device public key in PEM format
        std::string signature;       // Signature of device info using device private key
    };
    
    /**
     * Constructor
     */
    DeviceKeyManager();
    
    /**
     * Destructor
     */
    ~DeviceKeyManager();
    
    /**
     * Generate device key pair and register it with the license token
     * Called during first activation
     * @param license_token_json JSON string of the license token
     * @param license_id License ID
     * @param product_id Product ID
     * @return Updated token JSON with device info
     */
    std::string activate_and_generate_device_key(const std::string& license_token_json,
                                               const std::string& license_id,
                                               const std::string& product_id);
    
    /**
     * Verify device identity using the device signature in the token
     * @param device_info Device info structure from token
     * @return true if device identity is valid
     */
    bool verify_device_identity(const DeviceInfo& device_info) const;
    
    /**
     * Add a usage record to the token's usage chain
     * @param token_json JSON string of the license token
     * @param action Action performed (e.g., "api_call")
     * @param params Action parameters
     * @return Updated token JSON with new usage record
     */
    std::string add_usage_record(const std::string& token_json,
                                const std::string& action,
                                const std::string& params);
    
    /**
     * Verify the entire usage chain of a token
     * @param token_json JSON string of the license token
     * @return true if usage chain is valid
     */
    bool verify_usage_chain(const std::string& token_json) const;
    
private:
    /**
     * Generate Ed25519 key pair for device identity
     * @return KeyPair structure containing PEM-encoded keys
     */
    CryptoUtils::KeyPair generate_device_keypair();
    
    /**
     * Store device private key securely based on platform
     * @param private_key_pem Private key in PEM format
     * @return true if storage successful
     */
    bool store_device_private_key_securely(const std::string& private_key_pem);
    
    /**
     * Retrieve device private key from secure storage
     * @return Private key in PEM format, empty string if not found
     */
    std::string retrieve_device_private_key_securely() const;
    
    /**
     * Generate hardware fingerprint for device identification
     * @return Hardware fingerprint string
     */
    std::string generate_hardware_fingerprint() const;
    
    /**
     * Sign device info with device private key
     * @param device_info Device info to sign
     * @return Base64-encoded signature
     */
    std::string sign_device_info(const DeviceInfo& device_info) const;
    
    /**
     * Platform-specific secure storage implementations
     */
#ifdef __APPLE__
    bool store_key_macos(const std::string& key_data);
    std::string retrieve_key_macos() const;
#elif _WIN32
    bool store_key_windows(const std::string& key_data);
    std::string retrieve_key_windows() const;
#elif __linux__
    bool store_key_linux(const std::string& key_data);
    std::string retrieve_key_linux() const;
#endif
    
    // Private members
    std::string device_id_;
    bool initialized_;
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_DEVICE_KEY_MANAGER_HPP