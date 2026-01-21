# DecentriLicense Python SDK

Python bindings for the DecentriLicense C++ SDK.

## Installation

To install the Python SDK, you first need to build the C++ core library and C bindings:

```bash
# From the project root directory
mkdir build
cd build
cmake ..
make
```

Then install the Python package:

```bash
# From the bindings/python directory
pip install .
```

## Usage

```python
from decenlicense import DecentriLicenseClient

# Create and initialize the client
client = DecentriLicenseClient()
client.initialize("YOUR-LICENSE-CODE")

# Activate the license
result = client.activate()

# Check if activated
if client.is_activated():
    token = client.get_current_token()
    print(f"License activated with token: {token['token_id']}")

# Clean up
client.shutdown()
```

Or using the context manager:

```python
from decenlicense import DecentriLicenseClient

with DecentriLicenseClient() as client:
    client.initialize("YOUR-LICENSE-CODE")
    client.activate()
    
    if client.is_activated():
        token = client.get_current_token()
        print(f"License activated with token: {token['token_id']}")
```

## API Reference

### DecentriLicenseClient

#### Methods

- `initialize(license_code, udp_port=8888, tcp_port=8889, registry_server_url=None)` - Initialize the client
- `activate()` - Activate the license
- `get_current_token()` - Get the current token if activated
- `is_activated()` - Check if the license is activated
- `get_device_id()` - Get the device ID
- `get_device_state()` - Get the current device state
- `shutdown()` - Shutdown the client and release resources

## License

MIT License. See LICENSE file for details.