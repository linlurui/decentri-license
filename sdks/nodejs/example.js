#!/usr/bin/env node

/**
 * DecentriLicense Node.js SDK Example
 */

const DecentriLicenseClient = require('./index.js');

async function main() {
    // License code to use
    const licenseCode = process.argv[2] || "EXAMPLE-LICENSE-12345";
    
    console.log("==========================================");
    console.log("DecentriLicense Node.js SDK Example");
    console.log("==========================================");
    console.log(`Using license code: ${licenseCode}`);
    console.log();
    
    try {
        // Create and initialize the client
        const client = new DecentriLicenseClient();
        
        console.log("[1] Initializing client...");
        const initResult = client.initialize({
            licenseCode: licenseCode,
            udpPort: 8888,
            tcpPort: 8889,
            registryServerUrl: ""
        });
        
        console.log(`    Device ID: ${client.getDeviceId()}`);
        console.log("    Initialization successful!");
        console.log();
        
        // Activate the license
        console.log("[2] Activating license...");
        console.log("    Broadcasting discovery on LAN...");
        console.log("    Waiting for peers and election...");
        console.log();
        
        try {
            const result = client.activate();
            console.log(`    Success: ${result.success}`);
            console.log(`    Message: ${result.message}`);
        } catch (error) {
            console.log(`    Activation failed: ${error.message}`);
        }
        console.log();
        
        // Check activation status
        console.log("[3] Current status:");
        console.log(`    Activated: ${client.isActivated()}`);
        console.log(`    Device State: ${client.getDeviceState()}`);
        console.log();
        
        // Get current token if available
        console.log("[4] Current token:");
        const token = client.getCurrentToken();
        if (token) {
            console.log(`    Token ID: ${token.tokenId}`);
            console.log(`    Holder: ${token.holderDeviceId}`);
            console.log(`    License: ${licenseCode}`);
        } else {
            console.log("    No token available");
        }
        console.log();
        
        // Clean up
        client.shutdown();
        
    } catch (error) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
    
    console.log("[5] Example completed successfully!");
    process.exit(0);
}

// Run the example
main().catch(console.error);