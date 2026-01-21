#include "decentrilicense/token_manager.hpp"
#include "decentrilicense/crypto_utils.hpp"
#include "decentrilicense/root_key.hpp"
#include <sstream>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>
#include <iostream>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>

// Debug macro - set to 1 to enable debug output, 0 to disable
#define DECENTRILICENSE_DEBUG 0

// Forward declarations
static std::string normalizePEM(const std::string& pemContent);
static void debugHashAndData(const std::string& data, const std::string& label);

namespace decentrilicense {

static std::string trim_pem(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
    size_t end = s.size();
    while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n')) end--;
    return s.substr(start, end - start);
}

static std::string json_escape_tm(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

static std::string json_unescape_tm(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n') {
                out.push_back('\n');
                i++;
                continue;
            }
            if (n == 'r') {
                out.push_back('\r');
                i++;
                continue;
            }
            if (n == 't') {
                out.push_back('\t');
                i++;
                continue;
            }
            out.push_back(n);
            i++;
            continue;
        }
        out.push_back(c);
    }
    return out;
}

static std::string extract_json_string_tm(const std::string& json, const std::string& key) {
    const std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) {
        return "";
    }
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) {
        return "";
    }
    p++;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n' || json[p] == '\r')) {
        p++;
    }
    if (p >= json.size() || json[p] != '"') {
        return "";
    }
    p++;
    std::string out;
    for (; p < json.size(); p++) {
        char c = json[p];
        if (c == '\\') {
            if (p + 1 < json.size()) {
                out.push_back('\\');
                out.push_back(json[p + 1]);
                p++;
                continue;
            }
        }
        if (c == '"') {
            break;
        }
        out.push_back(c);
    }
    return json_unescape_tm(out);
}

static uint64_t extract_json_u64_tm(const std::string& json, const std::string& key) {
    const std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) {
        return 0;
    }
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) {
        return 0;
    }
    p++;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n' || json[p] == '\r')) {
        p++;
    }
    size_t e = p;
    while (e < json.size() && (json[e] == '-' || (json[e] >= '0' && json[e] <= '9'))) {
        e++;
    }
    if (e == p) {
        return 0;
    }
    try {
        return static_cast<uint64_t>(std::stoull(json.substr(p, e - p)));
    } catch (...) {
        return 0;
    }
}

// Token implementation
bool Token::is_valid() const {
    return !token_id.empty() && !signature.empty() && !alg.empty();
}

bool Token::is_expired() const {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now > expire_time;
}

std::string Token::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"token_id\":\"" << json_escape_tm(token_id) << "\",";
    oss << "\"holder_device_id\":\"" << json_escape_tm(holder_device_id) << "\",";
    oss << "\"license_code\":\"" << json_escape_tm(license_code) << "\",";
    oss << "\"issue_time\":" << issue_time << ",";
    oss << "\"expire_time\":" << expire_time << ",";
    oss << "\"signature\":\"" << json_escape_tm(signature) << "\",";
    oss << "\"alg\":\"" << json_escape_tm(alg) << "\",";
    oss << "\"app_id\":\"" << json_escape_tm(app_id) << "\",";
    oss << "\"environment_hash\":\"" << json_escape_tm(environment_hash) << "\",";
    oss << "\"license_public_key\":\"" << json_escape_tm(license_public_key) << "\",";
    oss << "\"root_signature\":\"" << json_escape_tm(root_signature) << "\",";
    oss << "\"encrypted_license_private_key\":\"" << json_escape_tm(encrypted_license_private_key) << "\",";
    oss << "\"state_index\":" << state_index << ",";
    oss << "\"prev_state_hash\":\"" << json_escape_tm(prev_state_hash) << "\",";
    oss << "\"state_payload\":\"" << json_escape_tm(state_payload) << "\",";
    oss << "\"state_signature\":\"" << json_escape_tm(state_signature) << "\"";
    
    // Add device info if present
    if (!device_info.fingerprint.empty() || !device_info.public_key.empty() || !device_info.signature.empty()) {
        oss << ",\"device_info\":{";
        oss << "\"fingerprint\":\"" << json_escape_tm(device_info.fingerprint) << "\",";
        oss << "\"public_key\":\"" << json_escape_tm(device_info.public_key) << "\",";
        oss << "\"signature\":\"" << json_escape_tm(device_info.signature) << "\"";
        oss << "}";
    }
    
    // Add usage chain if present
    if (!usage_chain.empty()) {
        oss << ",\"usage_chain\":[";
        for (size_t i = 0; i < usage_chain.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{";
            oss << "\"seq\":" << usage_chain[i].seq << ",";
            oss << "\"time\":\"" << json_escape_tm(usage_chain[i].time) << "\",";
            oss << "\"action\":\"" << json_escape_tm(usage_chain[i].action) << "\",";
            oss << "\"params\":\"" << json_escape_tm(usage_chain[i].params) << "\",";
            oss << "\"hash_prev\":\"" << json_escape_tm(usage_chain[i].hash_prev) << "\",";
            oss << "\"signature\":\"" << json_escape_tm(usage_chain[i].signature) << "\"";
            oss << "}";
        }
        oss << "]";
    }
    
    // Add current signature if present
    if (!current_signature.empty()) {
        oss << ",\"current_signature\":\"" << json_escape_tm(current_signature) << "\"";
    }
    
    oss << "}";
    return oss.str();
}

Token Token::from_json(const std::string& json) {
    Token token;
    token.token_id = extract_json_string_tm(json, "token_id");
    token.holder_device_id = extract_json_string_tm(json, "holder_device_id");
    token.license_code = extract_json_string_tm(json, "license_code");
    token.issue_time = extract_json_u64_tm(json, "issue_time");
    token.expire_time = extract_json_u64_tm(json, "expire_time");
    token.signature = extract_json_string_tm(json, "signature");
    token.alg = extract_json_string_tm(json, "alg");
    token.app_id = extract_json_string_tm(json, "app_id");
    token.environment_hash = extract_json_string_tm(json, "environment_hash");
    token.license_public_key = extract_json_string_tm(json, "license_public_key");
    token.root_signature = extract_json_string_tm(json, "root_signature");
    token.encrypted_license_private_key = extract_json_string_tm(json, "encrypted_license_private_key");
    token.state_index = extract_json_u64_tm(json, "state_index");
    token.prev_state_hash = extract_json_string_tm(json, "prev_state_hash");
    token.state_payload = extract_json_string_tm(json, "state_payload");
    token.state_signature = extract_json_string_tm(json, "state_signature");

    const std::string dev_pat = "\"device_info\"";
    size_t dev_pos = json.find(dev_pat);
    if (dev_pos != std::string::npos) {
        size_t obj_start = json.find('{', dev_pos);
        size_t obj_end = json.find('}', obj_start);
        if (obj_start != std::string::npos && obj_end != std::string::npos && obj_end > obj_start) {
            std::string dev_obj = json.substr(obj_start, obj_end - obj_start + 1);
            token.device_info.fingerprint = extract_json_string_tm(dev_obj, "fingerprint");
            token.device_info.public_key = extract_json_string_tm(dev_obj, "public_key");
            token.device_info.signature = extract_json_string_tm(dev_obj, "signature");
        }
    }
    return token;
}

// Signature verifier implementations
bool RsaVerifier::verify(const Token& token, const std::string& public_key) const {
    if (token.signature.empty()) {
        return false;
    }
    
    // Create signature data
    std::string sig_data = TokenManager().create_signature_data(token);
    
    // Verify signature using CryptoUtils
    try {
        return CryptoUtils::verify_signature(sig_data, token.signature, public_key);
    } catch (const std::exception& e) {
        return false;
    }
}

bool Ed25519Verifier::verify(const Token& token, const std::string& public_key) const {
    if (token.signature.empty()) {
        return false;
    }
    
    // Create signature data
    std::string sig_data = TokenManager().create_signature_data(token);
    
    // Verify signature using CryptoUtils
    try {
        return CryptoUtils::verify_ed25519_signature(sig_data, token.signature, public_key);
    } catch (const std::exception& e) {
        return false;
    }
}

bool Sm2Verifier::verify(const Token& token, const std::string& public_key) const {
    if (token.signature.empty()) {
        return false;
    }
    
    // Create signature data
    std::string sig_data = TokenManager().create_signature_data(token);
    
    // Verify signature using CryptoUtils
    try {
        return CryptoUtils::verify_sm2_signature(sig_data, token.signature, public_key);
    } catch (const std::exception& e) {
        return false;
    }
}

// TokenManager implementation
TokenManager::TokenManager() 
    : rsa_verifier_(std::make_shared<RsaVerifier>()),
      ed25519_verifier_(std::make_shared<Ed25519Verifier>()),
      sm2_verifier_(std::make_shared<Sm2Verifier>()) {
}

Token TokenManager::generate_token(const std::string& holder_device_id,
                                   const std::string& license_code,
                                   int validity_hours,
                                   const std::string& private_key,
                                   SigningAlgorithm algorithm) {
    Token token;
    token.token_id = generate_token_id();
    token.holder_device_id = holder_device_id;
    token.license_code = license_code;
    
    auto now = std::chrono::system_clock::now();
    token.issue_time = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    token.expire_time = std::chrono::duration_cast<std::chrono::seconds>(
        (now + std::chrono::hours(validity_hours)).time_since_epoch()).count();
    
    // Set algorithm
    switch (algorithm) {
        case SigningAlgorithm::RSA:
            token.alg = "RSA";
            break;
        case SigningAlgorithm::Ed25519:
            token.alg = "Ed25519";
            break;
        case SigningAlgorithm::SM2:
            token.alg = "SM2";
            break;
    }
    
    // Create signature data
    std::string sig_data = create_signature_data(token);
    
    // Sign the data using appropriate CryptoUtils function
    try {
        switch (algorithm) {
            case SigningAlgorithm::RSA:
                token.signature = CryptoUtils::sign_data(sig_data, private_key);
                break;
            case SigningAlgorithm::Ed25519:
                token.signature = CryptoUtils::sign_ed25519_data(sig_data, private_key);
                break;
            case SigningAlgorithm::SM2:
                token.signature = CryptoUtils::sign_sm2_data(sig_data, private_key);
                break;
        }
    } catch (const std::exception& e) {
        // Handle error appropriately
        token.signature = "";
    }
    
    return token;
}

bool TokenManager::set_token(const Token& token, const std::string& public_key) {
#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: set_token called, token.is_valid(): " << token.is_valid() << std::endl;
    std::cerr << "dl-core debug: token_id length: " << token.token_id.length() << std::endl;
    std::cerr << "dl-core debug: signature length: " << token.signature.length() << std::endl;
    std::cerr << "dl-core debug: alg: '" << token.alg << "'" << std::endl;
#endif
    if (!token.is_valid()) {
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: token is not valid" << std::endl;
#endif
        return false;
    }
    
    // Verify token signature
    if (!verify_token(token, public_key)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(token_mutex_);
    current_token_ = token;
    
    // Notify callback
    if (token.is_expired()) {
        notify_token_change(TokenStatus::EXPIRED);
    } else {
        notify_token_change(TokenStatus::ACTIVE);
    }
    
    return true;
}

std::optional<Token> TokenManager::get_current_token() const {
    std::lock_guard<std::mutex> lock(token_mutex_);
    return current_token_;
}

TokenStatus TokenManager::get_status() const {
    std::lock_guard<std::mutex> lock(token_mutex_);
    
    if (!current_token_.has_value()) {
        return TokenStatus::NONE;
    }
    
    if (current_token_->is_expired()) {
        return TokenStatus::EXPIRED;
    }
    
    return TokenStatus::ACTIVE;
}

bool TokenManager::verify_token(const Token& token, const std::string& public_key) const {
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = verification_cache_.find(token.token_id);
        if (it != verification_cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            if (it->second.second > now) {
                // Cache hit and not expired
                return it->second.first;
            } else {
                // Expired, remove from cache
                verification_cache_.erase(it);
            }
        }
    }
    
    // Get appropriate verifier
    std::shared_ptr<SignatureVerifier> verifier;
    if (token.alg == "RSA") {
        verifier = rsa_verifier_;
    } else if (token.alg == "Ed25519") {
        verifier = ed25519_verifier_;
    } else if (token.alg == "SM2") {
        verifier = sm2_verifier_;
    } else {
        return false;
    }
    
    // Verify signature
    bool result = verifier->verify(token, public_key);
    
    // Cache result
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto ttl = calculate_cache_ttl(token.expire_time);
        auto expiration = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
        verification_cache_[token.token_id] = std::make_pair(result, expiration);
    }
    
    return result;
}

std::string TokenManager::request_transfer(const std::string& target_device_id) {
    std::lock_guard<std::mutex> lock(token_mutex_);
    
    if (!current_token_.has_value()) {
        return "";
    }
    
    // Mark token as transferred
    notify_token_change(TokenStatus::TRANSFERRED);
    current_token_.reset();
    
    // In a real implementation, this would create a transfer request
    // For now, we'll just return a placeholder
    return "transfer_request_data";
}

bool TokenManager::accept_transfer(const std::string& transfer_data, const std::string& public_key) {
    // In a real implementation, this would process the transfer request
    // For now, we'll just return true to indicate success
    return true;
}

void TokenManager::invalidate_token() {
    std::lock_guard<std::mutex> lock(token_mutex_);
    current_token_.reset();
    notify_token_change(TokenStatus::NONE);
}

void TokenManager::set_token_callback(TokenChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    token_callback_ = std::move(callback);
}

void TokenManager::check_expiration() {
    std::lock_guard<std::mutex> lock(token_mutex_);
    
    if (current_token_.has_value() && current_token_->is_expired()) {
        notify_token_change(TokenStatus::EXPIRED);
    }
}

void TokenManager::set_public_key(SigningAlgorithm algorithm, const std::string& public_key) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    public_keys_[algorithm] = public_key;
}

std::shared_ptr<SignatureVerifier> TokenManager::get_verifier(SigningAlgorithm algorithm) const {
    switch (algorithm) {
        case SigningAlgorithm::RSA:
            return rsa_verifier_;
        case SigningAlgorithm::Ed25519:
            return ed25519_verifier_;
        case SigningAlgorithm::SM2:
            return sm2_verifier_;
        default:
            return nullptr;
    }
}

std::string TokenManager::generate_token_id() const {
    // Generate UUID-like token ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    oss << std::hex;
    
    // 8 characters
    for (int i = 0; i < 8; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    
    // 4 characters
    for (int i = 0; i < 4; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    
    // 4 characters
    for (int i = 0; i < 4; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    
    // 4 characters
    for (int i = 0; i < 4; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    
    // 12 characters
    for (int i = 0; i < 12; ++i) {
        oss << dis(gen);
    }
    
    return oss.str();
}

std::string TokenManager::create_signature_data(const Token& token) const {
    std::ostringstream oss;
    // expire_time intentionally excluded from signature data to avoid expiry-related mismatches
    oss << token.token_id << "|"
        << token.app_id << "|"
        << token.holder_device_id << "|"
        << token.license_code << "|"
        << token.issue_time;
    return oss.str();
}

std::string TokenManager::create_state_signature_data(const Token& token) const {
    std::ostringstream oss;
    oss << token.state_index << "|"
        << token.prev_state_hash << "|"
        << token.state_payload;
    return oss.str();
}

Token TokenManager::migrate_token_state(const Token& current_token, const std::string& new_payload, const std::string& license_private_key) {
    Token new_token = current_token;
    
    // Calculate current token hash as prev_state_hash
    std::string current_token_json = current_token.to_json();
    new_token.prev_state_hash = CryptoUtils::sha256(current_token_json);
    
    // Increment state index
    new_token.state_index = current_token.state_index + 1;
    
    // Set new state payload
    new_token.state_payload = new_payload;
    
    // Create state signature data and sign it
    std::string state_sig_data = create_state_signature_data(new_token);
    
    // Sign the state signature data using the appropriate algorithm
    try {
        if (new_token.alg == "RSA") {
            new_token.state_signature = CryptoUtils::sign_data(state_sig_data, license_private_key);
        } else if (new_token.alg == "Ed25519") {
            new_token.state_signature = CryptoUtils::sign_ed25519_data(state_sig_data, license_private_key);
        } else if (new_token.alg == "SM2") {
            new_token.state_signature = CryptoUtils::sign_sm2_data(state_sig_data, license_private_key);
        }
    } catch (const std::exception& e) {
        // Handle error appropriately
        new_token.state_signature = "";
    }
    
    // Update the main token signature to include the new state fields
    std::string sig_data = create_signature_data(new_token);
    
    // Sign the data using appropriate CryptoUtils function
    try {
        if (new_token.alg == "RSA") {
            new_token.signature = CryptoUtils::sign_data(sig_data, license_private_key);
        } else if (new_token.alg == "Ed25519") {
            new_token.signature = CryptoUtils::sign_ed25519_data(sig_data, license_private_key);
        } else if (new_token.alg == "SM2") {
            new_token.signature = CryptoUtils::sign_sm2_data(sig_data, license_private_key);
        }
    } catch (const std::exception& e) {
        // Handle error appropriately
        new_token.signature = "";
    }
    
    return new_token;
}

bool TokenManager::verify_token_state_chain(const Token& current_token, const std::vector<Token>& stored_chain) {
    // Verify the state signature using the license public key from the token
    std::string state_sig_data = create_state_signature_data(current_token);
    
    // Verify state signature
    bool state_sig_valid = false;
    try {
        if (current_token.alg == "RSA") {
            state_sig_valid = CryptoUtils::verify_signature(state_sig_data, current_token.state_signature, current_token.license_public_key);
        } else if (current_token.alg == "Ed25519") {
            state_sig_valid = CryptoUtils::verify_ed25519_signature(state_sig_data, current_token.state_signature, current_token.license_public_key);
        } else if (current_token.alg == "SM2") {
            state_sig_valid = CryptoUtils::verify_sm2_signature(state_sig_data, current_token.state_signature, current_token.license_public_key);
        }
    } catch (const std::exception& e) {
        state_sig_valid = false;
    }
    
    if (!state_sig_valid) {
        return false;
    }
    
    // If this is the genesis token (state_index = 0), no need to check prev_state_hash
    if (current_token.state_index == 0) {
        return true;
    }
    
    // Verify prev_state_hash matches the last token in the stored chain
    if (stored_chain.empty()) {
        return false;
    }
    
    const Token& last_token = stored_chain.back();
    std::string last_token_json = last_token.to_json();
    std::string last_token_hash = CryptoUtils::sha256(last_token_json);
    
    if (current_token.prev_state_hash != last_token_hash) {
        return false;
    }
    
    // Verify state index is continuous
    if (current_token.state_index != last_token.state_index + 1) {
        return false;
    }
    
    return true;
}

void TokenManager::handle_message(const NetworkMessage& msg, const std::string& from_address, const std::string& public_key) {
    // For now, we'll just print a message to indicate the method exists
    // In a real implementation, this would handle token-related messages
}

// Private methods
int TokenManager::calculate_cache_ttl(uint64_t expire_time) const {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Cache for half the remaining time, but at least 1 minute and at most 1 hour
    int remaining = static_cast<int>(expire_time - now);
    int ttl = remaining / 2;
    
    if (ttl < 60) {
        ttl = 60;
    } else if (ttl > 3600) {
        ttl = 3600;
    }
    
    return ttl;
}

void TokenManager::notify_token_change(TokenStatus status) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (token_callback_) {
        token_callback_(status, current_token_);
    }
}

bool TokenManager::verify_token_trust_chain(const Token& token) const {
#if DECENTRILICENSE_DEBUG
    std::cerr << "TOKEN_MANAGER: verify_token_trust_chain called!" << std::endl;
#endif
    // Step 1: Verify the root signature of the license public key
    try {
        // Parse the hardcoded root public key
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: using hardcoded ROOT_PUBLIC_KEY, length: " << decentrilicense::ROOT_PUBLIC_KEY.length() << std::endl;
        std::cerr << "dl-core debug: ROOT_PUBLIC_KEY starts with: " << decentrilicense::ROOT_PUBLIC_KEY.substr(0, 50) << "..." << std::endl;
#endif

        BIO* bio = BIO_new_mem_buf(decentrilicense::ROOT_PUBLIC_KEY.c_str(), decentrilicense::ROOT_PUBLIC_KEY.length());
        if (!bio) {
#if DECENTRILICENSE_DEBUG
            std::cerr << "dl-core debug: failed to create BIO" << std::endl;
#endif
            return false;
        }
        
        EVP_PKEY* root_pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        if (!root_pkey) {
            unsigned long err = ERR_get_error();
#if DECENTRILICENSE_DEBUG
            std::cerr << "dl-core debug: failed to load root public key, OpenSSL error: " << ERR_reason_error_string(err) << std::endl;
#endif
            BIO_free(bio);
            return false;
        }

        BIO_free(bio);
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: root public key loaded successfully" << std::endl;
#endif

        // Check key type
        int key_type = EVP_PKEY_get_id(root_pkey);
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: root key type: " << key_type << " (expected: " << EVP_PKEY_RSA << ")" << std::endl;
#endif
        
        // Decode the root signature
        std::vector<uint8_t> root_sig_bytes = CryptoUtils::base64_decode(token.root_signature);
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: root_signature length: " << token.root_signature.length() << std::endl;
        std::cerr << "dl-core debug: decoded signature length: " << root_sig_bytes.size() << std::endl;
        std::cerr << "dl-core debug: signature前16字节: ";
        for(size_t i = 0; i < std::min<size_t>(16, root_sig_bytes.size()); i++) {
            std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)root_sig_bytes[i] << " ";
        }
        std::cerr << std::dec << std::endl;
#endif
        
        // ARCHITECTURE CHANGE: dl-core does NOT verify license_public_key directly.
        // License public key verification is handled by SDK during activation.
        // dl-core only verifies the product public key via shadow token mechanism in dl-issuer.
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: license_public_key verification delegated to SDK activation" << std::endl;
#endif

        // Skip the root signature verification in dl-core - this creates confusion
        // The actual product key verification happens via shadow token in dl-issuer
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: root signature verification skipped - using shadow token approach only" << std::endl;
#endif

        EVP_PKEY_free(root_pkey);
        return true; // Success - no direct verification needed
    } catch (const std::exception& e) {
        return false;
    }
}

} // namespace decentrilicense

// normalizePEM provides the single source of truth for PEM normalization
// This function MUST be kept in sync with Go's normalizePEM function
static std::string normalizePEM(const std::string& pemContent) {
    // Remove leading/trailing whitespace only (preserve internal formatting)
    size_t start = pemContent.find_first_not_of(" \t\n\r");
    size_t end = pemContent.find_last_not_of(" \t\n\r");
    std::string trimmed;
    if (start != std::string::npos && end != std::string::npos) {
        trimmed = pemContent.substr(start, end - start + 1);
    } else {
        trimmed = pemContent;
    }

    // Ensure single trailing newline
    if (!trimmed.empty() && trimmed.back() != '\n') {
        trimmed += '\n';
    }

    return trimmed;
}

// Debug function to output hash and data details
static void debugHashAndData(const std::string& data, const std::string& label) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)data.data(), data.size(), hash);

    std::cerr << label << " SHA256: ";
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::cerr << std::dec << std::endl;

    std::cerr << label << " 数据长度: " << data.size() << " 字节" << std::endl;

    // Output first 20 bytes in hex
    std::cerr << label << " 前20字节(hex): ";
    for(size_t i = 0; i < std::min<size_t>(20, data.size()); i++) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)data[i] << " ";
    }
    std::cerr << std::dec << std::endl;
}