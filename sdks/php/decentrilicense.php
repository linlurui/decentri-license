<?php
/**
 * DecentriLicense PHP SDK
 * 
 * 用于验证DecentriLicense许可证令牌的PHP库
 */

class LicenseException extends Exception {
}

class DecentriLicenseClientConfig {
    public $license_code = '';
    public $udp_port = 8888;
    public $tcp_port = 8889;
    public $registry_server_url = '';
}

class DecentriLicenseClient {
    private $ffi = null;
    private $client = null;
    private $initialized = false;

    public function __construct() {
        $this->ffi = $this->loadFfi();
        $this->client = $this->ffi->dl_client_create();
        if ($this->client === null) {
            throw new LicenseException('failed to create client');
        }
    }

    public function initialize(DecentriLicenseClientConfig $config): void {
        $this->ensureNotDisposed();
        if ($this->initialized) {
            throw new LicenseException('client already initialized');
        }

        $cConfig = $this->ffi->new('DL_ClientConfig');
        $cConfig->license_code = $config->license_code;
        $cConfig->preferred_mode = 1; // DL_CONNECTION_MODE_LAN_P2P
        $cConfig->udp_port = (int)$config->udp_port;
        $cConfig->tcp_port = (int)$config->tcp_port;
        $cConfig->registry_server_url = $config->registry_server_url;

        $rc = $this->ffi->dl_client_initialize($this->client, FFI::addr($cConfig));
        $this->throwIfError($rc, 'initialize failed');
        $this->initialized = true;
    }

    public function setProductPublicKey(string $product_public_key_file_content): void {
        $this->ensureInitialized();
        $rc = $this->ffi->dl_client_set_product_public_key($this->client, $product_public_key_file_content);
        $this->throwIfError($rc, 'set product public key failed');
    }

    public function importToken(string $token_input): void {
        $this->ensureInitialized();
        $rc = $this->ffi->dl_client_import_token($this->client, $token_input);
        $this->throwIfError($rc, 'import token failed');
    }

    public function offlineVerifyCurrentToken(): array {
        $this->ensureInitialized();
        $vr = $this->ffi->new('DL_VerificationResult');
        $rc = $this->ffi->dl_client_offline_verify_current_token($this->client, FFI::addr($vr));
        $this->throwIfError($rc, 'offline verify failed');
        return [
            'valid' => ((int)$vr->valid) !== 0,
            'error_message' => $this->cStringFromArray($vr->error_message),
        ];
    }

    public function activateBindDevice(): array {
        $this->ensureInitialized();
        $vr = $this->ffi->new('DL_VerificationResult');
        $rc = $this->ffi->dl_client_activate_bind_device($this->client, FFI::addr($vr));
        $this->throwIfError($rc, 'activate bind device failed');
        return [
            'valid' => ((int)$vr->valid) !== 0,
            'error_message' => $this->cStringFromArray($vr->error_message),
        ];
    }

    public function getStatus(): array {
        $this->ensureInitialized();
        $st = $this->ffi->new('DL_StatusResult');
        $rc = $this->ffi->dl_client_get_status($this->client, FFI::addr($st));
        $this->throwIfError($rc, 'get status failed');
        return [
            'has_token' => ((int)$st->has_token) !== 0,
            'is_activated' => ((int)$st->is_activated) !== 0,
            'issue_time' => (int)$st->issue_time,
            'expire_time' => (int)$st->expire_time,
            'state_index' => (int)$st->state_index,
            'token_id' => $this->cStringFromArray($st->token_id),
            'holder_device_id' => $this->cStringFromArray($st->holder_device_id),
            'app_id' => $this->cStringFromArray($st->app_id),
            'license_code' => $this->cStringFromArray($st->license_code),
        ];
    }

    public function recordUsage(string $new_state_payload_json): array {
        $this->ensureInitialized();
        $vr = $this->ffi->new('DL_VerificationResult');
        $rc = $this->ffi->dl_client_record_usage($this->client, $new_state_payload_json, FFI::addr($vr));
        $this->throwIfError($rc, 'record usage failed');
        return [
            'valid' => ((int)$vr->valid) !== 0,
            'error_message' => $this->cStringFromArray($vr->error_message),
        ];
    }

    public function getDeviceId(): string {
        $this->ensureInitialized();
        $buf = $this->ffi->new('char[256]');
        $rc = $this->ffi->dl_client_get_device_id($this->client, $buf, 256);
        $this->throwIfError($rc, 'get device id failed');
        return FFI::string($buf);
    }

    public function exportActivatedTokenEncrypted(): string {
        $this->ensureInitialized();
        $buf = $this->ffi->new('char[65536]');
        $rc = $this->ffi->dl_client_export_activated_token_encrypted($this->client, $buf, 65536);
        $this->throwIfError($rc, 'export activated token encrypted failed');
        return FFI::string($buf);
    }

    public function exportStateChangedTokenEncrypted(): string {
        $this->ensureInitialized();
        $buf = $this->ffi->new('char[65536]');
        $rc = $this->ffi->dl_client_export_state_changed_token_encrypted($this->client, $buf, 65536);
        $this->throwIfError($rc, 'export state changed token encrypted failed');
        return FFI::string($buf);
    }

    public function shutdown(): void {
        if ($this->ffi === null || $this->client === null) {
            return;
        }
        $this->ffi->dl_client_shutdown($this->client);
        $this->ffi->dl_client_destroy($this->client);
        $this->client = null;
        $this->ffi = null;
        $this->initialized = false;
    }

    private function loadFfi() {
        $skip = getenv('DECENLICENSE_SKIP_LOAD');
        if ($skip !== false && $skip !== '' && $skip !== '0') {
            throw new LicenseException('native load skipped');
        }
        if (!class_exists('FFI')) {
            throw new LicenseException('FFI is not available');
        }

        $cdef = <<< 'CDEF'
typedef struct DL_Client DL_Client;
typedef struct DL_Token DL_Token;

typedef struct {
    int success;
    char message[256];
    DL_Token* token;
} DL_ActivationResult;

typedef struct {
    const char* license_code;
    int preferred_mode;
    uint16_t udp_port;
    uint16_t tcp_port;
    const char* registry_server_url;
} DL_ClientConfig;

typedef enum {
    DL_ERROR_SUCCESS = 0,
    DL_ERROR_INVALID_ARGUMENT,
    DL_ERROR_NOT_INITIALIZED,
    DL_ERROR_ALREADY_INITIALIZED,
    DL_ERROR_NETWORK_ERROR,
    DL_ERROR_CRYPTO_ERROR,
    DL_ERROR_UNKNOWN_ERROR
} DL_ErrorCode;

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

DL_Client* dl_client_create(void);
void dl_client_destroy(DL_Client* client);
DL_ErrorCode dl_client_initialize(DL_Client* client, const DL_ClientConfig* config);
DL_ErrorCode dl_client_set_product_public_key(DL_Client* client, const char* product_public_key_file_content);
DL_ErrorCode dl_client_import_token(DL_Client* client, const char* token_input);
DL_ErrorCode dl_client_offline_verify_current_token(DL_Client* client, DL_VerificationResult* result);
DL_ErrorCode dl_client_get_status(DL_Client* client, DL_StatusResult* status);
DL_ErrorCode dl_client_activate_bind_device(DL_Client* client, DL_VerificationResult* result);
DL_ErrorCode dl_client_record_usage(DL_Client* client, const char* new_state_payload_json, DL_VerificationResult* result);
DL_ErrorCode dl_client_get_device_id(DL_Client* client, char* device_id, size_t device_id_size);
DL_ErrorCode dl_client_export_activated_token_encrypted(DL_Client* client, char* out_token, size_t out_token_size);
DL_ErrorCode dl_client_export_state_changed_token_encrypted(DL_Client* client, char* out_token, size_t out_token_size);
DL_ErrorCode dl_client_shutdown(DL_Client* client);
CDEF;

        $libPath = getenv('DECENLICENSE_LIB_PATH');
        $lib = ($libPath !== false && $libPath !== '') ? $libPath : 'decentrilicense';
        return FFI::cdef($cdef, $lib);
    }

    private function ensureInitialized(): void {
        $this->ensureNotDisposed();
        if (!$this->initialized) {
            throw new LicenseException('client not initialized');
        }
    }

    private function ensureNotDisposed(): void {
        if ($this->ffi === null || $this->client === null) {
            throw new LicenseException('client disposed');
        }
    }

    private function throwIfError(int $rc, string $message): void {
        if ($rc === 0) {
            return;
        }
        throw new LicenseException($message . ': ' . $rc);
    }

    private function cStringFromArray($arr): string {
        return FFI::string(FFI::addr($arr[0]));
    }
}

class DecentriLicense {
}
?>