#ifndef DECENLICENSE_C_H
#define DECENLICENSE_C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// opaque pointer to the DecentriLicense client
typedef struct DL_Client DL_Client;

// Forward declaration of DL_Token
typedef struct DL_Token DL_Token;

// Device state enum
typedef enum {
    DL_DEVICE_STATE_IDLE = 0,
    DL_DEVICE_STATE_DISCOVERING,
    DL_DEVICE_STATE_ELECTING,
    DL_DEVICE_STATE_COORDINATOR,
    DL_DEVICE_STATE_FOLLOWER
} DL_DeviceState;

// Activation result
typedef struct {
    int success;
    char message[256];
    DL_Token* token;
} DL_ActivationResult;

// Connection mode
typedef enum {
    DL_CONNECTION_MODE_WAN_REGISTRY = 0,  // 广域网注册中心优先
    DL_CONNECTION_MODE_LAN_P2P = 1,       // 局域网P2P
    DL_CONNECTION_MODE_OFFLINE = 2        // 离线模式
} DL_ConnectionMode;

// Client configuration
typedef struct {
    const char* license_code;             // License identifier for P2P conflict detection
    DL_ConnectionMode preferred_mode;     // Preferred connection mode
    uint16_t udp_port;                    // UDP port for P2P discovery (0 = use default 13325)
    uint16_t tcp_port;                    // TCP port for P2P communication (0 = use default 23325)
    const char* registry_server_url;     // Optional WAN coordination server
} DL_ClientConfig;

// Error codes
typedef enum {
    DL_ERROR_SUCCESS = 0,
    DL_ERROR_INVALID_ARGUMENT,
    DL_ERROR_NOT_INITIALIZED,
    DL_ERROR_ALREADY_INITIALIZED,
    DL_ERROR_NETWORK_ERROR,
    DL_ERROR_CRYPTO_ERROR,
    DL_ERROR_UNKNOWN_ERROR
} DL_ErrorCode;

// Token structure
struct DL_Token {
    char token_id[128];             // Increased for future hash algorithm support
    char holder_device_id[256];     // 256 bytes for SHA512 (128 chars) + null terminator + future expansion
    int64_t issue_time;
    int64_t expire_time;
    char signature[512];
    char license_public_key[1024];  // Added for trust chain verification
    char root_signature[512];       // Added for trust chain verification
    char app_id[128];               // Increased for consistency
    char license_code[128];         // Increased for consistency
};

// Verification result structure
typedef struct {
    int valid;
    char error_message[256];
} DL_VerificationResult;

typedef struct {
    int has_token;
    int is_activated;
    int64_t issue_time;
    int64_t expire_time;
    uint64_t state_index;
    char token_id[128];             // Increased for future hash algorithm support
    char holder_device_id[256];     // 256 bytes for SHA512 (128 chars) + null terminator + future expansion
    char app_id[128];               // Increased for consistency
    char license_code[128];         // Increased for consistency
} DL_StatusResult;

// Create a new client
DL_Client* dl_client_create(void);

// Destroy a client
void dl_client_destroy(DL_Client* client);

// Initialize the client
DL_ErrorCode dl_client_initialize(DL_Client* client, const DL_ClientConfig* config);

DL_ErrorCode dl_client_set_product_public_key(DL_Client* client, const char* product_public_key_file_content);

DL_ErrorCode dl_client_import_token(DL_Client* client, const char* token_input);

DL_ErrorCode dl_client_get_current_token_json(DL_Client* client, char* out_json, size_t out_json_size);

DL_ErrorCode dl_client_export_current_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size);

DL_ErrorCode dl_client_export_activated_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size);

DL_ErrorCode dl_client_export_state_changed_token_encrypted(DL_Client* client, char* out_encrypted, size_t out_encrypted_size);

DL_ErrorCode dl_client_offline_verify_current_token(DL_Client* client, DL_VerificationResult* result);

DL_ErrorCode dl_client_get_status(DL_Client* client, DL_StatusResult* status);

DL_ErrorCode dl_client_activate_bind_device(DL_Client* client, DL_VerificationResult* result);

DL_ErrorCode dl_client_record_usage(DL_Client* client, const char* new_state_payload_json, DL_VerificationResult* result);

// Activate license
DL_ErrorCode dl_client_activate(DL_Client* client, DL_ActivationResult* result);

// Activate license with offline token string
DL_ErrorCode dl_client_activate_with_token(DL_Client* client, const char* token_string, DL_ActivationResult* result);

// Get current token
DL_ErrorCode dl_client_get_current_token(DL_Client* client, DL_Token* token);

// Check if license is activated
int dl_client_is_activated(DL_Client* client);

// Get device ID
DL_ErrorCode dl_client_get_device_id(DL_Client* client, char* device_id, size_t device_id_size);

// Get device state
DL_DeviceState dl_client_get_device_state(DL_Client* client);

// Verify token using trust chain
DL_ErrorCode dl_client_verify_token_trust_chain(DL_Client* client, const DL_Token* token, const char* root_public_key_pem, DL_VerificationResult* result);

// Shutdown the client
DL_ErrorCode dl_client_shutdown(DL_Client* client);

#ifdef __cplusplus
}
#endif

#endif // DECENLICENSE_C_H