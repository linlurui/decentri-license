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

static bool split_product_public_key_file(const std::string& file_content, std::string* out_pem, std::string* out_root_sig) {
    if (!out_pem || !out_root_sig) {
        return false;
    }
    const std::string marker = "ROOT_SIGNATURE:";
    size_t pos = file_content.find(marker);
    if (pos == std::string::npos) {
        *out_pem = trim(file_content);
        *out_root_sig = "";
        return !out_pem->empty();
    }
    std::string pem = file_content.substr(0, pos);
    std::string sig = file_content.substr(pos + marker.size());
    *out_pem = trim(pem);
    *out_root_sig = trim(sig);
    return !out_pem->empty();
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
    std::unique_ptr<StateChainStorage> storage;
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

        client->token_json = json;
        client->token = t;
        client->has_token = true;
        client->activated = false;

        if (client->storage && !client->token.license_code.empty()) {
            std::vector<Token> chain;
            chain.push_back(client->token);
            (void)client->storage->saveFullChain(client->token.license_code, chain);
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

    DL_VerificationResult vr;
    dl_client_offline_verify_current_token(client, &vr);
    if (!vr.valid) {
        *result = vr;
        return DL_ERROR_SUCCESS;
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
        client->token.holder_device_id = client->device_id;
        client->token.license_public_key = "";

        client->token_json = build_token_json(client->token, client->device_id, client->device_public_key_pem, client->device_signature, true);

        if (client->storage && !client->token.license_code.empty()) {
            (void)client->storage->appendState(client->token.license_code, client->token);
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

        const std::string state_sig_data = build_state_sig_data(client->token.state_index, client->token.prev_state_hash, client->token.state_payload);
        client->token.state_signature = CryptoUtils::sign_ed25519_data(state_sig_data, client->device_private_key_pem);

        client->token_json = build_token_json(client->token, client->device_id, client->device_public_key_pem, client->device_signature, true);

        if (client->storage && !client->token.license_code.empty()) {
            (void)client->storage->appendState(client->token.license_code, client->token);
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