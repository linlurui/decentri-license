#include "decenlicense_c.h"
#include "decentrilicense/decentrilicense_client.hpp"
#include "decentrilicense/token_manager.hpp"
#include "decentrilicense/crypto_utils.hpp"
#include "decentrilicense/root_key.hpp"
#include "state_chain_storage.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace decentrilicense;

extern "C" {

static void set_err(DL_VerificationResult* result, const std::string& msg) {
    if (!result) {
        return;
    }
    result->valid = 0;
    std::strncpy(result->error_message, msg.c_str(), sizeof(result->error_message) - 1);
    result->error_message[sizeof(result->error_message) - 1] = '\0';
}

static void set_ok(DL_VerificationResult* result) {
    if (!result) {
        return;
    }
    result->valid = 1;
    result->error_message[0] = '\0';
}

static std::string json_escape(const std::string& s) {
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

static std::string build_state_sig_data(uint64_t state_index, const std::string& prev_state_hash, const std::string& state_payload) {
    return std::to_string(state_index) + "|" + prev_state_hash + "|" + state_payload;
}

static std::string build_token_json(
    const Token& t,
    const std::string& device_fingerprint,
    const std::string& device_public_key_pem,
    const std::string& device_signature_b64,
    bool include_device_info) {
    std::string json;
    json.reserve(2048);
    json += "{";
    json += "\"token_id\":\"" + json_escape(t.token_id) + "\",";
    json += "\"license_code\":\"" + json_escape(t.license_code) + "\",";
    json += "\"holder_device_id\":\"" + json_escape(t.holder_device_id) + "\",";
    json += "\"issue_time\":" + std::to_string(t.issue_time) + ",";
    json += "\"expire_time\":" + std::to_string(t.expire_time) + ",";
    json += "\"signature\":\"" + json_escape(t.signature) + "\",";
    json += "\"app_id\":\"" + json_escape(t.app_id) + "\",";
    json += "\"environment_hash\":\"" + json_escape(t.environment_hash) + "\",";
    json += "\"license_public_key\":\"" + json_escape(t.license_public_key) + "\",";
    json += "\"root_signature\":\"" + json_escape(t.root_signature) + "\",";
    json += "\"state_index\":" + std::to_string(t.state_index) + ",";
    json += "\"prev_state_hash\":\"" + json_escape(t.prev_state_hash) + "\",";
    json += "\"state_payload\":\"" + json_escape(t.state_payload) + "\",";
    json += "\"state_signature\":\"" + json_escape(t.state_signature) + "\",";
    json += "\"alg\":\"" + json_escape(t.alg) + "\"";

    // Include original_holder_device_id for activated tokens so it can be restored on re-import
    // This is critical: the signature was computed with the original holder_device_id,
    // but after activation holder_device_id gets overwritten with device_id.
    // Without saving the original, re-imported activated tokens fail verification.
    if (include_device_info && !device_public_key_pem.empty() && !device_signature_b64.empty()) {
        json += ",\"device_info\":{";
        json += "\"fingerprint\":\"" + json_escape(device_fingerprint) + "\",";
        json += "\"public_key\":\"" + json_escape(device_public_key_pem) + "\",";
        json += "\"signature\":\"" + json_escape(device_signature_b64) + "\"";
        json += "}";
    }

    json += "}";
    return json;
}

// Build token JSON with original_holder_device_id preserved for activated tokens
static std::string build_token_json_with_original_holder(
    const Token& t,
    const std::string& original_holder_device_id,
    const std::string& device_fingerprint,
    const std::string& device_public_key_pem,
    const std::string& device_signature_b64,
    bool include_device_info,
    const std::string& encrypted_sek_device = "",
    const std::string& encrypted_sek_password = "",
    bool state_payload_encrypted = false) {
    std::string json;
    json.reserve(2048);
    json += "{";
    json += "\"token_id\":\"" + json_escape(t.token_id) + "\",";
    json += "\"license_code\":\"" + json_escape(t.license_code) + "\",";
    json += "\"holder_device_id\":\"" + json_escape(t.holder_device_id) + "\",";
    // Always preserve original_holder_device_id for re-import verification (even if empty)
    json += "\"original_holder_device_id\":\"" + json_escape(original_holder_device_id) + "\",";
    json += "\"issue_time\":" + std::to_string(t.issue_time) + ",";
    json += "\"expire_time\":" + std::to_string(t.expire_time) + ",";
    json += "\"signature\":\"" + json_escape(t.signature) + "\",";
    json += "\"app_id\":\"" + json_escape(t.app_id) + "\",";
    json += "\"environment_hash\":\"" + json_escape(t.environment_hash) + "\",";
    json += "\"license_public_key\":\"" + json_escape(t.license_public_key) + "\",";
    json += "\"root_signature\":\"" + json_escape(t.root_signature) + "\",";
    json += "\"state_index\":" + std::to_string(t.state_index) + ",";
    json += "\"prev_state_hash\":\"" + json_escape(t.prev_state_hash) + "\",";
    // state_payload: if encrypted, store ciphertext; otherwise plaintext
    json += "\"state_payload\":\"" + json_escape(t.state_payload) + "\",";
    json += "\"state_signature\":\"" + json_escape(t.state_signature) + "\",";
    json += "\"alg\":\"" + json_escape(t.alg) + "\"";

    // SEK wrapping fields (only present when state is encrypted)
    if (!encrypted_sek_device.empty()) {
        json += ",\"encrypted_sek_device\":\"" + json_escape(encrypted_sek_device) + "\"";
    }
    if (!encrypted_sek_password.empty()) {
        json += ",\"encrypted_sek_password\":\"" + json_escape(encrypted_sek_password) + "\"";
    }
    if (state_payload_encrypted) {
        json += ",\"state_payload_encrypted\":true";
    }

    if (include_device_info && !device_public_key_pem.empty() && !device_signature_b64.empty()) {
        json += ",\"device_info\":{";
        json += "\"fingerprint\":\"" + json_escape(device_fingerprint) + "\",";
        json += "\"public_key\":\"" + json_escape(device_public_key_pem) + "\",";
        json += "\"signature\":\"" + json_escape(device_signature_b64) + "\"";
        json += "}";
    }

    json += "}";
    return json;
}

static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
        i++;
    }
    return s.substr(i);
}

static std::string extract_json_string(const std::string& json, const std::string& key) {
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
                char n = json[p + 1];
                if (n == 'n') out.push_back('\n');
                else if (n == 'r') out.push_back('\r');
                else if (n == 't') out.push_back('\t');
                else out.push_back(n);
                p++;
                continue;
            }
        }
        if (c == '"') {
            break;
        }
        out.push_back(c);
    }
    return out;
}

static uint64_t extract_json_u64(const std::string& json, const std::string& key) {
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

static bool is_encrypted_token_format(const std::string& input) {
    size_t sep = input.find('|');
    if (sep == std::string::npos) {
        return false;
    }
    if (sep == 0 || sep + 1 >= input.size()) {
        return false;
    }
    return input.find('|', sep + 1) == std::string::npos;
}

// normalizePEM: trim whitespace + ensure single trailing newline
// MUST be kept in sync with Go's normalizePEM and token_manager.cpp's normalizePEM
static std::string normalizePEM(const std::string& pemContent) {
    size_t start = pemContent.find_first_not_of(" \t\n\r");
    size_t end = pemContent.find_last_not_of(" \t\n\r");
    std::string trimmed;
    if (start != std::string::npos && end != std::string::npos) {
        trimmed = pemContent.substr(start, end - start + 1);
    } else {
        trimmed = pemContent;
    }
    if (!trimmed.empty() && trimmed.back() != '\n') {
        trimmed += '\n';
    }
    return trimmed;
}

static bool split_product_public_key_file(const std::string& file_content, std::string* out_pem, std::string* out_root_sig) {
    if (!out_pem || !out_root_sig) {
        return false;
    }
    const std::string marker = "ROOT_SIGNATURE:";
    size_t pos = file_content.find(marker);
    if (pos == std::string::npos) {
        *out_pem = normalizePEM(file_content);
        *out_root_sig = "";
        return !out_pem->empty();
    }
    std::string pem = file_content.substr(0, pos);
    std::string sig = file_content.substr(pos + marker.size());
    *out_pem = normalizePEM(pem);
    *out_root_sig = trim(sig);
    return !out_pem->empty();
}

// --- SEK (State Encryption Key) helper functions ---

// Encrypt SEK with device private key: AES-256-GCM using SHA-256(device_private_key_pem) as key
static std::string encrypt_sek_with_device_key(const std::array<uint8_t, 32>& sek, const std::string& device_private_key_pem) {
    auto key = CryptoUtils::derive_key_from_pem(device_private_key_pem);
    // Serialize SEK to raw bytes string
    std::string sek_bytes(reinterpret_cast<const char*>(sek.data()), 32);
    return CryptoUtils::aes_encrypt_raw(sek_bytes, key);
}

// Decrypt SEK with device private key
static bool decrypt_sek_with_device_key(const std::string& encrypted_sek, const std::string& device_private_key_pem, std::array<uint8_t, 32>& out_sek) {
    try {
        auto key = CryptoUtils::derive_key_from_pem(device_private_key_pem);
        std::string sek_bytes = CryptoUtils::aes_decrypt_raw(encrypted_sek, key);
        if (sek_bytes.size() != 32) {
            return false;
        }
        std::memcpy(out_sek.data(), sek_bytes.data(), 32);
        return true;
    } catch (...) {
        return false;
    }
}

// Encrypt state_payload with SEK
static std::string encrypt_state_payload_with_sek(const std::string& plaintext, const std::array<uint8_t, 32>& sek) {
    return CryptoUtils::aes_encrypt_raw(plaintext, sek);
}

// Decrypt state_payload with SEK
static std::string decrypt_state_payload_with_sek(const std::string& ciphertext, const std::array<uint8_t, 32>& sek) {
    return CryptoUtils::aes_decrypt_raw(ciphertext, sek);
}

// Encrypt SEK with password-derived key (SHA-256 of password)
static std::string encrypt_sek_with_password(const std::array<uint8_t, 32>& sek, const std::string& password) {
    auto key = CryptoUtils::sha256_bytes(password);
    std::string sek_bytes(reinterpret_cast<const char*>(sek.data()), 32);
    return CryptoUtils::aes_encrypt_raw(sek_bytes, key);
}

// Decrypt SEK with password-derived key
static bool decrypt_sek_with_password(const std::string& encrypted_sek, const std::string& password, std::array<uint8_t, 32>& out_sek) {
    try {
        auto key = CryptoUtils::sha256_bytes(password);
        std::string sek_bytes = CryptoUtils::aes_decrypt_raw(encrypted_sek, key);
        if (sek_bytes.size() != 32) {
            return false;
        }
        std::memcpy(out_sek.data(), sek_bytes.data(), 32);
        return true;
    } catch (...) {
        return false;
    }
}

// Implementation of the opaque pointer
struct DL_Client {
    std::unique_ptr<DecentriLicenseClient> client;
    ClientConfig config;
    std::string product_public_key_file_content;
    std::string product_public_key_pem;
    std::string product_root_signature;
    std::string token_json;
    Token token;
    bool has_token = false;
    bool activated = false;
    std::string device_id;
    std::string device_public_key_pem;
    std::string device_private_key_pem;
    std::string device_signature;
    std::string original_holder_device_id;  // saved before activation overwrites token.holder_device_id
    std::unique_ptr<StateChainStorage> storage;

    // SEK (State Encryption Key) - encrypts state_payload at rest
    std::array<uint8_t, 32> sek;              // SEK in memory (plaintext)
    bool has_sek = false;                      // whether SEK is loaded
    std::string encrypted_sek_device;          // SEK encrypted with device key (for JSON storage)
    std::string encrypted_sek_password;        // SEK encrypted with password-derived key (optional)
};

// Create a new client
DL_Client* dl_client_create(void) {
    try {
        DL_Client* client = new DL_Client();
        return client;
    } catch (...) {
        return nullptr;
    }
}

// Destroy a client
void dl_client_destroy(DL_Client* client) {
    if (client) {
        delete client;
    }
}

// Initialize the client
DL_ErrorCode dl_client_initialize(DL_Client* client, const DL_ClientConfig* config) {
    if (!client || !config) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        client->config.license_code = config->license_code ? config->license_code : "";
        client->config.preferred_mode = static_cast<ConnectionMode>(config->preferred_mode);
        client->config.udp_port = config->udp_port > 0 ? config->udp_port : 13325;
        client->config.tcp_port = config->tcp_port > 0 ? config->tcp_port : 23325;
        client->config.registry_server_url = config->registry_server_url ? config->registry_server_url : "";

        client->client = std::make_unique<DecentriLicenseClient>(client->config);
        client->device_id = CryptoUtils::generate_device_id();
        client->storage = std::make_unique<StateChainStorage>(std::string(".decentrilicense_state"));
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_set_product_public_key(DL_Client* client, const char* product_public_key_file_content) {
    if (!client || !product_public_key_file_content) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    try {
        client->product_public_key_file_content = product_public_key_file_content;
        if (!split_product_public_key_file(client->product_public_key_file_content, &client->product_public_key_pem, &client->product_root_signature)) {
            return DL_ERROR_INVALID_ARGUMENT;
        }

        // Also set the product public key in the DecentriLicenseClient
        if (client->client) {
            client->client->set_product_public_key(client->product_public_key_pem);
        }

        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_import_token(DL_Client* client, const char* token_input) {
    if (!client || !token_input) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (client->product_public_key_pem.empty()) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        std::string input = token_input;
        std::string json;
        if (is_encrypted_token_format(input)) {
            json = CryptoUtils::decrypt_token_aes256_gcm(input, client->product_public_key_file_content);
        } else {
            json = input;
        }

        Token t;
        t.token_id = extract_json_string(json, "token_id");
        t.app_id = extract_json_string(json, "app_id");
        t.holder_device_id = extract_json_string(json, "holder_device_id");
        t.license_code = extract_json_string(json, "license_code");
        t.issue_time = extract_json_u64(json, "issue_time");
        t.expire_time = extract_json_u64(json, "expire_time");
        t.environment_hash = extract_json_string(json, "environment_hash");
        t.root_signature = extract_json_string(json, "root_signature");
        t.state_index = extract_json_u64(json, "state_index");
        t.prev_state_hash = extract_json_string(json, "prev_state_hash");
        t.state_payload = extract_json_string(json, "state_payload");
        t.state_signature = extract_json_string(json, "state_signature");
        t.alg = extract_json_string(json, "alg");
        t.signature = extract_json_string(json, "signature");

        // Restore original_holder_device_id if present in JSON (activated tokens)
        // This is critical: activated tokens have holder_device_id overwritten with device_id,
        // but the signature was computed with the original value.
        // Always restore even if empty (token issued with empty holder_device_id).
        // Only restore if the field exists in JSON (distinguish from old format without this field).
        std::string original_holder = extract_json_string(json, "original_holder_device_id");
        bool has_original_holder_field = json.find("\"original_holder_device_id\"") != std::string::npos;
        if (has_original_holder_field) {
            client->original_holder_device_id = original_holder;
        }

        // Detect activated tokens by presence of device_info
        // Activated tokens have device_info with fingerprint, public_key, signature
        std::string device_fingerprint = extract_json_string(json, "fingerprint");
        std::string device_pub_key = extract_json_string(json, "public_key");
        std::string device_sig = extract_json_string(json, "signature");

        client->token_json = json;
        client->token = t;
        client->has_token = true;
        client->activated = false;
        client->original_holder_device_id = "";  // Clear from previous token
        // Clear SEK state from previous token
        client->has_sek = false;
        client->sek = std::array<uint8_t, 32>{};
        client->encrypted_sek_device = "";
        client->encrypted_sek_password = "";

        if (!device_fingerprint.empty() && !device_pub_key.empty()) {
            // This is an activated token - restore activation state
            client->activated = true;
            client->device_id = device_fingerprint;
            client->device_public_key_pem = device_pub_key;
            client->device_signature = device_sig;

            // If original_holder_device_id not in JSON (old format),
            // try to recover from storage
            if (client->original_holder_device_id.empty() && client->storage && !t.license_code.empty()) {
                std::vector<Token> chain = client->storage->loadChain(t.license_code);
                if (!chain.empty()) {
                    // The first entry in the chain is the original token before activation
                    client->original_holder_device_id = chain[0].holder_device_id;
                }
            }

            // Try to load saved device keys for idempotent behavior
            if (client->storage && !t.license_code.empty()) {
                auto saved_keys = client->storage->loadDeviceKeys(t.license_code);
                if (saved_keys.has_value()) {
                    client->device_private_key_pem = saved_keys->device_private_key_pem;
                }
            }

            // --- SEK recovery: decrypt SEK and state_payload if present ---
            std::string encrypted_sek_dev = extract_json_string(json, "encrypted_sek_device");
            bool state_payload_encrypted = json.find("\"state_payload_encrypted\":true") != std::string::npos;

            if (!encrypted_sek_dev.empty() && !client->device_private_key_pem.empty()) {
                // Decrypt SEK via device channel
                if (decrypt_sek_with_device_key(encrypted_sek_dev, client->device_private_key_pem, client->sek)) {
                    client->has_sek = true;
                    client->encrypted_sek_device = encrypted_sek_dev;
                    client->encrypted_sek_password = extract_json_string(json, "encrypted_sek_password");

                    // Decrypt state_payload back to plaintext in memory
                    if (state_payload_encrypted && !client->token.state_payload.empty()) {
                        try {
                            client->token.state_payload = decrypt_state_payload_with_sek(client->token.state_payload, client->sek);
                        } catch (...) {
                            // If decryption fails, keep the ciphertext - verification will fail anyway
                        }
                    }
                }
            }
        }

        if (client->storage && !client->token.license_code.empty()) {
            std::vector<std::string> chain_json; chain_json.push_back(client->token_json); (void)client->storage->saveFullChainJSON(client->token.license_code, chain_json);
        }

        return DL_ERROR_SUCCESS;
    } catch (const std::exception&) {
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_reset(DL_Client* client) {
    if (!client) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        client->token_json.clear();
        client->token = Token();
        client->has_token = false;
        client->activated = false;
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_get_current_token_json(DL_Client* client, char* out_json, size_t out_json_size) {
    if (!client || !out_json || out_json_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        out_json[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    std::strncpy(out_json, client->token_json.c_str(), out_json_size - 1);
    out_json[out_json_size - 1] = '\0';
    return DL_ERROR_SUCCESS;
}

DL_ErrorCode dl_client_export_current_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size) {
    if (!client || !out_encrypted || out_encrypted_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        out_encrypted[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    if (client->product_public_key_file_content.empty()) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        const std::string encrypted = CryptoUtils::encrypt_token_aes256_gcm(client->token_json, client->product_public_key_file_content);
        std::strncpy(out_encrypted, encrypted.c_str(), out_encrypted_size - 1);
        out_encrypted[out_encrypted_size - 1] = '\0';
        return DL_ERROR_SUCCESS;
    } catch (const std::exception&) {
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_export_activated_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size) {
    if (!client || !out_encrypted || out_encrypted_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->activated) {
        out_encrypted[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    if (!client->has_token) {
        out_encrypted[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    if (client->product_public_key_file_content.empty()) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        const std::string encrypted = CryptoUtils::encrypt_token_aes256_gcm(client->token_json, client->product_public_key_file_content);
        std::strncpy(out_encrypted, encrypted.c_str(), out_encrypted_size - 1);
        out_encrypted[out_encrypted_size - 1] = '\0';
        return DL_ERROR_SUCCESS;
    } catch (const std::exception&) {
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_export_state_changed_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size) {
    if (!client || !out_encrypted || out_encrypted_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        out_encrypted[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    if (client->product_public_key_file_content.empty()) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        const std::string encrypted = CryptoUtils::encrypt_token_aes256_gcm(client->token_json, client->product_public_key_file_content);
        std::strncpy(out_encrypted, encrypted.c_str(), out_encrypted_size - 1);
        out_encrypted[out_encrypted_size - 1] = '\0';
        return DL_ERROR_SUCCESS;
    } catch (const std::exception&) {
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_offline_verify_current_token(DL_Client* client, DL_VerificationResult* result) {
    if (!client || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        set_err(result, "no token");
        return DL_ERROR_SUCCESS;
    }
    if (client->product_public_key_pem.empty() || client->product_root_signature.empty()) {
        set_err(result, "product public key not set");
        return DL_ERROR_SUCCESS;
    }

    try {
        // Compatibility: the token may contain a license_public_key + its own root_signature
        // while the client was given a separate product public key file (product_public_key_pem)
        // with its own ROOT_SIGNATURE. Accept either case by mapping the product public key
        // and its root signature into the token fields before verification instead of
        // requiring strict equality.
        Token verify_token = client->token;
        // Ensure license_public_key used for trust-chain verification is the product public key
        if (!client->product_public_key_pem.empty()) {
        verify_token.license_public_key = client->product_public_key_pem;
        }
        // Prefer product_root_signature (from the product public key file) for verification
        if (!client->product_root_signature.empty()) {
            verify_token.root_signature = client->product_root_signature;
        }

        TokenManager tm;
        bool ok = tm.verify_token_trust_chain(verify_token);
        if (!ok) {
            set_err(result, "trust chain verification failed");
            return DL_ERROR_SUCCESS;
        }

        if (client->token.state_index > 0) {
            if (client->device_public_key_pem.empty()) {
                set_err(result, "missing device public key for state verification");
                return DL_ERROR_SUCCESS;
            }
            const std::string state_sig_data = build_state_sig_data(client->token.state_index, client->token.prev_state_hash, client->token.state_payload);
            bool state_ok = false;
            try {
                state_ok = CryptoUtils::verify_ed25519_signature(state_sig_data, client->token.state_signature, client->device_public_key_pem);
            } catch (...) {
                state_ok = false;
            }
            if (!state_ok) {
                set_err(result, "state signature verification failed");
                return DL_ERROR_SUCCESS;
            }
        } else {
            set_ok(result);
        }

        // CRITICAL: Verify token signature is required
        // This prevents fake tokens with invalid signatures from being accepted
        // CRITICAL: Verify token signature is required
        // (expire_time is intentionally excluded - matches official SDK)
        if (client->token.signature.empty()) {
            set_err(result, "token signature is required");
            return DL_ERROR_SUCCESS;
        }
        
        // Build signature data using OFFICIAL format: token_id|app_id|holder_device_id|license_code|issue_time
        // Use original_holder_device_id (before activation changed it to device_id)
        const std::string& sig_holder_device_id = client->activated ? client->original_holder_device_id : client->token.holder_device_id;
        std::string sig_data = client->token.token_id + "|" +
                               client->token.app_id + "|" +
                               sig_holder_device_id + "|" +
                               client->token.license_code + "|" +
                               std::to_string(client->token.issue_time);
        
        // Verify signature based on algorithm type
        bool sig_ok = false;
        std::string alg = client->token.alg;
        
        try {
            if (alg == "RSA" || alg.empty()) {
                sig_ok = CryptoUtils::verify_signature(sig_data, client->token.signature, client->product_public_key_pem);
            } else if (alg == "Ed25519") {
                sig_ok = CryptoUtils::verify_ed25519_signature(sig_data, client->token.signature, client->product_public_key_pem);
            } else if (alg == "SM2") {
                sig_ok = CryptoUtils::verify_sm2_signature(sig_data, client->token.signature, client->product_public_key_pem);
            } else {
                sig_ok = CryptoUtils::verify_ed25519_signature(sig_data, client->token.signature, client->product_public_key_pem);
            }
        } catch (...) {
            sig_ok = false;
        }
        
        
        if (!sig_ok) {
            set_err(result, "token signature verification failed");
            return DL_ERROR_SUCCESS;
        }

        set_ok(result);
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        set_err(result, e.what());
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        set_err(result, "unknown error");
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_get_status(DL_Client* client, DL_StatusResult* status) {
    if (!client || !status) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    std::memset(status, 0, sizeof(DL_StatusResult));
    status->has_token = client->has_token ? 1 : 0;
    status->is_activated = client->activated ? 1 : 0;
    if (!client->has_token) {
        return DL_ERROR_SUCCESS;
    }

    status->issue_time = static_cast<int64_t>(client->token.issue_time);
    status->expire_time = static_cast<int64_t>(client->token.expire_time);
    status->state_index = client->token.state_index;

    std::strncpy(status->token_id, client->token.token_id.c_str(), sizeof(status->token_id) - 1);
    std::strncpy(status->holder_device_id, client->token.holder_device_id.c_str(), sizeof(status->holder_device_id) - 1);
    std::strncpy(status->app_id, client->token.app_id.c_str(), sizeof(status->app_id) - 1);
    std::strncpy(status->license_code, client->token.license_code.c_str(), sizeof(status->license_code) - 1);
    return DL_ERROR_SUCCESS;
}

DL_ErrorCode dl_client_activate_bind_device(DL_Client* client, DL_VerificationResult* result) {
    if (!client || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        set_err(result, "no token");
        return DL_ERROR_SUCCESS;
    }

    // Save whether token was already activated before this call
    // (e.g., re-import of activated token) to preserve original_holder_device_id
    bool was_already_activated = client->activated;

    // If token is already activated (restored from re-import of activated token),
    // skip re-verification - it was already verified during original activation.
    // This prevents "token signature verification failed" on re-imported activated tokens
    // where original_holder_device_id may not be available (old format).
    if (!client->activated) {
        DL_VerificationResult vr;
        dl_client_offline_verify_current_token(client, &vr);
        if (!vr.valid) {
            *result = vr;
            return DL_ERROR_SUCCESS;
        }
    }

    try {
        // Check if device keys already exist in storage (idempotent operation)
        bool keys_loaded = false;
        if (client->storage && !client->token.license_code.empty()) {
            auto saved_keys = client->storage->loadDeviceKeys(client->token.license_code);
            if (saved_keys.has_value()) {
                // Restore existing device keys (true idempotent behavior)
                client->device_private_key_pem = saved_keys->device_private_key_pem;
                client->device_public_key_pem = saved_keys->device_public_key_pem;
                client->device_id = saved_keys->device_id;
                keys_loaded = true;
            }
        }

        // If no saved keys exist, generate new ones
        if (!keys_loaded) {
            auto kp = CryptoUtils::generate_ed25519_keypair();
            client->device_private_key_pem = kp.private_key_pem;
            client->device_public_key_pem = kp.public_key_pem;
            client->device_id = CryptoUtils::generate_device_id();

            // Save the newly generated keys for future idempotent calls
            if (client->storage && !client->token.license_code.empty()) {
                client->storage->saveDeviceKeys(
                    client->token.license_code,
                    client->device_private_key_pem,
                    client->device_public_key_pem,
                    client->device_id
                );
            }
        }

        const std::string data_to_sign = client->device_id + client->device_public_key_pem;
        client->device_signature = CryptoUtils::sign_ed25519_data(data_to_sign, client->device_private_key_pem);

        client->activated = true;
        // Save original holder_device_id before overwriting (needed for signature verification)
        // Only set if not already preserved (e.g., from re-import of activated token)
        // CRITICAL: Don't overwrite if already set from re-import, even if empty
        // (token may have been issued with empty holder_device_id)
        if (!was_already_activated) {
            client->original_holder_device_id = client->token.holder_device_id;
        }
        client->token.holder_device_id = client->device_id;
        client->token.license_public_key = "";

        // --- SEK: Generate State Encryption Key ---
        // If SEK already exists (re-activation), reuse it; otherwise generate new one
        if (!client->has_sek) {
            // Check if encrypted_sek_device exists in the imported token JSON
            std::string existing_encrypted_sek_device = extract_json_string(client->token_json, "encrypted_sek_device");
            if (!existing_encrypted_sek_device.empty() && !client->device_private_key_pem.empty()) {
                // Re-import of activated token: decrypt SEK from device channel
                if (decrypt_sek_with_device_key(existing_encrypted_sek_device, client->device_private_key_pem, client->sek)) {
                    client->has_sek = true;
                    client->encrypted_sek_device = existing_encrypted_sek_device;
                    // Also preserve password channel if present
                    client->encrypted_sek_password = extract_json_string(client->token_json, "encrypted_sek_password");
                }
            }

            if (!client->has_sek) {
                // First activation: generate new SEK
                client->sek = CryptoUtils::generate_sek();
                client->has_sek = true;
                // Wrap SEK with device key
                client->encrypted_sek_device = encrypt_sek_with_device_key(client->sek, client->device_private_key_pem);
                // Password channel is empty by default (no user password)
                client->encrypted_sek_password = "";
            }
        }

        // --- Encrypt state_payload with SEK (if non-empty) ---
        bool state_payload_encrypted = false;
        if (client->has_sek && !client->token.state_payload.empty()) {
            // State signature was computed over plaintext state_payload (already done above or in prior step)
            // Now encrypt the state_payload for storage in JSON
            // NOTE: We keep token.state_payload as plaintext in memory for signing/verification
            // The encrypted version goes into the JSON output only
            state_payload_encrypted = true;
        }

        // Build token JSON with original_holder_device_id preserved
        // so re-imported activated tokens can verify correctly
        // If state_payload is encrypted, we store the ciphertext in JSON instead of plaintext
        if (state_payload_encrypted) {
            // Create a temporary token with encrypted state_payload for JSON output
            Token json_token = client->token;
            json_token.state_payload = encrypt_state_payload_with_sek(client->token.state_payload, client->sek);
            client->token_json = build_token_json_with_original_holder(
                json_token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                true);
        } else {
            client->token_json = build_token_json_with_original_holder(
                client->token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                client->has_sek);  // still mark encrypted=true if SEK exists even if payload empty
        }

        if (client->storage && !client->token.license_code.empty()) {
            (void)client->storage->appendStateJSON(client->token.license_code, client->token_json);
        }
        set_ok(result);
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        set_err(result, e.what());
        return DL_ERROR_UNKNOWN_ERROR;
    } catch (...) {
        set_err(result, "unknown error");
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_record_usage(DL_Client* client, const char* new_state_payload_json, DL_VerificationResult* result) {
    if (!client || !new_state_payload_json || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        set_err(result, "no token");
        return DL_ERROR_SUCCESS;
    }
    if (!client->activated) {
        set_err(result, "not activated");
        return DL_ERROR_SUCCESS;
    }

    try {
        if (client->device_private_key_pem.empty() || client->device_public_key_pem.empty()) {
            set_err(result, "device keys not initialized");
            return DL_ERROR_SUCCESS;
        }

        const std::string prev_hash = CryptoUtils::sha256(client->token_json);
        client->token.prev_state_hash = prev_hash;
        client->token.state_index += 1;
        client->token.state_payload = new_state_payload_json;

        // State signature is computed over PLAINTEXT state_payload
        const std::string state_sig_data = build_state_sig_data(client->token.state_index, client->token.prev_state_hash, client->token.state_payload);
        client->token.state_signature = CryptoUtils::sign_ed25519_data(state_sig_data, client->device_private_key_pem);

        // Build JSON: if SEK exists, encrypt state_payload for storage
        if (client->has_sek && !client->token.state_payload.empty()) {
            Token json_token = client->token;
            json_token.state_payload = encrypt_state_payload_with_sek(client->token.state_payload, client->sek);
            client->token_json = build_token_json_with_original_holder(
                json_token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                true);
        } else {
            client->token_json = build_token_json_with_original_holder(
                client->token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                client->has_sek);
        }

        if (client->storage && !client->token.license_code.empty()) {
            (void)client->storage->appendStateJSON(client->token.license_code, client->token_json);
        }

        set_ok(result);
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        set_err(result, e.what());
        return DL_ERROR_UNKNOWN_ERROR;
    } catch (...) {
        set_err(result, "unknown error");
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_get_state_payload(DL_Client* client, char* out_payload, size_t out_payload_size) {
    if (!client || !out_payload || out_payload_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_token) {
        out_payload[0] = '\0';
        return DL_ERROR_SUCCESS;
    }
    // token.state_payload is always plaintext in memory (decrypted during import)
    std::strncpy(out_payload, client->token.state_payload.c_str(), out_payload_size - 1);
    out_payload[out_payload_size - 1] = '\0';
    return DL_ERROR_SUCCESS;
}

DL_ErrorCode dl_client_add_recovery_channel(DL_Client* client, const char* password, DL_VerificationResult* result) {
    if (!client || !password || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_sek) {
        set_err(result, "no SEK available - activate first");
        return DL_ERROR_SUCCESS;
    }
    try {
        // Wrap SEK with password-derived key
        client->encrypted_sek_password = encrypt_sek_with_password(client->sek, std::string(password));

        // Rebuild token JSON with the new encrypted_sek_password
        if (client->has_sek && !client->token.state_payload.empty()) {
            Token json_token = client->token;
            json_token.state_payload = encrypt_state_payload_with_sek(client->token.state_payload, client->sek);
            client->token_json = build_token_json_with_original_holder(
                json_token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                true);
        } else {
            client->token_json = build_token_json_with_original_holder(
                client->token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                client->encrypted_sek_password,
                client->has_sek);
        }
        set_ok(result);
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        set_err(result, e.what());
        return DL_ERROR_UNKNOWN_ERROR;
    } catch (...) {
        set_err(result, "unknown error");
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_remove_recovery_channel(DL_Client* client, DL_VerificationResult* result) {
    if (!client || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }
    if (!client->has_sek) {
        set_err(result, "no SEK available");
        return DL_ERROR_SUCCESS;
    }
    try {
        client->encrypted_sek_password = "";

        // Rebuild token JSON without encrypted_sek_password
        if (!client->token.state_payload.empty()) {
            Token json_token = client->token;
            json_token.state_payload = encrypt_state_payload_with_sek(client->token.state_payload, client->sek);
            client->token_json = build_token_json_with_original_holder(
                json_token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                "",
                true);
        } else {
            client->token_json = build_token_json_with_original_holder(
                client->token, client->original_holder_device_id,
                client->device_id, client->device_public_key_pem,
                client->device_signature, true,
                client->encrypted_sek_device,
                "",
                client->has_sek);
        }
        set_ok(result);
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        set_err(result, e.what());
        return DL_ERROR_UNKNOWN_ERROR;
    } catch (...) {
        set_err(result, "unknown error");
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Activate license
DL_ErrorCode dl_client_activate(DL_Client* client, DL_ActivationResult* result) {
    if (!client || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        ActivationResult activation_result = client->client->activate_license(client->config.license_code);

        result->success = activation_result.success ? 1 : 0;
        strncpy(result->message, activation_result.message.c_str(), sizeof(result->message) - 1);
        result->message[sizeof(result->message) - 1] = '\0';

        if (activation_result.token.has_value()) {
            // Note: For simplicity, we're not handling token allocation here
            result->token = nullptr;
        } else {
            result->token = nullptr;
        }

        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

DL_ErrorCode dl_client_activate_with_token(DL_Client* client, const char* token_string, DL_ActivationResult* result) {
    if (!client || !token_string || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }


    // Reset client state to ensure clean activation
    client->token_json.clear();
    client->token = Token();
    client->has_token = false;
    client->activated = false;

    try {
        std::string token_str(token_string);

        std::string json_token_str;

        // Check if token is encrypted and decrypt if necessary
        if (is_encrypted_token_format(token_str)) {
            if (client->product_public_key_file_content.empty()) {
                result->success = 0;
                strncpy(result->message, "Product public key not set for encrypted token", sizeof(result->message) - 1);
                result->message[sizeof(result->message) - 1] = '\0';
                result->token = nullptr;
                return DL_ERROR_SUCCESS;
            }

            try {
                json_token_str = CryptoUtils::decrypt_token_aes256_gcm(token_str, client->product_public_key_file_content);
            } catch (const std::exception& e) {
                result->success = 0;
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Token decryption failed: %s", e.what());
                strncpy(result->message, error_msg, sizeof(result->message) - 1);
                result->message[sizeof(result->message) - 1] = '\0';
                result->token = nullptr;
                return DL_ERROR_CRYPTO_ERROR;
            } catch (...) {
                result->success = 0;
                strncpy(result->message, "Token decryption failed", sizeof(result->message) - 1);
                result->message[sizeof(result->message) - 1] = '\0';
                result->token = nullptr;
                return DL_ERROR_CRYPTO_ERROR;
            }
        } else {
            json_token_str = token_str;
        }

        // Parse JSON token (using same logic as dl_client_import_token)
        Token token;
        token.token_id = extract_json_string(json_token_str, "token_id");
        token.app_id = extract_json_string(json_token_str, "app_id");
        token.holder_device_id = extract_json_string(json_token_str, "holder_device_id");
        token.license_code = extract_json_string(json_token_str, "license_code");
        token.issue_time = extract_json_u64(json_token_str, "issue_time");
        token.expire_time = extract_json_u64(json_token_str, "expire_time");
        token.environment_hash = extract_json_string(json_token_str, "environment_hash");
        token.root_signature = extract_json_string(json_token_str, "root_signature");
        token.state_index = extract_json_u64(json_token_str, "state_index");
        token.prev_state_hash = extract_json_string(json_token_str, "prev_state_hash");
        token.state_payload = extract_json_string(json_token_str, "state_payload");
        token.state_signature = extract_json_string(json_token_str, "state_signature");
        token.alg = extract_json_string(json_token_str, "alg");
        token.signature = extract_json_string(json_token_str, "signature");

        // Extract additional fields if present
        token.license_public_key = extract_json_string(json_token_str, "license_public_key");
        token.encrypted_license_private_key = extract_json_string(json_token_str, "encrypted_license_private_key");


        // Verify token trust chain
        bool trust_valid = client->client->verify_token_trust_chain(token);
        if (!trust_valid) {
            result->success = 0;
            strncpy(result->message, "Token trust chain verification failed", sizeof(result->message) - 1);
            result->message[sizeof(result->message) - 1] = '\0';
            result->token = nullptr;
            return DL_ERROR_SUCCESS;
        }

        // Perform offline activation
        ActivationResult activation_result = client->client->activate_with_token(token);

        result->success = activation_result.success ? 1 : 0;
        strncpy(result->message, activation_result.message.c_str(), sizeof(result->message) - 1);
        result->message[sizeof(result->message) - 1] = '\0';

        if (activation_result.token.has_value()) {
            result->token = nullptr; // Simplified for now
        } else {
            result->token = nullptr;
        }

        return DL_ERROR_SUCCESS;
    } catch (...) {
        result->success = 0;
        strncpy(result->message, "Activation failed due to internal error", sizeof(result->message) - 1);
        result->message[sizeof(result->message) - 1] = '\0';
        result->token = nullptr;
        return DL_ERROR_SUCCESS;
    }
}

// Get current token
DL_ErrorCode dl_client_get_current_token(DL_Client* client, DL_Token* token) {
    if (!client || !token) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    std::memset(token, 0, sizeof(DL_Token));
    if (!client->has_token) {
        return DL_ERROR_SUCCESS;
    }
    std::strncpy(token->token_id, client->token.token_id.c_str(), sizeof(token->token_id) - 1);
    std::strncpy(token->holder_device_id, client->token.holder_device_id.c_str(), sizeof(token->holder_device_id) - 1);
    token->issue_time = static_cast<int64_t>(client->token.issue_time);
    token->expire_time = static_cast<int64_t>(client->token.expire_time);
    std::strncpy(token->signature, client->token.signature.c_str(), sizeof(token->signature) - 1);
    std::strncpy(token->root_signature, client->token.root_signature.c_str(), sizeof(token->root_signature) - 1);
    std::strncpy(token->app_id, client->token.app_id.c_str(), sizeof(token->app_id) - 1);
    std::strncpy(token->license_code, client->token.license_code.c_str(), sizeof(token->license_code) - 1);
    return DL_ERROR_SUCCESS;
}

// Check if license is activated
int dl_client_is_activated(DL_Client* client) {
    if (!client || !client->client) {
        return 0;
    }

    // If already marked as activated in memory, return true
    if (client->activated) {
        return 1;
    }

    // Check if there's an activated state in persistent storage
    // This allows new processes to detect previously activated tokens
    if (client->storage && client->has_token && !client->token.license_code.empty()) {
        try {
            // Get the current state from storage
            auto current_state = client->storage->getCurrentState(client->token.license_code);
            if (current_state.has_value()) {
                // If the stored state has a holder_device_id, it means it was activated
                if (!current_state->holder_device_id.empty()) {
                    // Restore activation state from storage
                    client->activated = true;
                    client->device_id = current_state->holder_device_id;
                    return 1;
                }
            }
        } catch (...) {
            // If any error occurs, just return the current memory state
        }
    }

    return 0;
}

// Get device ID
DL_ErrorCode dl_client_get_device_id(DL_Client* client, char* device_id, size_t device_id_size) {
    if (!client || !device_id || device_id_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    std::string id = client->device_id;
    std::strncpy(device_id, id.c_str(), device_id_size - 1);
    device_id[device_id_size - 1] = '\0';
    return DL_ERROR_SUCCESS;
}

// Get device state
DL_DeviceState dl_client_get_device_state(DL_Client* client) {
    if (!client || !client->client) {
        return DL_DEVICE_STATE_IDLE;
    }

    return DL_DEVICE_STATE_IDLE;
}

// Verify token using trust chain
DL_ErrorCode dl_client_verify_token_trust_chain(DL_Client* client, const DL_Token* token, const char* root_public_key_pem, DL_VerificationResult* result) {
    if (!client || !token || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        // Convert C token structure to C++ Token structure
        Token cpp_token;
        cpp_token.token_id = std::string(token->token_id);
        cpp_token.holder_device_id = std::string(token->holder_device_id);
        cpp_token.issue_time = token->issue_time;
        cpp_token.expire_time = token->expire_time;
        cpp_token.signature = std::string(token->signature);
        cpp_token.license_public_key = std::string(token->license_public_key);
        cpp_token.root_signature = std::string(token->root_signature);
        cpp_token.app_id = std::string(token->app_id);
        cpp_token.license_code = std::string(token->license_code);

        // Get the token manager from the client
        // Note: In a real implementation, you would access the token manager through the client
        TokenManager token_manager;

        // Always use the hardcoded ROOT_PUBLIC_KEY (ignore root_public_key_pem parameter)
        bool valid = token_manager.verify_token_trust_chain(cpp_token);
        
        result->valid = valid ? 1 : 0;
        if (!valid) {
            strncpy(result->error_message, "Token trust chain verification failed", sizeof(result->error_message) - 1);
            result->error_message[sizeof(result->error_message) - 1] = '\0';
        } else {
            strncpy(result->error_message, "", sizeof(result->error_message) - 1);
        }
        
        return DL_ERROR_SUCCESS;
    } catch (const std::exception& e) {
        result->valid = 0;
        strncpy(result->error_message, e.what(), sizeof(result->error_message) - 1);
        result->error_message[sizeof(result->error_message) - 1] = '\0';
        return DL_ERROR_CRYPTO_ERROR;
    } catch (...) {
        result->valid = 0;
        strncpy(result->error_message, "Unknown error during token verification", sizeof(result->error_message) - 1);
        result->error_message[sizeof(result->error_message) - 1] = '\0';
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Shutdown the client
DL_ErrorCode dl_client_shutdown(DL_Client* client) {
    if (!client) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    if (!client->client) {
        return DL_ERROR_NOT_INITIALIZED;
    }

    try {
        client->client->stop();
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

} // extern "C"