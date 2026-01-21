//! DecentriLicense Rust SDK Example

use decenlicense::{DecentriLicenseClient, DecentriLicenseError};

fn main() -> Result<(), DecentriLicenseError> {
    // License code to use
    let license_code = std::env::args().nth(1).unwrap_or_else(|| "EXAMPLE-LICENSE-12345".to_string());
    
    println!("==========================================");
    println!("DecentriLicense Rust SDK Example");
    println!("==========================================");
    println!("Using license code: {}", license_code);
    println!();
    
    // Create and initialize the client
    let client = DecentriLicenseClient::new()?;
    
    println!("[1] Initializing client...");
    client.initialize(&license_code, 8888, 8889, None)?;
    println!("    Device ID: {}", client.get_device_id()?);
    println!("    Initialization successful!");
    println!();
    
    // Activate the license
    println!("[2] Activating license...");
    println!("    Broadcasting discovery on LAN...");
    println!("    Waiting for peers and election...");
    println!();
    
    match client.activate() {
        Ok(result) => {
            println!("    Success: {}", result.success);
            println!("    Message: {}", result.message);
        }
        Err(e) => {
            println!("    Activation failed: {}", e);
        }
    }
    println!();
    
    // Check activation status
    println!("[3] Current status:");
    println!("    Activated: {}", client.is_activated());
    println!("    Device State: {:?}", client.get_device_state());
    println!();
    
    // Get current token if available
    println!("[4] Current token:");
    match client.get_current_token()? {
        Some(token) => {
            println!("    Token ID: {}", token.token_id);
            println!("    Holder: {}", token.holder_device_id);
            println!("    License: {}", license_code);
        }
        None => {
            println!("    No token available");
        }
    }
    println!();
    
    // Clean up
    client.shutdown()?;
    
    println!("[5] Example completed successfully!");
    Ok(())
}