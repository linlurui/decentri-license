# DecentriLicense Node.js SDK

This SDK provides Node.js bindings for the DecentriLicense licensing system.

## Prerequisites

- Node.js v12 or higher
- npm v6 or higher
- Python 3.x (for node-gyp)
- C++ compiler (for building native addons)

## Installation

1. Install dependencies:
   ```bash
   npm install
   ```

2. If you encounter library loading issues, you may need to set the library path:
   ```bash
   export DYLD_LIBRARY_PATH=/path/to/decentri-license-issuer/dl-core/build:$DYLD_LIBRARY_PATH
   ```

## Usage

```javascript
const { DecentriLicenseClient } = require('./build/Release/decenlicense_node');

// Create a new client instance
const client = new DecentriLicenseClient();

// Initialize the client
const initResult = client.initialize({
    licenseCode: "YOUR_LICENSE_CODE",
    udpPort: 8888,
    tcpPort: 8889,
    registryServerUrl: ""
});

console.log("Initialization result:", initResult);

// Activate the license
const activationResult = client.activate();
console.log("Activation result:", activationResult);

// Check if activated
console.log("Is activated:", client.isActivated());

// Get device information
console.log("Device ID:", client.getDeviceId());
console.log("Device state:", client.getDeviceState());

// Get current token (if available)
const token = client.getCurrentToken();
if (token) {
    console.log("Current token:", token);
}

// Clean up
client.shutdown();
```

## Example

See [example.js](file:///Volumes/project/decentri-license-issuer/sdks/nodejs/example.js) for a complete example of how to use the SDK.

## API Reference

### DecentriLicenseClient

#### constructor()
Creates a new DecentriLicenseClient instance.

#### initialize(options)
Initializes the client with the provided options.

Parameters:
- `options`: Object containing initialization options
  - `licenseCode`: String - The license code to use
  - `udpPort`: Number - UDP port for communication
  - `tcpPort`: Number - TCP port for communication
  - `registryServerUrl`: String - Registry server URL

Returns: Object with success status and message.

#### activate()
Activates the license.

Returns: Object with success status and message.

#### isActivated()
Checks if the license is activated.

Returns: Boolean indicating activation status.

#### getDeviceId()
Gets the device ID.

Returns: String representing the device ID.

#### getDeviceState()
Gets the current device state.

Returns: String representing the device state.

#### getCurrentToken()
Gets the current token if available.

Returns: Object representing the current token, or null if not available.

#### shutdown()
Shuts down the client and cleans up resources.

Returns: Boolean indicating success.

## Building from Source

If you need to rebuild the native addon:

```bash
npm install
```

Or manually with node-gyp:

```bash
node-gyp configure
node-gyp build
```

## Troubleshooting

### Library Loading Issues

If you encounter errors like "Library not loaded", make sure the dl-core library is built and accessible:

1. Build dl-core:
   ```bash
   cd ../../dl-core
   mkdir -p build
   cd build
   cmake ..
   make
   ```

2. Set the library path:
   ```bash
   export DYLD_LIBRARY_PATH=/path/to/decentri-license-issuer/dl-core/build:$DYLD_LIBRARY_PATH
   ```

## License

This SDK is licensed under the MIT License. See the LICENSE file for details.