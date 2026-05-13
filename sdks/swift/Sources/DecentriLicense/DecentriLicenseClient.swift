import Foundation
import CDecentriLicense

/// Verification result
public struct DLVerificationResult {
    public let valid: Bool
    public let errorMessage: String
}

/// Status result
public struct DLStatusResult {
    public let hasToken: Bool
    public let isActivated: Bool
    public let issueTime: Int64
    public let expireTime: Int64
    public let stateIndex: UInt64
    public let tokenId: String
    public let holderDeviceId: String
    public let appId: String
    public let licenseCode: String
}

/// Connection mode
public enum DLConnectionMode: UInt32 {
    case wanRegistry = 0
    case lanP2P = 1
    case offline = 2
}

/// Client configuration
public struct DLConfig {
    public var licenseCode: String
    public var preferredMode: DLConnectionMode
    public var udpPort: UInt16
    public var tcpPort: UInt16
    public var registryServerURL: String

    public init(licenseCode: String = "",
                preferredMode: DLConnectionMode = .wanRegistry,
                udpPort: UInt16 = 13325,
                tcpPort: UInt16 = 23325,
                registryServerURL: String = "") {
        self.licenseCode = licenseCode
        self.preferredMode = preferredMode
        self.udpPort = udpPort
        self.tcpPort = tcpPort
        self.registryServerURL = registryServerURL
    }
}

/// Token structure
public struct DLToken {
    public let tokenId: String
    public let holderDeviceId: String
    public let issueTime: Int64
    public let expireTime: Int64
    public let signature: String
}

/// Activation result
public struct DLActivationResult {
    public let success: Bool
    public let message: String
}

/// License error
public enum LicenseError: Error, LocalizedError {
    case clientNotCreated
    case notInitialized
    case alreadyInitialized
    case failed(String, DL_ErrorCode)

    public var errorDescription: String? {
        switch self {
        case .clientNotCreated: return "Client not created"
        case .notInitialized: return "Client not initialized"
        case .alreadyInitialized: return "Client already initialized"
        case .failed(let ctx, let code): return "\(ctx): error code=\(code.rawValue)"
        }
    }
}

/// Helper to convert a fixed-size C char array (imported as tuple) to Swift String
private func cCharArrayToString<T>(_ ptr: UnsafePointer<T>) -> String {
    let count = MemoryLayout<T>.size
    var buffer = [CChar](repeating: 0, count: count + 1)
    memcpy(&buffer, UnsafeRawPointer(ptr), count)
    return buffer.withUnsafeBufferPointer { bufPtr -> String in
        String(cString: bufPtr.baseAddress!)
    }
}

/// DecentriLicense Swift Client
public class DecentriLicenseClient {
    private var clientPtr: OpaquePointer?
    private var initialized = false

    public init() throws {
        clientPtr = dl_client_create()
        guard clientPtr != nil else {
            throw LicenseError.clientNotCreated
        }
    }

    deinit {
        if let ptr = clientPtr {
            if initialized { _ = dl_client_shutdown(ptr) }
            dl_client_destroy(ptr)
        }
    }

    /// Initialize client with configuration
    public func initialize(_ config: DLConfig = DLConfig()) throws {
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        let licenseCodeStr = strdup(config.licenseCode)!
        let registryUrlStr = strdup(config.registryServerURL)!
        defer {
            free(licenseCodeStr)
            free(registryUrlStr)
        }

        var cConfig = DL_ClientConfig()
        cConfig.license_code = UnsafePointer(licenseCodeStr)
        switch config.preferredMode {
        case .wanRegistry: cConfig.preferred_mode = DL_CONNECTION_MODE_WAN_REGISTRY
        case .lanP2P: cConfig.preferred_mode = DL_CONNECTION_MODE_LAN_P2P
        case .offline: cConfig.preferred_mode = DL_CONNECTION_MODE_OFFLINE
        }
        cConfig.udp_port = config.udpPort
        cConfig.tcp_port = config.tcpPort
        cConfig.registry_server_url = UnsafePointer(registryUrlStr)

        let rc = dl_client_initialize(ptr, &cConfig)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_initialize", rc)
        }
        initialized = true
    }

    /// Set product public key (PEM content)
    public func setProductPublicKey(_ content: String) throws {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        let rc = dl_client_set_product_public_key(ptr, content)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_set_product_public_key", rc)
        }
    }

    /// Import token (encrypted string or JSON)
    public func importToken(_ tokenInput: String) throws {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        let rc = dl_client_import_token(ptr, tokenInput)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_import_token", rc)
        }
    }

    /// Offline verify current token
    public func offlineVerifyCurrentToken() throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var vr = DL_VerificationResult()
        let rc = dl_client_offline_verify_current_token(ptr, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_offline_verify_current_token", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Get client status
    public func getStatus() throws -> DLStatusResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var sr = DL_StatusResult()
        let rc = dl_client_get_status(ptr, &sr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_get_status", rc)
        }
        return DLStatusResult(
            hasToken: sr.has_token == 1,
            isActivated: sr.is_activated == 1,
            issueTime: sr.issue_time,
            expireTime: sr.expire_time,
            stateIndex: sr.state_index,
            tokenId: withUnsafePointer(to: sr.token_id) { cCharArrayToString($0) },
            holderDeviceId: withUnsafePointer(to: sr.holder_device_id) { cCharArrayToString($0) },
            appId: withUnsafePointer(to: sr.app_id) { cCharArrayToString($0) },
            licenseCode: withUnsafePointer(to: sr.license_code) { cCharArrayToString($0) }
        )
    }

    /// Activate and bind device
    public func activateBindDevice() throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var vr = DL_VerificationResult()
        let rc = dl_client_activate_bind_device(ptr, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_activate_bind_device", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Record usage / state change
    public func recordUsage(_ payloadJson: String) throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var vr = DL_VerificationResult()
        let rc = dl_client_record_usage(ptr, payloadJson, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_record_usage", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Export current token encrypted
    public func exportCurrentTokenEncrypted() throws -> String {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var buf = [CChar](repeating: 0, count: 65536)
        let rc = dl_client_export_current_token_encrypted(ptr, &buf, buf.count)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_export_current_token_encrypted", rc)
        }
        return String(cString: buf)
    }

    /// Export activated token encrypted
    public func exportActivatedTokenEncrypted() throws -> String {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var buf = [CChar](repeating: 0, count: 65536)
        let rc = dl_client_export_activated_token_encrypted(ptr, &buf, buf.count)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_export_activated_token_encrypted", rc)
        }
        return String(cString: buf)
    }

    /// Export state-changed token encrypted
    public func exportStateChangedTokenEncrypted() throws -> String {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var buf = [CChar](repeating: 0, count: 65536)
        let rc = dl_client_export_state_changed_token_encrypted(ptr, &buf, buf.count)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_export_state_changed_token_encrypted", rc)
        }
        return String(cString: buf)
    }

    /// Check if activated
    public func isActivated() -> Bool {
        guard let ptr = clientPtr else { return false }
        return dl_client_is_activated(ptr) == 1
    }

    /// Get device ID
    public func getDeviceId() throws -> String {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var buf = [CChar](repeating: 0, count: 256)
        let rc = dl_client_get_device_id(ptr, &buf, buf.count)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_get_device_id", rc)
        }
        return String(cString: buf)
    }

    /// Get device state string
    public func getDeviceState() -> String {
        guard let ptr = clientPtr else { return "idle" }
        let st = dl_client_get_device_state(ptr)
        switch st {
        case DL_DEVICE_STATE_DISCOVERING: return "discovering"
        case DL_DEVICE_STATE_ELECTING:    return "electing"
        case DL_DEVICE_STATE_COORDINATOR: return "coordinator"
        case DL_DEVICE_STATE_FOLLOWER:    return "follower"
        default:                          return "idle"
        }
    }

    // MARK: - New methods matching Go SDK

    /// Activate with token (import + activate in one step)
    public func activateWithToken(_ tokenString: String) throws -> Bool {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        // Import the token first
        let importRc = dl_client_import_token(ptr, tokenString)
        guard importRc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_import_token", importRc)
        }

        // Then activate
        var result = DL_ActivationResult()
        let rc = dl_client_activate(ptr, &result)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_activate", rc)
        }

        if result.success == 0 {
            let message = withUnsafePointer(to: result.message) { cCharArrayToString($0) }
            throw LicenseError.failed("Activation failed: " + message, DL_ERROR_UNKNOWN_ERROR)
        }

        return true
    }

    /// Validate token without activating it
    public func validateToken(_ tokenString: String) throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        let importRc = dl_client_import_token(ptr, tokenString)
        guard importRc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_import_token", importRc)
        }

        var vr = DL_VerificationResult()
        let rc = dl_client_offline_verify_current_token(ptr, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_offline_verify_current_token", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Activate license (legacy method, may require network)
    public func activate() throws -> Bool {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var result = DL_ActivationResult()
        let rc = dl_client_activate(ptr, &result)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_activate", rc)
        }
        return result.success != 0
    }

    /// Get current token
    public func getCurrentToken() throws -> DLToken {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var cToken = DL_Token()
        let rc = dl_client_get_current_token(ptr, &cToken)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_get_current_token", rc)
        }
        return DLToken(
            tokenId: withUnsafePointer(to: cToken.token_id) { cCharArrayToString($0) },
            holderDeviceId: withUnsafePointer(to: cToken.holder_device_id) { cCharArrayToString($0) },
            issueTime: cToken.issue_time,
            expireTime: cToken.expire_time,
            signature: withUnsafePointer(to: cToken.signature) { cCharArrayToString($0) }
        )
    }

    /// Get state payload (automatically decrypted from SEK if applicable)
    public func getStatePayload() throws -> String {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var buf = [CChar](repeating: 0, count: 65536)
        let rc = dl_client_get_state_payload(ptr, &buf, buf.count)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_get_state_payload", rc)
        }
        return String(cString: buf)
    }

    /// Add recovery channel (password/mnemonic) to wrap SEK
    public func addRecoveryChannel(password: String) throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var vr = DL_VerificationResult()
        let rc = dl_client_add_recovery_channel(ptr, password, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_add_recovery_channel", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Remove recovery channel (clears encrypted_sek_password)
    public func removeRecoveryChannel() throws -> DLVerificationResult {
        guard initialized else { throw LicenseError.notInitialized }
        guard let ptr = clientPtr else { throw LicenseError.clientNotCreated }

        var vr = DL_VerificationResult()
        let rc = dl_client_remove_recovery_channel(ptr, &vr)
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_remove_recovery_channel", rc)
        }
        return DLVerificationResult(
            valid: vr.valid == 1,
            errorMessage: withUnsafePointer(to: vr.error_message) { cCharArrayToString($0) }
        )
    }

    /// Close client and release resources (without shutdown)
    public func close() {
        if let ptr = clientPtr {
            dl_client_destroy(ptr)
            clientPtr = nil
        }
        initialized = false
    }

    /// Shutdown client
    public func shutdown() throws {
        guard let ptr = clientPtr else { return }
        let rc = dl_client_shutdown(ptr)
        initialized = false
        guard rc == DL_ERROR_SUCCESS else {
            throw LicenseError.failed("dl_client_shutdown", rc)
        }
    }
}
