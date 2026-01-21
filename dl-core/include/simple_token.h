#ifndef SIMPLE_TOKEN_H
#define SIMPLE_TOKEN_H

#include <string>
#include <cstdint>
#include <sstream>

namespace decentrilicense {

// Simplified Token structure with state chain fields
struct Token {
    std::string token_id;                   // UUID
    std::string holder_device_id;           // Current holder
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
    
    bool is_valid() const {
        return !token_id.empty() && !holder_device_id.empty() && !signature.empty() && !alg.empty();
    }
    
    bool is_expired() const {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return now > expire_time;
    }
    
    std::string to_json() const {
        std::ostringstream oss;
        oss << "{"
            << "\"token_id\":\"" << token_id << "\",";
        oss << "\"holder_device_id\":\"" << holder_device_id << "\",";
        oss << "\"license_code\":\"" << license_code << "\",";
        oss << "\"issue_time\":" << issue_time << ",";
        oss << "\"expire_time\":" << expire_time << ",";
        oss << "\"signature\":\"" << signature << "\",";
        oss << "\"alg\":\"" << alg << "\",";
        oss << "\"app_id\":\"" << app_id << "\",";
        oss << "\"environment_hash\":\"" << environment_hash << "\",";
        oss << "\"license_public_key\":\"" << license_public_key << "\",";
        oss << "\"root_signature\":\"" << root_signature << "\",";
        oss << "\"encrypted_license_private_key\":\"" << encrypted_license_private_key << "\",";
        oss << "\"state_index\":" << state_index << ",";
        oss << "\"prev_state_hash\":\"" << prev_state_hash << "\",";
        oss << "\"state_payload\":\"" << state_payload << "\",";
        oss << "\"state_signature\":\"" << state_signature << "\"";
        oss << "}";
        return oss.str();
    }
    
    // 从JSON字符串创建Token对象的静态方法
    static Token from_json(const std::string& json_str);
};

} // namespace decentrilicense

#endif // SIMPLE_TOKEN_H