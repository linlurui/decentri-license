#ifndef DECENTRILICENSE_TOKEN_MANAGER_HPP
#define DECENTRILICENSE_TOKEN_MANAGER_HPP

#include <string>
#include <optional>
#include <functional>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include "network_manager.hpp"

namespace decentrilicense {

// Algorithm enumeration for signing
enum class SigningAlgorithm {
    RSA,
    Ed25519,
    SM2
};

// Enhanced Token structure with algorithm support and device identity
struct Token {
    std::string token_id;                   // UUID
    std::string holder_device_id;           // Current holder (can be empty initially)
    std::string license_code;               // Associated license
    uint64_t issue_time;                    // Unix timestamp
    uint64_t expire_time;                   // Unix timestamp
    std::string signature;                  // Cryptographic signature
    std::string alg;                        // Algorithm identifier ("RSA", "Ed25519", "SM2")
    std::string app_id;                     // Application identifier for business isolation
    std::string environment_hash;           // Optional environment hash for anti-copy protection
    std::string license_public_key;         // License public key (PEM string)
    std::string root_signature;             // Root signature of license public key
    std::string encrypted_license_private_key; // Encrypted license private key (optional)
    
    // State chain fields for offline state recording
    uint64_t state_index;                   // State index, starting from 0
    std::string prev_state_hash;            // SHA256 hash of previous token's JSON
    std::string state_payload;              // JSON string containing state update business data
    std::string state_signature;            // Signature of state_index + prev_state_hash + state_payload using license private key
    
    // Device identity fields for enhanced verification and traceability
    struct DeviceInfo {
        std::string fingerprint;            // Hardware fingerprint
        std::string public_key;             // Device public key in PEM format
        std::string signature;              // Signature of device info using device private key
    } device_info;
    
    // Usage chain for traceability
    struct UsageRecord {
        uint64_t seq;                       // Sequence number
        std::string time;                   // ISO 8601 timestamp
        std::string action;                 // Action performed
        std::string params;                 // Action parameters (JSON string)
        std::string hash_prev;              // SHA256 hash of previous usage record
        std::string signature;              // Signature using product private key
    };
    std::vector<UsageRecord> usage_chain;   // Usage chain for traceability
    std::string current_signature;          // Signature of entire token using product private key
    
    bool is_valid() const;
    bool is_expired() const;
    std::string to_json() const;
    static Token from_json(const std::string& json);
};

// Token status
enum class TokenStatus {
    NONE,       // No token
    ACTIVE,     // Valid token held
    EXPIRED,    // Token expired
    TRANSFERRED // Token transferred to another device
};

// Token change callback
using TokenChangeCallback = std::function<void(TokenStatus status, const std::optional<Token>& token)>;

// Signature verifier interface
class SignatureVerifier {
public:
    virtual ~SignatureVerifier() = default;
    virtual bool verify(const Token& token, const std::string& public_key) const = 0;
};

// RSA verifier implementation
class RsaVerifier : public SignatureVerifier {
public:
    bool verify(const Token& token, const std::string& public_key) const override;
};

// Ed25519 verifier implementation
class Ed25519Verifier : public SignatureVerifier {
public:
    bool verify(const Token& token, const std::string& public_key) const override;
};

// SM2 verifier implementation
class Sm2Verifier : public SignatureVerifier {
public:
    bool verify(const Token& token, const std::string& public_key) const override;
};

/**
 * TokenManager - Manages software license tokens with multi-algorithm support
 * 
 * Features:
 * - Token generation (coordinator only)
 * - Token validation with multiple algorithms (RSA, Ed25519, SM2)
 * - Token transfer between devices
 * - Signature verification with factory pattern
 * - Smart caching of verification results
 * 
 * Thread-safe operations
 * 
 * Algorithm Selection Guide:
 * | 场景 | 推荐算法 | 理由 |
 * | :--- | :--- | :--- |
 * | 追求极致性能与现代化 | Ed25519 | 速度最快，签名短，设计安全 |
 * | 最大兼容性（国际传统环境） | RSA | 行业标准，无处不在 |
 * | 中国合规性需求 | SM2 | 满足国密标准，金融政务必备 |
 */
class TokenManager {
public:
    TokenManager();
    ~TokenManager() = default;
    
    // Non-copyable
    TokenManager(const TokenManager&) = delete;
    TokenManager& operator=(const TokenManager&) = delete;
    
    /**
     * Generate new token (coordinator only)
     * @param holder_device_id Device to receive token
     * @param license_code License code
     * @param validity_hours Token validity in hours
     * @param private_key Private key for signing
     * @param algorithm Signing algorithm to use
     * @return Generated token
     */
    Token generate_token(const std::string& holder_device_id,
                       const std::string& license_code,
                       int validity_hours,
                       const std::string& private_key,
                       SigningAlgorithm algorithm = SigningAlgorithm::RSA);
    
    /**
     * Set current token
     * @param token Token to set
     * @param public_key Public key for verification
     * @return true if token is valid and accepted
     */
    bool set_token(const Token& token, const std::string& public_key);
    
    /**
     * Get current token
     * @return Current token if exists
     */
    std::optional<Token> get_current_token() const;
    
    /**
     * Get token status
     */
    TokenStatus get_status() const;
    
    /**
     * Verify token signature with smart caching
     * @param token Token to verify
     * @param public_key Public key for verification
     * @return true if signature is valid
     */
    bool verify_token(const Token& token, const std::string& public_key) const;
    
    /**
     * Verify token using trust chain model
     * First verifies the root signature of the license public key,
     * then verifies the token signature using the verified license public key
     * @param token Token to verify
     * @param root_public_key Root public key for verifying license public key signature
     * @return true if both signatures are valid
     */
    bool verify_token_trust_chain(const Token& token) const;
    
    /**
     * Request token transfer to another device
     * Called by current holder
     * @param target_device_id Target device ID
     * @return Transfer request data
     */
    std::string request_transfer(const std::string& target_device_id);
    
    /**
     * Accept incoming token transfer
     * Called by receiving device
     * @param transfer_data Transfer request data
     * @param public_key Public key for verification
     * @return true if transfer accepted
     */
    bool accept_transfer(const std::string& transfer_data, const std::string& public_key);
    
    /**
     * Invalidate current token
     * Called when transferring to another device
     */
    void invalidate_token();
    
    /**
     * Set callback for token changes
     */
    void set_token_callback(TokenChangeCallback callback);
    
    /**
     * Check and update token expiration status
     */
    void check_expiration();
    
    /**
     * Set public key for a specific algorithm
     * @param algorithm The signing algorithm
     * @param public_key The public key in PEM format
     */
    void set_public_key(SigningAlgorithm algorithm, const std::string& public_key);
    
    /**
     * Get verifier for a specific algorithm
     * @param algorithm The signing algorithm
     * @return Pointer to the verifier
     */
    std::shared_ptr<SignatureVerifier> get_verifier(SigningAlgorithm algorithm) const;
    
    /**
     * Generate a unique token ID
     * @return Token ID string
     */
    std::string generate_token_id() const;

    /**
     * Create signature data from token
     * @param token The token to create signature data for
     * @return Signature data string
     */
    std::string create_signature_data(const Token& token) const;

    /**
     * Create signature data for state chain
     * @param token The token to create state signature data for
     * @return State signature data string
     */
    std::string create_state_signature_data(const Token& token) const;

    /**
     * Migrate token state
     * @param current_token Current valid token
     * @param new_payload New business data payload
     * @param license_private_key License private key for signing
     * @return New token with updated state
     */
    Token migrate_token_state(const Token& current_token, const std::string& new_payload, const std::string& license_private_key);

    /**
     * Verify token state chain
     * @param current_token Current token to verify
     * @param stored_chain Stored chain of previous tokens
     * @return true if state chain is valid
     */
    bool verify_token_state_chain(const Token& current_token, const std::vector<Token>& stored_chain);

    /**
     * Handle incoming network message
     * @param msg Network message
     * @param from_address Sender address
     * @param public_key Public key for verification
     */
    void handle_message(const NetworkMessage& msg, const std::string& from_address, const std::string& public_key);

private:
    // Calculate cache TTL based on token expiration time
    int calculate_cache_ttl(uint64_t expire_time) const;
    
    /**
     * Notify token change to callback
     * @param status New token status
     */
    void notify_token_change(TokenStatus status);
    
    std::optional<Token> current_token_;
    mutable std::mutex token_mutex_;
    
    TokenChangeCallback token_callback_;
    mutable std::mutex callback_mutex_;
    
    // Verifiers for different algorithms
    std::shared_ptr<SignatureVerifier> rsa_verifier_;
    std::shared_ptr<SignatureVerifier> ed25519_verifier_;
    std::shared_ptr<SignatureVerifier> sm2_verifier_;
    
    // Public keys for different algorithms
    std::unordered_map<SigningAlgorithm, std::string> public_keys_;
    mutable std::mutex keys_mutex_;
    
    // Verification cache: token_id -> (verification_result, expiration_time)
    mutable std::unordered_map<std::string, std::pair<bool, std::chrono::steady_clock::time_point>> verification_cache_;
    mutable std::mutex cache_mutex_;
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_TOKEN_MANAGER_HPP