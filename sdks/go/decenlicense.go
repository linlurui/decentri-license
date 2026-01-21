package decenlicense

/*
#cgo CFLAGS: -I../../dl-core/include
#cgo LDFLAGS: -L../../dl-core/build -ldecentrilicense -Wl,-rpath,${SRCDIR}/../../dl-core/build

#include "decenlicense_c.h"
#include <stdlib.h>
*/
import "C"
import (
	"bytes"
	"errors"
	"unsafe"
)

// ConnectionMode represents the preferred connection strategy
type ConnectionMode int

const (
	ConnectionModeWANRegistry ConnectionMode = 0 // 广域网注册中心优先
	ConnectionModeLANP2P      ConnectionMode = 1 // 局域网P2P
	ConnectionModeOffline     ConnectionMode = 2 // 离线模式
)

// C bindings updated with dl-core C API integration

// Config represents client configuration
type Config struct {
	LicenseCode       string         // License identifier for P2P conflict detection
	PreferredMode     ConnectionMode // Preferred connection mode (default: WAN)
	UDPPort           uint16         // UDP port for P2P discovery (default: 13325)
	TCPPort           uint16         // TCP port for P2P communication (default: 23325)
	RegistryServerURL string         // Optional WAN coordination server
}

// Token represents a license token
type Token struct {
	TokenID        string
	HolderDeviceID string
	IssueTime      int64
	ExpireTime     int64
	Signature      string
}

// VerificationResult maps to DL_VerificationResult
type VerificationResult struct {
	Valid        bool
	ErrorMessage string
}

// StatusResult maps to DL_StatusResult
type StatusResult struct {
	HasToken       bool
	IsActivated    bool
	IssueTime      int64
	ExpireTime     int64
	StateIndex     uint64
	TokenID        string
	HolderDeviceID string
	AppID          string
	LicenseCode    string
}

// DeviceState represents the state of a device
type DeviceState int

const (
	DeviceStateIdle DeviceState = iota
	DeviceStateDiscovering
	DeviceStateElecting
	DeviceStateCoordinator
	DeviceStateFollower
)

// Error codes
var (
	ErrInvalidArgument      = errors.New("invalid argument")
	ErrNotInitialized       = errors.New("not initialized")
	ErrAlreadyInitialized   = errors.New("already initialized")
	ErrNetworkError         = errors.New("network error")
	ErrCryptoError          = errors.New("crypto error")
	ErrUnknownError         = errors.New("unknown error")
)

// Client represents a DecentriLicense client
type Client struct {
	client *C.DL_Client
}

// NewClient creates a new DecentriLicense client
func NewClient() (*Client, error) {
	client := C.dl_client_create()
	if client == nil {
		return nil, errors.New("failed to create client")
	}
	return &Client{client: client}, nil
}

// Close closes the client and releases resources
func (c *Client) Close() error {
	if c.client != nil {
		C.dl_client_destroy(c.client)
		c.client = nil
	}
	return nil
}

// Initialize initializes the client with configuration
func (c *Client) Initialize(config Config) error {
	if c.client == nil {
		return ErrNotInitialized
	}

	// Convert Go config to C config
	var registryURL *C.char
	if config.RegistryServerURL != "" {
		registryURL = C.CString(config.RegistryServerURL)
		defer C.free(unsafe.Pointer(registryURL))
	}

	// C bindings updated with dl-core C API integration
	cConfig := C.DL_ClientConfig{
		license_code:        C.CString(config.LicenseCode),
		udp_port:            C.uint16_t(config.UDPPort),
		tcp_port:            C.uint16_t(config.TCPPort),
		registry_server_url: registryURL,
	}
	// Note: preferred_mode and dynamic_ports fields not available in current C API
	defer C.free(unsafe.Pointer(cConfig.license_code))

	result := C.dl_client_initialize(c.client, &cConfig)
	return c.errorFromCode(result)
}

// ActivateWithToken activates the license with an offline token string
func (c *Client) ActivateWithToken(tokenString string) (bool, error) {
	if c.client == nil {
		return false, ErrNotInitialized
	}

	tokenCString := C.CString(tokenString)
	defer C.free(unsafe.Pointer(tokenCString))

	// Import the token first (like ValidateToken does)
	errorCode := C.dl_client_import_token(c.client, tokenCString)
	if err := c.errorFromCode(errorCode); err != nil {
		return false, err
	}

	// Then activate using the imported token
	var result C.DL_ActivationResult
	errorCode = C.dl_client_activate(c.client, &result)
	if err := c.errorFromCode(errorCode); err != nil {
		return false, err
	}

	if result.success == 0 {
		// Convert C string to Go string
		message := C.GoString(&result.message[0])
		return false, errors.New("Activation failed: " + message)
	}

	return true, nil
}

// ValidateToken validates any token string without activating it
func (c *Client) ValidateToken(tokenString string) (VerificationResult, error) {
	if c.client == nil {
		return VerificationResult{}, ErrNotInitialized
	}

	tokenCString := C.CString(tokenString)
	defer C.free(unsafe.Pointer(tokenCString))

	var result C.DL_VerificationResult
	errorCode := C.dl_client_import_token(c.client, tokenCString)
	if err := c.errorFromCode(errorCode); err != nil {
		return VerificationResult{}, err
	}

	// Try to verify the imported token
	errorCode = C.dl_client_offline_verify_current_token(c.client, &result)
	if err := c.errorFromCode(errorCode); err != nil {
		return VerificationResult{}, err
	}

	return VerificationResult{
		Valid:        result.valid != 0,
		ErrorMessage: C.GoString(&result.error_message[0]),
	}, nil
}

// Activate activates the license (legacy method, may require network)
func (c *Client) Activate() (bool, error) {
	if c.client == nil {
		return false, ErrNotInitialized
	}

	var result C.DL_ActivationResult
	code := C.dl_client_activate(c.client, &result)
	err := c.errorFromCode(code)
	if err != nil {
		return false, err
	}

	return int(result.success) != 0, nil
}

// GetCurrentToken gets the current token if activated
func (c *Client) GetCurrentToken() (*Token, error) {
	if c.client == nil {
		return nil, ErrNotInitialized
	}

	var cToken C.DL_Token
	code := C.dl_client_get_current_token(c.client, &cToken)
	err := c.errorFromCode(code)
	if err != nil {
		return nil, err
	}

	token := &Token{
		TokenID:        C.GoString(&cToken.token_id[0]),
		HolderDeviceID: C.GoString(&cToken.holder_device_id[0]),
		IssueTime:      int64(cToken.issue_time),
		ExpireTime:     int64(cToken.expire_time),
		Signature:      C.GoString(&cToken.signature[0]),
	}

	return token, nil
}

// IsActivated checks if the license is activated
func (c *Client) IsActivated() (bool, error) {
	if c.client == nil {
		return false, ErrNotInitialized
	}

	result := C.dl_client_is_activated(c.client)
	return int(result) != 0, nil
}

// GetDeviceID gets the device ID
func (c *Client) GetDeviceID() (string, error) {
	if c.client == nil {
		return "", ErrNotInitialized
	}

	// Allocate buffer for device ID
	deviceIDBuf := make([]C.char, 256)
	result := C.dl_client_get_device_id(c.client, &deviceIDBuf[0], C.size_t(len(deviceIDBuf)))
	err := c.errorFromCode(result)
	if err != nil {
		return "", err
	}

	return C.GoString(&deviceIDBuf[0]), nil
}

// GetDeviceState gets the current device state
func (c *Client) GetDeviceState() (DeviceState, error) {
	if c.client == nil {
		return DeviceStateIdle, ErrNotInitialized
	}

	state := C.dl_client_get_device_state(c.client)
	return DeviceState(state), nil
}

// Shutdown shuts down the client
func (c *Client) Shutdown() error {
	if c.client == nil {
		return ErrNotInitialized
	}

	result := C.dl_client_shutdown(c.client)
	return c.errorFromCode(result)
}

func (c *Client) SetProductPublicKey(productPublicKeyFileContent string) error {
	if c.client == nil {
		return ErrNotInitialized
	}
	cs := C.CString(productPublicKeyFileContent)
	defer C.free(unsafe.Pointer(cs))
	result := C.dl_client_set_product_public_key(c.client, cs)
	return c.errorFromCode(result)
}

func (c *Client) ImportToken(tokenInput string) error {
	if c.client == nil {
		return ErrNotInitialized
	}
	cs := C.CString(tokenInput)
	defer C.free(unsafe.Pointer(cs))
	result := C.dl_client_import_token(c.client, cs)
	return c.errorFromCode(result)
}

func (c *Client) OfflineVerifyCurrentToken() (VerificationResult, error) {
	if c.client == nil {
		return VerificationResult{}, ErrNotInitialized
	}
	var vr C.DL_VerificationResult
	code := C.dl_client_offline_verify_current_token(c.client, &vr)
	err := c.errorFromCode(code)
	if err != nil {
		return VerificationResult{}, err
	}
	return VerificationResult{
		Valid:        int(vr.valid) != 0,
		ErrorMessage: C.GoString(&vr.error_message[0]),
	}, nil
}

func (c *Client) GetStatus() (StatusResult, error) {
	if c.client == nil {
		return StatusResult{}, ErrNotInitialized
	}
	var st C.DL_StatusResult
	code := C.dl_client_get_status(c.client, &st)
	err := c.errorFromCode(code)
	if err != nil {
		return StatusResult{}, err
	}
	return StatusResult{
		HasToken:       int(st.has_token) != 0,
		IsActivated:    int(st.is_activated) != 0,
		IssueTime:      int64(st.issue_time),
		ExpireTime:     int64(st.expire_time),
		StateIndex:     uint64(st.state_index),
		TokenID:        C.GoString(&st.token_id[0]),
		HolderDeviceID: C.GoString(&st.holder_device_id[0]),
		AppID:          C.GoString(&st.app_id[0]),
		LicenseCode:    C.GoString(&st.license_code[0]),
	}, nil
}

func (c *Client) ActivateBindDevice() (VerificationResult, error) {
	if c.client == nil {
		return VerificationResult{}, ErrNotInitialized
	}
	var vr C.DL_VerificationResult
	code := C.dl_client_activate_bind_device(c.client, &vr)
	err := c.errorFromCode(code)
	if err != nil {
		return VerificationResult{}, err
	}
	return VerificationResult{
		Valid:        int(vr.valid) != 0,
		ErrorMessage: C.GoString(&vr.error_message[0]),
	}, nil
}

func (c *Client) RecordUsage(newStatePayloadJSON string) (VerificationResult, error) {
	if c.client == nil {
		return VerificationResult{}, ErrNotInitialized
	}
	cs := C.CString(newStatePayloadJSON)
	defer C.free(unsafe.Pointer(cs))
	var vr C.DL_VerificationResult
	code := C.dl_client_record_usage(c.client, cs, &vr)
	err := c.errorFromCode(code)
	if err != nil {
		return VerificationResult{}, err
	}
	return VerificationResult{
		Valid:        int(vr.valid) != 0,
		ErrorMessage: C.GoString(&vr.error_message[0]),
	}, nil
}

// ExportActivatedTokenEncrypted exports the activated token as an encrypted string
func (c *Client) ExportActivatedTokenEncrypted() (string, error) {
	if c.client == nil {
		return "", ErrNotInitialized
	}
	buf := make([]byte, 65536)
	code := C.dl_client_export_activated_token_encrypted(
		c.client,
		(*C.char)(unsafe.Pointer(&buf[0])),
		C.size_t(len(buf)))
	err := c.errorFromCode(code)
	if err != nil {
		return "", err
	}
	n := bytes.IndexByte(buf, 0)
	if n < 0 {
		n = len(buf)
	}
	return string(buf[:n]), nil
}

// ExportStateChangedTokenEncrypted exports the state-changed token as an encrypted string
func (c *Client) ExportStateChangedTokenEncrypted() (string, error) {
	if c.client == nil {
		return "", ErrNotInitialized
	}
	buf := make([]byte, 65536)
	code := C.dl_client_export_state_changed_token_encrypted(
		c.client,
		(*C.char)(unsafe.Pointer(&buf[0])),
		C.size_t(len(buf)))
	err := c.errorFromCode(code)
	if err != nil {
		return "", err
	}
	n := bytes.IndexByte(buf, 0)
	if n < 0 {
		n = len(buf)
	}
	return string(buf[:n]), nil
}

// errorFromCode converts C error code to Go error
func (c *Client) errorFromCode(code C.DL_ErrorCode) error {
	switch code {
	case C.DL_ERROR_SUCCESS:
		return nil
	case C.DL_ERROR_INVALID_ARGUMENT:
		return ErrInvalidArgument
	case C.DL_ERROR_NOT_INITIALIZED:
		return ErrNotInitialized
	case C.DL_ERROR_ALREADY_INITIALIZED:
		return ErrAlreadyInitialized
	case C.DL_ERROR_NETWORK_ERROR:
		return ErrNetworkError
	case C.DL_ERROR_CRYPTO_ERROR:
		return ErrCryptoError
	case C.DL_ERROR_UNKNOWN_ERROR:
		return ErrUnknownError
	default:
		return ErrUnknownError
	}
}
