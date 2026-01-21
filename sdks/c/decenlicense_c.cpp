#include "decenlicense_c.h"
#include "decentrilicense/decentrilicense_client.hpp"
#include <cstring>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque wrapper struct
struct DL_Client {
    std::unique_ptr<decentrilicense::DecentriLicenseClient> client;
};

// Create a new client
DL_Client* dl_client_create(void) {
    try {
        DL_Client* wrapper = new DL_Client();
        // Create a default config for the client
        decentrilicense::ClientConfig config;
        config.license_code = "";
        config.udp_port = 8888;
        config.tcp_port = 8889;
        config.generate_keys_automatically = true;
        wrapper->client = std::make_unique<decentrilicense::DecentriLicenseClient>(config);
        return wrapper;
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
    if (!client || !config || !config->license_code) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        // Re-create the client with the new configuration
        decentrilicense::ClientConfig cpp_config;
        cpp_config.license_code = config->license_code ? config->license_code : "";
        cpp_config.udp_port = config->udp_port;
        cpp_config.tcp_port = config->tcp_port;
        if (config->registry_server_url) {
            cpp_config.registry_server_url = config->registry_server_url;
        }
        cpp_config.generate_keys_automatically = true;

        client->client = std::make_unique<decentrilicense::DecentriLicenseClient>(cpp_config);
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Activate license
DL_ErrorCode dl_client_activate(DL_Client* client, DL_ActivationResult* result) {
    if (!client || !result) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        // Note: This is a simplified implementation since std::future cannot be easily exposed in C
        // In a real implementation, we would need a callback mechanism or polling approach
        memset(result, 0, sizeof(DL_ActivationResult));
        result->success = 0;  // Indicate that this is not fully implemented
        strncpy(result->message, "Not implemented in C binding", sizeof(result->message) - 1);
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Get current token
DL_ErrorCode dl_client_get_current_token(DL_Client* client, DL_Token* token) {
    if (!client || !token) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        // This function doesn't exist in the C++ client, so we'll return an error
        return DL_ERROR_NOT_INITIALIZED;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Check if license is activated
int dl_client_is_activated(DL_Client* client) {
    if (!client) {
        return 0;
    }

    try {
        // This function doesn't exist in the C++ client, so we'll return false
        return 0;
    } catch (...) {
        return 0;
    }
}

// Get device ID
DL_ErrorCode dl_client_get_device_id(DL_Client* client, char* device_id, size_t device_id_size) {
    if (!client || !device_id || device_id_size == 0) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        // This function doesn't exist in the C++ client, so we'll return an error
        return DL_ERROR_NOT_INITIALIZED;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

// Get device state
DL_DeviceState dl_client_get_device_state(DL_Client* client) {
    if (!client) {
        return DL_DEVICE_STATE_IDLE;
    }

    try {
        // This function doesn't exist in the C++ client, so we'll return idle
        return DL_DEVICE_STATE_IDLE;
    } catch (...) {
        return DL_DEVICE_STATE_IDLE;
    }
}

// Shutdown the client
DL_ErrorCode dl_client_shutdown(DL_Client* client) {
    if (!client) {
        return DL_ERROR_INVALID_ARGUMENT;
    }

    try {
        // This function doesn't exist in the C++ client, so we'll just reset the client
        client->client.reset();
        return DL_ERROR_SUCCESS;
    } catch (...) {
        return DL_ERROR_UNKNOWN_ERROR;
    }
}

#ifdef __cplusplus
}
#endif