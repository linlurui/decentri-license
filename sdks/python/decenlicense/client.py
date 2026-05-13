"""
DecentriLicense Python Client
===========================

High-level Python client for the DecentriLicense SDK.
"""

import ctypes
from typing import Optional
from ._decenlicense import *


class LicenseError(Exception):
    """Exception raised for license-related errors."""
    pass


class DecentriLicenseClient:
    """
    A Python client for the DecentriLicense SDK.
    
    This client provides a Pythonic interface to the underlying C++ SDK
    through the C bindings.
    """
    
    def __init__(self):
        """Initialize a new DecentriLicense client."""
        self._client = dl_client_create()
        if not self._client:
            raise LicenseError("Failed to create DecentriLicense client")
        self._initialized = False
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.shutdown()
    
    def initialize(self, license_code: str, udp_port: int = 13325, tcp_port: int = 23325,
                   registry_server_url: Optional[str] = None) -> None:
        """
        Initialize the client with configuration.
        
        Args:
            license_code: The license code to use
            udp_port: UDP port for discovery (default: 8888)
            tcp_port: TCP port for P2P communication (default: 8889)
            registry_server_url: Optional registry server URL
            
        Raises:
            LicenseError: If initialization fails
        """
        if self._initialized:
            raise LicenseError("Client already initialized")
        
        config = DL_ClientConfig()
        config.license_code = license_code.encode('utf-8')
        config.preferred_mode = 1  # DL_CONNECTION_MODE_LAN_P2P
        config.udp_port = udp_port
        config.tcp_port = tcp_port
        config.registry_server_url = (
            registry_server_url.encode('utf-8') if registry_server_url else None
        )
        
        result = dl_client_initialize(self._client, ctypes.byref(config))
        if result != DL_ERROR_SUCCESS:
            raise LicenseError(f"Initialization failed with error code: {result}")
        
        self._initialized = True
    
    def activate(self) -> dict:
        """
        Activate the license (may require network/P2P).
        
        Returns:
            A dictionary with activation result information
            
        Raises:
            LicenseError: If the client is not initialized
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        result = DL_ActivationResult()
        error_code = dl_client_activate(self._client, ctypes.byref(result))
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Activation failed with error code: {error_code}")
        
        return {
            'success': bool(result.success),
            'message': result.message.decode('utf-8').rstrip('\x00') if result.message else '',
        }

    def activate_with_token(self, token_string: str) -> dict:
        """
        Activate the license with an offline token string.
        
        This imports the token and then activates it in one call,
        equivalent to Go's ActivateWithToken.
        
        Args:
            token_string: The token string (encrypted or JSON format)
            
        Returns:
            A dictionary with activation result information
            
        Raises:
            LicenseError: If the client is not initialized or activation fails
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        result = DL_ActivationResult()
        error_code = dl_client_activate_with_token(
            self._client,
            token_string.encode('utf-8'),
            ctypes.byref(result)
        )
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Activate with token failed with error code: {error_code}")
        
        if not bool(result.success):
            msg = result.message.decode('utf-8').rstrip('\x00') if result.message else 'Unknown error'
            raise LicenseError(f"Activation failed: {msg}")
        
        return {
            'success': True,
            'message': result.message.decode('utf-8').rstrip('\x00') if result.message else '',
        }

    def validate_token(self, token_string: str) -> dict:
        """
        Validate a token string without activating it.
        
        This is the standard offline validation flow:
        1. Import the token into the client
        2. Verify the imported token offline
        
        Equivalent to Go's ValidateToken method.
        
        Args:
            token_string: The token string (encrypted, raw, or JSON format)
            
        Returns:
            A dictionary with 'valid' (bool) and 'error_message' (str)
            
        Raises:
            LicenseError: If the client is not initialized or import fails
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        # Step 1: Import the token
        error_code = dl_client_import_token(self._client, token_string.encode('utf-8'))
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Import token failed with error code: {error_code}")
        
        # Step 2: Verify the imported token offline
        result = DL_VerificationResult()
        error_code = dl_client_offline_verify_current_token(self._client, ctypes.byref(result))
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Verify token failed with error code: {error_code}")
        
        return {
            'valid': bool(result.valid),
            'error_message': result.error_message.decode('utf-8').rstrip('\x00'),
        }

    def verify_token_trust_chain(self, token: dict, root_public_key_pem: str) -> dict:
        """
        Verify a token using the trust chain model.
        
        This verifies:
        1. The root signature of the license public key using the root public key
        2. The token signature using the verified license public key
        
        Args:
            token: Token dictionary (from get_current_token() or parsed JSON)
            root_public_key_pem: Root public key PEM content
            
        Returns:
            A dictionary with 'valid' (bool) and 'error_message' (str)
            
        Raises:
            LicenseError: If the client is not initialized or verification fails
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        # Build DL_Token struct from dict
        c_token = DL_Token()
        c_token.token_id = token.get('token_id', '').encode('utf-8')[:127]
        c_token.holder_device_id = token.get('holder_device_id', '').encode('utf-8')[:255]
        c_token.issue_time = token.get('issue_time', 0)
        c_token.expire_time = token.get('expire_time', 0)
        c_token.signature = token.get('signature', '').encode('utf-8')[:511]
        c_token.license_public_key = token.get('license_public_key', '').encode('utf-8')[:1023]
        c_token.root_signature = token.get('root_signature', '').encode('utf-8')[:511]
        c_token.app_id = token.get('app_id', '').encode('utf-8')[:127]
        c_token.license_code = token.get('license_code', '').encode('utf-8')[:127]
        
        result = DL_VerificationResult()
        error_code = dl_client_verify_token_trust_chain(
            self._client,
            ctypes.byref(c_token),
            root_public_key_pem.encode('utf-8'),
            ctypes.byref(result)
        )
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Trust chain verification failed with error code: {error_code}")
        
        return {
            'valid': bool(result.valid),
            'error_message': result.error_message.decode('utf-8').rstrip('\x00'),
        }

    def set_product_public_key(self, product_public_key_file_content: str) -> None:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        rc = dl_client_set_product_public_key(self._client, product_public_key_file_content.encode('utf-8'))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Set product public key failed with error code: {rc}")

    def import_token(self, token_input: str) -> None:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        rc = dl_client_import_token(self._client, token_input.encode('utf-8'))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Import token failed with error code: {rc}")

    def get_current_token_json(self) -> str:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        buf = ctypes.create_string_buffer(65536)
        rc = dl_client_get_current_token_json(self._client, ctypes.cast(buf, ctypes.c_void_p), ctypes.sizeof(buf))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Get current token json failed with error code: {rc}")
        return buf.value.decode('utf-8')

    def export_current_token_encrypted(self) -> str:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        buf = ctypes.create_string_buffer(65536)
        rc = dl_client_export_current_token_encrypted(self._client, ctypes.cast(buf, ctypes.c_void_p), ctypes.sizeof(buf))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Export current token encrypted failed with error code: {rc}")
        return buf.value.decode('utf-8')

    def offline_verify_current_token(self) -> dict:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        vr = DL_VerificationResult()
        rc = dl_client_offline_verify_current_token(self._client, ctypes.byref(vr))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Offline verify failed with error code: {rc}")
        return {
            'valid': bool(vr.valid),
            'error_message': vr.error_message.decode('utf-8').rstrip('\x00'),
        }

    def get_status(self) -> dict:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        st = DL_StatusResult()
        rc = dl_client_get_status(self._client, ctypes.byref(st))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Get status failed with error code: {rc}")
        return {
            'has_token': bool(st.has_token),
            'is_activated': bool(st.is_activated),
            'issue_time': st.issue_time,
            'expire_time': st.expire_time,
            'state_index': int(st.state_index),
            'token_id': st.token_id.decode('utf-8').rstrip('\x00'),
            'holder_device_id': st.holder_device_id.decode('utf-8').rstrip('\x00'),
            'app_id': st.app_id.decode('utf-8').rstrip('\x00'),
            'license_code': st.license_code.decode('utf-8').rstrip('\x00'),
        }

    def activate_bind_device(self) -> dict:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        vr = DL_VerificationResult()
        rc = dl_client_activate_bind_device(self._client, ctypes.byref(vr))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Activate bind device failed with error code: {rc}")
        return {
            'valid': bool(vr.valid),
            'error_message': vr.error_message.decode('utf-8').rstrip('\x00'),
        }

    def record_usage(self, new_state_payload_json: str) -> dict:
        if not self._initialized:
            raise LicenseError("Client not initialized")
        vr = DL_VerificationResult()
        rc = dl_client_record_usage(self._client, new_state_payload_json.encode('utf-8'), ctypes.byref(vr))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Record usage failed with error code: {rc}")
        return {
            'valid': bool(vr.valid),
            'error_message': vr.error_message.decode('utf-8').rstrip('\x00'),
        }

    def export_activated_token_encrypted(self) -> str:
        """Export the activated token as an encrypted string."""
        if not self._initialized:
            raise LicenseError("Client not initialized")
        buf = ctypes.create_string_buffer(65536)
        rc = dl_client_export_activated_token_encrypted(
            self._client,
            ctypes.cast(buf, ctypes.c_void_p),
            ctypes.sizeof(buf))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Export activated token failed with error code: {rc}")
        return buf.value.decode('utf-8')

    def export_state_changed_token_encrypted(self) -> str:
        """Export the state-changed token as an encrypted string."""
        if not self._initialized:
            raise LicenseError("Client not initialized")
        buf = ctypes.create_string_buffer(65536)
        rc = dl_client_export_state_changed_token_encrypted(
            self._client,
            ctypes.cast(buf, ctypes.c_void_p),
            ctypes.sizeof(buf))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Export state changed token failed with error code: {rc}")
        return buf.value.decode('utf-8')

    def get_current_token(self) -> Optional[dict]:
        """
        Get the current token if activated.
        
        Returns:
            A dictionary with token information or None if not activated
            
        Raises:
            LicenseError: If the client is not initialized
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        token = DL_Token()
        error_code = dl_client_get_current_token(self._client, ctypes.byref(token))
        if error_code != DL_ERROR_SUCCESS:
            return None
        
        return {
            'token_id': token.token_id.decode('utf-8').rstrip('\x00'),
            'holder_device_id': token.holder_device_id.decode('utf-8').rstrip('\x00'),
            'issue_time': token.issue_time,
            'expire_time': token.expire_time,
            'signature': token.signature.decode('utf-8').rstrip('\x00')
        }
    
    def is_activated(self) -> bool:
        """
        Check if the license is currently activated.
        
        Returns:
            True if activated, False otherwise
        """
        if not self._initialized:
            return False
        return bool(dl_client_is_activated(self._client))
    
    def get_device_id(self) -> str:
        """
        Get the device ID.
        
        Returns:
            The device ID as a string
            
        Raises:
            LicenseError: If the client is not initialized
        """
        if not self._initialized:
            raise LicenseError("Client not initialized")
        
        device_id = ctypes.create_string_buffer(256)
        error_code = dl_client_get_device_id(
            self._client, ctypes.cast(device_id, ctypes.c_void_p), ctypes.sizeof(device_id)
        )
        if error_code != DL_ERROR_SUCCESS:
            raise LicenseError(f"Failed to get device ID with error code: {error_code}")
        
        return device_id.value.decode('utf-8').rstrip('\x00')
    
    def get_device_state(self) -> str:
        """
        Get the current device state.
        
        Returns:
            The device state as a string
        """
        state = dl_client_get_device_state(self._client)
        state_map = {
            DL_DEVICE_STATE_IDLE: 'IDLE',
            DL_DEVICE_STATE_DISCOVERING: 'DISCOVERING',
            DL_DEVICE_STATE_ELECTING: 'ELECTING',
            DL_DEVICE_STATE_COORDINATOR: 'COORDINATOR',
            DL_DEVICE_STATE_FOLLOWER: 'FOLLOWER'
        }
        return state_map.get(state, 'UNKNOWN')
    
    def get_state_payload(self) -> str:
        """Get plaintext state_payload (automatically decrypted from SEK if applicable)."""
        if not self._initialized:
            raise LicenseError("Client not initialized")
        buf = ctypes.create_string_buffer(65536)
        rc = dl_client_get_state_payload(self._client, ctypes.cast(buf, ctypes.c_void_p), ctypes.sizeof(buf))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Get state payload failed with error code: {rc}")
        return buf.value.decode('utf-8')

    def add_recovery_channel(self, password: str) -> dict:
        """Add recovery channel (password/mnemonic) to wrap SEK for migration/recovery."""
        if not self._initialized:
            raise LicenseError("Client not initialized")
        vr = DL_VerificationResult()
        rc = dl_client_add_recovery_channel(self._client, password.encode('utf-8'), ctypes.byref(vr))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Add recovery channel failed with error code: {rc}")
        return {
            'valid': bool(vr.valid),
            'error_message': vr.error_message.decode('utf-8').rstrip('\x00'),
        }

    def remove_recovery_channel(self) -> dict:
        """Remove recovery channel (clears password-encrypted SEK)."""
        if not self._initialized:
            raise LicenseError("Client not initialized")
        vr = DL_VerificationResult()
        rc = dl_client_remove_recovery_channel(self._client, ctypes.byref(vr))
        if rc != DL_ERROR_SUCCESS:
            raise LicenseError(f"Remove recovery channel failed with error code: {rc}")
        return {
            'valid': bool(vr.valid),
            'error_message': vr.error_message.decode('utf-8').rstrip('\x00'),
        }

    def shutdown(self) -> None:
        """
        Shutdown the client and release resources.
        """
        if self._client:
            if self._initialized:
                dl_client_shutdown(self._client)
            dl_client_destroy(self._client)
            self._client = None
            self._initialized = False