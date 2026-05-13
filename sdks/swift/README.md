# DecentriLicense Swift SDK

Swift wrapper for the DecentriLicense `dl-core` C library via SPM.

## Structure

```
swift/
├── Package.swift
├── Sources/
│   ├── CDecentriLicense/
│   │   ├── decenlicense_c.h        # C header for dl-core
│   │   ├── libdecentrilicense.dylib
│   │   └── module.modulemap        # System library module for dl-core
│   ├── DecentriLicense/
│   │   ├── DecentriLicenseClient.swift  # Swift client wrapper
│   │   └── CryptoUtils.swift           # Crypto utility functions
│   └── ValidationWizard/
│       └── main.swift              # Interactive validation wizard CLI
```

## Build

```bash
# Build dl-core first
cd dl-core && mkdir build && cd build && cmake .. && cmake --build .

# Build Swift SDK
cd sdks/swift
PKG_CONFIG_PATH=../../dl-core/build swift build

# Or compile directly
swiftc -I../../dl-core/include \
       -L../../dl-core/build \
       -ldecentrilicense \
       -o validation_wizard \
       Sources/DecentriLicense/DecentriLicenseClient.swift \
       Sources/DecentriLicense/CryptoUtils.swift \
       Sources/ValidationWizard/main.swift

# Run
DYLD_LIBRARY_PATH=../../dl-core/build ./validation_wizard
```

## Quick Validate

```bash
./validation_wizard validate <token_file> <product_public_key_file>
```

## Usage

### Basic Usage

```swift
import DecentriLicense

let client = try DecentriLicenseClient()

// Initialize with config
let config = DLConfig(
    licenseCode: "TEST-2025-001",
    preferredMode: .offline,
    udpPort: 13325,
    tcpPort: 23325
)
try client.initialize(config)

// Set product public key
try client.setProductPublicKey(productKeyPEM)

// Import and verify token
try client.importToken(tokenString)
let result = try client.offlineVerifyCurrentToken()
if result.valid {
    print("✅ Token is valid")
    let status = try client.getStatus()
    print("Token ID: \(status.tokenId)")
}

try client.shutdown()
client.close()
```

### Activate with Token

```swift
// One-step import + activate
let success = try client.activateWithToken(tokenString)
if success {
    print("✅ Activation successful")
}
```

### Validate Token (without activating)

```swift
let result = try client.validateToken(tokenString)
print(result.valid ? "✅ Valid" : "❌ Invalid: \(result.errorMessage)")
```

### Get Current Token

```swift
let token = try client.getCurrentToken()
print("Token ID: \(token.tokenId)")
print("Signature: \(token.signature)")
```

### Recovery Channel

```swift
// Add password-based recovery channel
let result = try client.addRecoveryChannel(password: "my-password")
print(result.valid ? "✅ Recovery channel added" : "❌ Failed")

// Remove recovery channel
let removeResult = try client.removeRecoveryChannel()
```

### Get State Payload

```swift
let payload = try client.getStatePayload()
print("State payload: \(payload)")
```

### Crypto Utils

```swift
let hash = computeLicenseKeyHash(publicKeyPEM)
// Returns SHA256 hash Data for use as AES key
```

## API Reference

| Method | Description |
|--------|-------------|
| `initialize(_:)` | Initialize client with `DLConfig` |
| `setProductPublicKey(_:)` | Set product public key (PEM content) |
| `importToken(_:)` | Import token (encrypted string or JSON) |
| `offlineVerifyCurrentToken()` | Offline verify current token |
| `validateToken(_:)` | Validate token without activating |
| `activateWithToken(_:)` | Import + activate in one step |
| `activate()` | Activate license (legacy, may require network) |
| `activateBindDevice()` | Activate and bind device |
| `getCurrentToken()` | Get current token details |
| `getStatus()` | Get client status |
| `getStatePayload()` | Get plaintext state payload |
| `recordUsage(_:)` | Record usage / state change |
| `addRecoveryChannel(password:)` | Add password-encrypted SEK channel |
| `removeRecoveryChannel()` | Remove recovery channel |
| `exportCurrentTokenEncrypted()` | Export current token encrypted |
| `exportActivatedTokenEncrypted()` | Export activated token encrypted |
| `exportStateChangedTokenEncrypted()` | Export state-changed token encrypted |
| `isActivated()` | Check if license is activated |
| `getDeviceId()` | Get device ID |
| `getDeviceState()` | Get device state string |
| `shutdown()` | Shutdown client |
| `close()` | Close client and release resources |
