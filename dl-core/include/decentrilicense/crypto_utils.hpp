#ifndef DECENTRILICENSE_CRYPTO_UTILS_HPP
#define DECENTRILICENSE_CRYPTO_UTILS_HPP

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

namespace decentrilicense {

/**
 * CryptoUtils - Cryptographic utilities using OpenSSL
 * 
 * Provides:
 * - RSA/ECC/SM2 key generation and serialization
 * - Digital signatures (RSA-SHA256, Ed25519, SM2-SM3)
 * - AES-GCM encryption/decryption
 * - SHA-256/SM3 hashing
 * - Secure random number generation
 * 
 * All functions are thread-safe
 */
class CryptoUtils {
public:
    // RSA key pair structure
    struct KeyPair {
        std::string public_key_pem;
        std::string private_key_pem;
    };
    
    /**
     * Generate RSA key pair
     * @param key_size Key size in bits (2048 or 4096 recommended)
     * @return KeyPair structure containing PEM-encoded keys
     */
    static KeyPair generate_rsa_keypair(int key_size = 2048);
    
    /**
     * Generate Ed25519 key pair
     * @return KeyPair structure containing PEM-encoded keys
     */
    static KeyPair generate_ed25519_keypair();
    
    /**
     * Generate SM2 key pair
     * @return KeyPair structure containing PEM-encoded keys
     */
    static KeyPair generate_sm2_keypair();
    
    /**
     * Sign data using RSA private key (SHA-256)
     * @param data Data to sign
     * @param private_key_pem Private key in PEM format
     * @return Base64-encoded signature
     */
    static std::string sign_data(const std::string& data, const std::string& private_key_pem);
    
    /**
     * Verify RSA signature
     * @param data Original data
     * @param signature Base64-encoded signature
     * @param public_key_pem Public key in PEM format
     * @return true if signature is valid
     */
    static bool verify_signature(const std::string& data, 
                               const std::string& signature,
                               const std::string& public_key_pem);
    
    /**
     * Sign data using Ed25519 private key
     * @param data Data to sign
     * @param private_key_pem Private key in PEM format
     * @return Base64-encoded signature
     */
    static std::string sign_ed25519_data(const std::string& data, const std::string& private_key_pem);
    
    /**
     * Verify Ed25519 signature
     * @param data Original data
     * @param signature Base64-encoded signature
     * @param public_key_pem Public key in PEM format
     * @return true if signature is valid
     */
    static bool verify_ed25519_signature(const std::string& data, 
                                       const std::string& signature,
                                       const std::string& public_key_pem);

    /**
     * Sign data using SM2 private key with SM3 hash
     * @param data Data to sign
     * @param private_key_pem Private key in PEM format
     * @return Base64-encoded signature
     */
    static std::string sign_sm2_data(const std::string& data, const std::string& private_key_pem);
    
    /**
     * Verify SM2 signature
     * @param data Original data
     * @param signature Base64-encoded signature
     * @param public_key_pem Public key in PEM format
     * @return true if signature is valid
     */
    static bool verify_sm2_signature(const std::string& data, 
                                   const std::string& signature,
                                   const std::string& public_key_pem);

    /**
     * Encrypt data using AES-256-GCM
     * @param plaintext Data to encrypt
     * @param key Encryption key (32 bytes for AES-256)
     * @return Base64-encoded ciphertext (includes IV and auth tag)
     */
    static std::string aes_encrypt(const std::string& plaintext, const std::string& key);
    
    /**
     * Decrypt data using AES-256-GCM
     * @param ciphertext Base64-encoded ciphertext
     * @param key Decryption key (32 bytes for AES-256)
     * @return Decrypted plaintext
     */
    static std::string aes_decrypt(const std::string& ciphertext, const std::string& key);
    
    /**
     * Compute SHA-256 hash of license public key for use as AES key
     * @param license_public_key_pem License public key in PEM format
     * @return SHA-256 hash of the license public key
     */
    static std::string compute_license_key_hash(const std::string& license_public_key_pem);
    
    /**
     * Compute SHA-256 hash
     * @param data Input data
     * @return Hex-encoded hash
     */
    static std::string sha256(const std::string& data);
    
    /**
     * Generate cryptographically secure random bytes
     * @param num_bytes Number of random bytes to generate
     * @return Vector of random bytes
     */
    static std::vector<uint8_t> random_bytes(size_t num_bytes);
    
    /**
     * Generate UUID v4
     * @return UUID string
     */
    static std::string generate_uuid();
    
    /**
     * Generate device ID (SHA-256 of machine-specific data)
     * @return Device ID string
     */
    static std::string generate_device_id();
    
    /**
     * Base64 encode
     * @param data Input data
     * @return Base64-encoded string
     */
    static std::string base64_encode(const std::vector<uint8_t>& data);
    static std::string base64_encode(const std::string& data);
    
    /**
     * Base64 decode
     * @param data Base64-encoded string
     * @return Decoded data
     */
    static std::vector<uint8_t> base64_decode(const std::string& data);
    static std::string base64url_encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> base64url_decode(const std::string& data);
    static std::array<uint8_t, 32> sha256_bytes(const std::string& data);
    static std::array<uint8_t, 32> derive_aes256_key_from_product_public_key(const std::string& product_public_key_file_content);
    static std::string encrypt_token_aes256_gcm(const std::string& token_json, const std::string& product_public_key_file_content);
    static std::string decrypt_token_aes256_gcm(const std::string& encrypted_token, const std::string& product_public_key_file_content);

private:
    static void initialize_openssl();
    static void cleanup_openssl();
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_CRYPTO_UTILS_HPP