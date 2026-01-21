#!/usr/bin/env node

/**
 * DecentriLicense Node.js SDK Test Script
 */

const { DecentriLicenseClient } = require('./build/Release/decenlicense_node');

console.log("==========================================");
console.log("DecentriLicense Node.js SDK Test");
console.log("==========================================");

try {
    // Test 1: Create client instance
    console.log("[1] Creating client instance...");
    const client = new DecentriLicenseClient();
    console.log("    ✅ Client created successfully");

    // Test 2: Initialize client
    console.log("[2] Initializing client...");
    const initResult = client.initialize({
        licenseCode: "TEST-LICENSE-001",
        udpPort: 8888,
        tcpPort: 8889,
        registryServerUrl: ""
    });
    console.log(`    Success: ${initResult.success}`);
    console.log(`    Message: ${initResult.message}`);
    console.log("    ✅ Client initialized successfully");

    // Test 3: Check device ID
    console.log("[3] Checking device ID...");
    const deviceId = client.getDeviceId();
    console.log(`    Device ID: ${deviceId}`);
    console.log("    ✅ Device ID retrieved successfully");

    // Test 4: Activate license
    console.log("[4] Activating license...");
    const activateResult = client.activate();
    console.log(`    Success: ${activateResult.success}`);
    console.log(`    Message: ${activateResult.message}`);
    console.log("    ✅ License activated successfully");

    // Test 5: Check activation status
    console.log("[5] Checking activation status...");
    const isActivated = client.isActivated();
    console.log(`    Activated: ${isActivated}`);
    console.log("    ✅ Activation status checked successfully");

    // Test 6: Check device state
    console.log("[6] Checking device state...");
    const deviceState = client.getDeviceState();
    console.log(`    Device State: ${deviceState}`);
    console.log("    ✅ Device state retrieved successfully");

    // Test 7: Get current token
    console.log("[7] Getting current token...");
    const token = client.getCurrentToken();
    if (token) {
        console.log(`    Token ID: ${token.tokenId}`);
        console.log(`    Holder: ${token.holderDeviceId}`);
    } else {
        console.log("    No token available");
    }
    console.log("    ✅ Token retrieved successfully");

    // Test 8: Shutdown client
    console.log("[8] Shutting down client...");
    const shutdownResult = client.shutdown();
    console.log(`    Shutdown successful: ${shutdownResult}`);
    console.log("    ✅ Client shutdown successfully");

    console.log("\n🎉 All tests passed!");
    process.exit(0);

} catch (error) {
    console.error(`❌ Error: ${error.message}`);
    process.exit(1);
}