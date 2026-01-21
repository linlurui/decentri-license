#![allow(non_camel_case_types, non_upper_case_globals, non_snake_case)]

//! Rust bindings for the DecentriLicense C++ SDK
//!
//! This crate provides safe Rust bindings for the DecentriLicense C++ SDK
//! through the C binding layer.

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::ffi::{CStr, CString};
use std::ptr;

/// DecentriLicense client
pub struct DecentriLicenseClient {
    client: *mut DL_Client,
}

impl DecentriLicenseClient {
    /// Create a new DecentriLicense client
    pub fn new() -> Result<Self, DecentriLicenseError> {
        let client = unsafe { dl_client_create() };
        if client.is_null() {
            Err(DecentriLicenseError::CreationFailed)
        } else {
            Ok(Self { client })
        }
    }

    /// Initialize the client with configuration
    pub fn initialize(
        &self,
        license_code: &str,
        udp_port: u16,
        tcp_port: u16,
        registry_server_url: Option<&str>,
    ) -> Result<(), DecentriLicenseError> {
        let c_license_code = CString::new(license_code)
            .map_err(|_| DecentriLicenseError::InvalidArgument)?;
        
        let c_registry_server_url = registry_server_url
            .map(|s| CString::new(s).map_err(|_| DecentriLicenseError::InvalidArgument))
            .transpose()?;

        let config = DL_ClientConfig {
            license_code: c_license_code.as_ptr(),
            preferred_mode: DL_ConnectionMode_DL_CONNECTION_MODE_LAN_P2P,
            udp_port,
            tcp_port,
            registry_server_url: c_registry_server_url
                .as_ref()
                .map_or(ptr::null(), |s| s.as_ptr()),
        };

        let result = unsafe { dl_client_initialize(self.client, &config) };
        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }

    pub fn set_product_public_key(&self, product_public_key_file_content: &str) -> Result<(), DecentriLicenseError> {
        let c_content = CString::new(product_public_key_file_content)
            .map_err(|_| DecentriLicenseError::InvalidArgument)?;

        let result = unsafe { dl_client_set_product_public_key(self.client, c_content.as_ptr()) };
        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }

    pub fn import_token(&self, token_input: &str) -> Result<(), DecentriLicenseError> {
        let c_input = CString::new(token_input)
            .map_err(|_| DecentriLicenseError::InvalidArgument)?;

        let result = unsafe { dl_client_import_token(self.client, c_input.as_ptr()) };
        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }

    pub fn get_current_token_json(&self) -> Result<String, DecentriLicenseError> {
        let mut buf = vec![0u8; 1024 * 64];
        let result = unsafe {
            dl_client_get_current_token_json(
                self.client,
                buf.as_mut_ptr() as *mut i8,
                buf.len(),
            )
        };

        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            let c_str = unsafe { CStr::from_ptr(buf.as_ptr() as *const i8) };
            Ok(c_str.to_string_lossy().into_owned())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }

    pub fn export_current_token_encrypted(&self) -> Result<String, DecentriLicenseError> {
        let mut buf = vec![0u8; 1024 * 64];
        let result = unsafe {
            dl_client_export_current_token_encrypted(
                self.client,
                buf.as_mut_ptr() as *mut i8,
                buf.len(),
            )
        };

        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            let c_str = unsafe { CStr::from_ptr(buf.as_ptr() as *const i8) };
            Ok(c_str.to_string_lossy().into_owned())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }

    pub fn offline_verify_current_token(&self) -> Result<VerificationResult, DecentriLicenseError> {
        let mut result = unsafe { std::mem::zeroed::<DL_VerificationResult>() };
        let rc = unsafe { dl_client_offline_verify_current_token(self.client, &mut result) };
        if rc == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(VerificationResult::from(result))
        } else {
            Err(DecentriLicenseError::from(rc))
        }
    }

    pub fn get_status(&self) -> Result<StatusResult, DecentriLicenseError> {
        let mut status = unsafe { std::mem::zeroed::<DL_StatusResult>() };
        let rc = unsafe { dl_client_get_status(self.client, &mut status) };
        if rc == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(StatusResult::from(status))
        } else {
            Err(DecentriLicenseError::from(rc))
        }
    }

    pub fn activate_bind_device(&self) -> Result<VerificationResult, DecentriLicenseError> {
        let mut result = unsafe { std::mem::zeroed::<DL_VerificationResult>() };
        let rc = unsafe { dl_client_activate_bind_device(self.client, &mut result) };
        if rc == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(VerificationResult::from(result))
        } else {
            Err(DecentriLicenseError::from(rc))
        }
    }

    pub fn record_usage(&self, new_state_payload_json: &str) -> Result<VerificationResult, DecentriLicenseError> {
        let c_payload = CString::new(new_state_payload_json)
            .map_err(|_| DecentriLicenseError::InvalidArgument)?;
        let mut result = unsafe { std::mem::zeroed::<DL_VerificationResult>() };
        let rc = unsafe { dl_client_record_usage(self.client, c_payload.as_ptr(), &mut result) };
        if rc == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(VerificationResult::from(result))
        } else {
            Err(DecentriLicenseError::from(rc))
        }
    }

    /// Activate the license
    pub fn activate(&self) -> Result<ActivationResult, DecentriLicenseError> {
        let mut result = unsafe { std::mem::zeroed::<DL_ActivationResult>() };
        let error_code = unsafe { dl_client_activate(self.client, &mut result) };
        
        if error_code == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(ActivationResult::from(result))
        } else {
            Err(DecentriLicenseError::from(error_code))
        }
    }

    /// Get the current token if activated
    pub fn get_current_token(&self) -> Result<Option<Token>, DecentriLicenseError> {
        let mut token = unsafe { std::mem::zeroed::<DL_Token>() };
        let error_code = unsafe { dl_client_get_current_token(self.client, &mut token) };
        
        if error_code == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(Some(Token::from(token)))
        } else if error_code == DL_ErrorCode_DL_ERROR_UNKNOWN_ERROR {
            // No token available
            Ok(None)
        } else {
            Err(DecentriLicenseError::from(error_code))
        }
    }

    /// Check if the license is activated
    pub fn is_activated(&self) -> bool {
        let result = unsafe { dl_client_is_activated(self.client) };
        result != 0
    }

    /// Get the device ID
    pub fn get_device_id(&self) -> Result<String, DecentriLicenseError> {
        let mut buffer = [0u8; 256];
        let error_code = unsafe {
            dl_client_get_device_id(
                self.client,
                buffer.as_mut_ptr() as *mut i8,
                buffer.len(),
            )
        };

        if error_code == DL_ErrorCode_DL_ERROR_SUCCESS {
            let c_str = unsafe { CStr::from_ptr(buffer.as_ptr() as *const i8) };
            Ok(c_str.to_string_lossy().into_owned())
        } else {
            Err(DecentriLicenseError::from(error_code))
        }
    }

    /// Get the device state
    pub fn get_device_state(&self) -> DeviceState {
        let state = unsafe { dl_client_get_device_state(self.client) };
        DeviceState::from(state)
    }

    /// Shutdown the client
    pub fn shutdown(&self) -> Result<(), DecentriLicenseError> {
        let result = unsafe { dl_client_shutdown(self.client) };
        if result == DL_ErrorCode_DL_ERROR_SUCCESS {
            Ok(())
        } else {
            Err(DecentriLicenseError::from(result))
        }
    }
}

impl Drop for DecentriLicenseClient {
    fn drop(&mut self) {
        if !self.client.is_null() {
            unsafe { dl_client_destroy(self.client) };
        }
    }
}

/// Activation result
#[derive(Debug, Clone)]
pub struct ActivationResult {
    pub success: bool,
    pub message: String,
    pub token: Option<Token>,
}

impl From<DL_ActivationResult> for ActivationResult {
    fn from(result: DL_ActivationResult) -> Self {
        // For arrays, we need to handle them differently
        let c_str = unsafe { CStr::from_ptr(result.message.as_ptr()) };
        let message = c_str.to_string_lossy().into_owned();

        Self {
            success: result.success != 0,
            message,
            token: if !result.token.is_null() {
                Some(Token::from(unsafe { *result.token }))
            } else {
                None
            },
        }
    }
}

#[derive(Debug, Clone)]
pub struct VerificationResult {
    pub valid: bool,
    pub error_message: String,
}

impl From<DL_VerificationResult> for VerificationResult {
    fn from(result: DL_VerificationResult) -> Self {
        Self {
            valid: result.valid != 0,
            error_message: c_array_to_string(&result.error_message),
        }
    }
}

#[derive(Debug, Clone)]
pub struct StatusResult {
    pub has_token: bool,
    pub is_activated: bool,
    pub issue_time: i64,
    pub expire_time: i64,
    pub state_index: u64,
    pub token_id: String,
    pub holder_device_id: String,
    pub app_id: String,
    pub license_code: String,
}

impl From<DL_StatusResult> for StatusResult {
    fn from(status: DL_StatusResult) -> Self {
        Self {
            has_token: status.has_token != 0,
            is_activated: status.is_activated != 0,
            issue_time: status.issue_time,
            expire_time: status.expire_time,
            state_index: status.state_index,
            token_id: c_array_to_string(&status.token_id),
            holder_device_id: c_array_to_string(&status.holder_device_id),
            app_id: c_array_to_string(&status.app_id),
            license_code: c_array_to_string(&status.license_code),
        }
    }
}

/// License token
#[derive(Debug, Clone)]
pub struct Token {
    pub token_id: String,
    pub holder_device_id: String,
    pub issue_time: i64,
    pub expire_time: i64,
    pub signature: String,
}

impl From<DL_Token> for Token {
    fn from(token: DL_Token) -> Self {
        Self {
            token_id: c_array_to_string(&token.token_id),
            holder_device_id: c_array_to_string(&token.holder_device_id),
            issue_time: token.issue_time,
            expire_time: token.expire_time,
            signature: c_array_to_string(&token.signature),
        }
    }
}

/// Device state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeviceState {
    Idle,
    Discovering,
    Electing,
    Coordinator,
    Follower,
}

impl From<u32> for DeviceState {
    fn from(state: u32) -> Self {
        match state {
            0 => DeviceState::Idle,
            1 => DeviceState::Discovering,
            2 => DeviceState::Electing,
            3 => DeviceState::Coordinator,
            4 => DeviceState::Follower,
            _ => DeviceState::Idle,
        }
    }
}

/// DecentriLicense error
#[derive(Debug, Clone)]
pub enum DecentriLicenseError {
    InvalidArgument,
    NotInitialized,
    AlreadyInitialized,
    NetworkError,
    CryptoError,
    UnknownError,
    CreationFailed,
}

impl From<u32> for DecentriLicenseError {
    fn from(code: u32) -> Self {
        match code {
            1 => DecentriLicenseError::InvalidArgument,
            2 => DecentriLicenseError::NotInitialized,
            3 => DecentriLicenseError::AlreadyInitialized,
            4 => DecentriLicenseError::NetworkError,
            5 => DecentriLicenseError::CryptoError,
            6 => DecentriLicenseError::UnknownError,
            _ => DecentriLicenseError::UnknownError,
        }
    }
}

impl std::fmt::Display for DecentriLicenseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DecentriLicenseError::InvalidArgument => write!(f, "Invalid argument"),
            DecentriLicenseError::NotInitialized => write!(f, "Not initialized"),
            DecentriLicenseError::AlreadyInitialized => write!(f, "Already initialized"),
            DecentriLicenseError::NetworkError => write!(f, "Network error"),
            DecentriLicenseError::CryptoError => write!(f, "Crypto error"),
            DecentriLicenseError::UnknownError => write!(f, "Unknown error"),
            DecentriLicenseError::CreationFailed => write!(f, "Client creation failed"),
        }
    }
}

impl std::error::Error for DecentriLicenseError {}

/// Helper function to convert C array to String
fn c_array_to_string<const N: usize>(array: &[i8; N]) -> String {
    let c_str = unsafe { CStr::from_ptr(array.as_ptr()) };
    c_str.to_string_lossy().into_owned()
}