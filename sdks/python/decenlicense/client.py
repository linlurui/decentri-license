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
        Activate the license.
        
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
            'message': result.message.decode('utf-8') if result.message else '',
            'token': result.token  # This would need further processing
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