# DecentriLicense Rust SDK

Rust bindings for the DecentriLicense C++ SDK.

## Installation

To use the Rust SDK, you first need to build the C++ core library and C bindings:

```bash
# From the project root directory
mkdir build
cd build
cmake ..
make
```

Then you can use the Rust package:

```bash
# From the bindings/rust directory
cargo build
```

## Usage

```rust
use decentrilicense::{DecentriLicenseClient, ClientConfig};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create a new client
    let mut client = DecentriLicenseClient::new()?;
    
    // Initialize the client
    let config = ClientConfig {
        license_code: "YOUR-LICENSE-CODE".to_string(),
        udp_port: 8888,
        tcp_port: 8889,
        registry_server_url: None,
    };
    
    client.initialize(config)?;
    
    // Activate the license
    let result = client.activate()?;
    if result.success {
        println!("License activated successfully");
        
        // Get current token
        if let Some(token) = client.get_current_token()? {
            println!("Token ID: {}", token.token_id);
        }
    }
    
    Ok(())
}
```

## Package Installation

To use the pre-built package:

### Method 1: Using Cargo with local path
Add the following to your Cargo.toml:
```toml
[dependencies]
decentrilicense = { path = "path/to/decentrilicense-rust" }
```

### Method 2: Using Cargo with version
```toml
[dependencies]
decentrilicense = "1.0.0"
```

### Method 3: From source code
1. Extract the source package:
   ```bash
   tar -xzf decentrilicense-rust-src-1.0.0.tar.gz
   ```
2. Navigate to the extracted directory:
   ```bash
   cd decentrilicense-rust-src-1.0.0
   ```
3. Build:
   ```bash
   cargo build --release
   ```

## API Reference

### DecentriLicenseClient
- `new()` - Create a new DecentriLicense client
- `initialize(config: ClientConfig)` - Initialize the client with configuration
- `activate()` - Activate the license
- `get_current_token()` - Get the current token if activated
- `is_activated()` - Check if the license is activated
- `get_device_id()` - Get the device ID
- `get_device_state()` - Get the current device state
- `shutdown()` - Shutdown the client

### ClientConfig
- `license_code` - The license code string
- `udp_port` - UDP port for discovery (default: 8888)
- `tcp_port` - TCP port for P2P communication (default: 8889)
- `registry_server_url` - Optional registry server URL

### DecentriLicenseError
Error enum for license-related errors.

## Example

See the examples directory for complete examples of how to use the SDK.